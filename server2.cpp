#include "httplib.h"
#include <libpq-fe.h>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <list>
#include <string>
#include <memory>
#include <thread>
#include <vector>

using namespace std;

// LRU Cache Implementation
class LRUCache {
private:
    size_t capacity;
    unordered_map<string, pair<string, list<string>::iterator>> cache;
    list<string> lru_list;
    mutex mtx;

public:
    LRUCache(size_t cap) : capacity(cap) {}

    bool get(const string& key, string& value) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(key);
        if (it == cache.end()) {
            return false;
        }
        // Move to front (most recently used)
        lru_list.erase(it->second.second);
        lru_list.push_front(key);
        it->second.second = lru_list.begin();
        value = it->second.first;
        return true;
    }

    void put(const string& key, const string& value) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(key);
        
        if (it != cache.end()) {
            // Update existing key
            lru_list.erase(it->second.second);
            lru_list.push_front(key);
            it->second = {value, lru_list.begin()};
        } else {
            // Insert new key
            if (cache.size() >= capacity) {
                // Evict LRU item
                string lru_key = lru_list.back();
                lru_list.pop_back();
                cache.erase(lru_key);
            }
            lru_list.push_front(key);
            cache[key] = {value, lru_list.begin()};
        }
    }

    void remove(const string& key) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(key);
        if (it != cache.end()) {
            lru_list.erase(it->second.second);
            cache.erase(it);
        }
    }

    size_t size() {
        lock_guard<mutex> lock(mtx);
        return cache.size();
    }
};

// Database Connection Pool
class DBPool {
private:
    vector<PGconn*> connections;
    mutex mtx;
    string conn_info;
    size_t pool_size;

public:
    DBPool(const string& conn_str, size_t size) : conn_info(conn_str), pool_size(size) {
        for (size_t i = 0; i < pool_size; ++i) {
            PGconn* conn = PQconnectdb(conn_info.c_str());
            if (PQstatus(conn) != CONNECTION_OK) {
                cerr << "Connection to database failed: " << PQerrorMessage(conn) << endl;
                PQfinish(conn);
            } else {
                connections.push_back(conn);
            }
        }
        
        // Create table if not exists
        if (!connections.empty()) {
            PGconn* conn = connections[0];
            const char* create_table = "CREATE TABLE IF NOT EXISTS kv_store (key VARCHAR(255) PRIMARY KEY, value TEXT)";
            PGresult* res = PQexec(conn, create_table);
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                cerr << "Create table failed: " << PQerrorMessage(conn) << endl;
            }
            PQclear(res);
        }
    }

    ~DBPool() {
        for (auto conn : connections) {
            PQfinish(conn);
        }
    }

    PGconn* getConnection() {
        lock_guard<mutex> lock(mtx);
        if (connections.empty()) return nullptr;
        PGconn* conn = connections.back();
        connections.pop_back();
        return conn;
    }

    void releaseConnection(PGconn* conn) {
        lock_guard<mutex> lock(mtx);
        connections.push_back(conn);
    }
};

// KV Server
class KVServer {
private:
    LRUCache cache;
    shared_ptr<DBPool> db_pool;

public:
    KVServer(size_t cache_capacity, shared_ptr<DBPool> pool) 
        : cache(cache_capacity), db_pool(pool) {}

    // CREATE operation
    pair<bool, string> create(const string& key, const string& value) {
        PGconn* conn = db_pool->getConnection();
        if (!conn) {
            return {false, "Database connection failed"};
        }

        // Insert into database
        const char* paramValues[2] = {key.c_str(), value.c_str()};
        PGresult* res = PQexecParams(conn,
            "INSERT INTO kv_store (key, value) VALUES ($1, $2) ON CONFLICT (key) DO UPDATE SET value = $2",
            2, nullptr, paramValues, nullptr, nullptr, 0);

        bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
        string error = success ? "" : PQerrorMessage(conn);
        
        PQclear(res);
        db_pool->releaseConnection(conn);

        if (success) {
            // Insert into cache
            cache.put(key, value);
        }

        return {success, error};
    }

    // READ operation
    pair<bool, string> read(const string& key) {
        string value;
        
        // Check cache first
        if (cache.get(key, value)) {
            return {true, value};
        }

        // Cache miss - fetch from database
        PGconn* conn = db_pool->getConnection();
        if (!conn) {
            return {false, "Database connection failed"};
        }

        const char* paramValues[1] = {key.c_str()};
        PGresult* res = PQexecParams(conn,
            "SELECT value FROM kv_store WHERE key = $1",
            1, nullptr, paramValues, nullptr, nullptr, 0);

        bool success = false;
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
            value = PQgetvalue(res, 0, 0);
            success = true;
            // Insert into cache
            cache.put(key, value);
        } else {
            value = "Key not found";
        }

        PQclear(res);
        db_pool->releaseConnection(conn);

        return {success, value};
    }

    // DELETE operation
    pair<bool, string> remove(const string& key) {
        PGconn* conn = db_pool->getConnection();
        if (!conn) {
            return {false, "Database connection failed"};
        }

        const char* paramValues[1] = {key.c_str()};
        PGresult* res = PQexecParams(conn,
            "DELETE FROM kv_store WHERE key = $1",
            1, nullptr, paramValues, nullptr, nullptr, 0);

        bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
        string error = success ? "" : PQerrorMessage(conn);

        PQclear(res);
        db_pool->releaseConnection(conn);

        if (success) {
            // Remove from cache
            cache.remove(key);
        }

        return {success, error};
    }
};

int main() {
    // Database connection string - modify as needed
    string db_conn = "host=localhost port=5432 dbname=kvstore user=postgres password=postgres";
    
    auto db_pool = make_shared<DBPool>(db_conn, 20);
    KVServer kv_server(1000, db_pool);  // Cache capacity of 1000

    httplib::Server svr;

    // Set thread pool size
    svr.new_task_queue = [] { return new httplib::ThreadPool(8); };

    // POST /kv?id=<key> - Create key-value pair (body contains value)
    svr.Post("/kv", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Missing id parameter\n", "text/plain");
            return;
        }

        string key = req.get_param_value("id");
        string value = req.body;

        auto [success, msg] = kv_server.create(key, value);
        if (success) {
            res.status = 200;
            res.set_content("Key-value pair created successfully\n", "text/plain");
        } else {
            res.status = 500;
            res.set_content("Error: " + msg + "\n", "text/plain");
        }
    });

    // GET /kv?id=<key> - Read key-value pair
    svr.Get("/kv", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Missing id parameter\n", "text/plain");
            return;
        }

        string key = req.get_param_value("id");
        auto [success, value] = kv_server.read(key);
        
        if (success) {
            res.status = 200;
            res.set_content(value + "\n", "text/plain");
        } else {
            res.status = 404;
            res.set_content("Key not found\n", "text/plain");
        }
    });

    // DELETE /kv?id=<key> - Delete key-value pair
    svr.Delete("/kv", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Missing id parameter\n", "text/plain");
            return;
        }

        string key = req.get_param_value("id");
        auto [success, msg] = kv_server.remove(key);
        
        if (success) {
            res.status = 200;
            res.set_content("Key-value pair deleted successfully\n", "text/plain");
        } else {
            res.status = 500;
            res.set_content("Error: " + msg + "\n", "text/plain");
        }
    });

    cout << "Server starting on http://localhost:8000" << endl;
    svr.listen("0.0.0.0", 8000);

    return 0;
}
#include "httplib.h"
#include <libpq-fe.h>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <list>
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <stdexcept> 

using namespace std;

//LRU Cache 
class LRUCache {
private:
    size_t capacity;
    unordered_map<int, pair<string, list<int>::iterator>> cache;
    list<int> lru_list;
    mutex mtx;

public:
    LRUCache(size_t cap) : capacity(cap) {}

    bool get(int key, string& value) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(key);
        if (it == cache.end()) {
            return false;
        }
        lru_list.erase(it->second.second);
        lru_list.push_front(key);
        it->second.second = lru_list.begin();
        value = it->second.first;
        return true;
    }


    void put(int key, const string& value) {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(key);
        
        if (it != cache.end()) {
            lru_list.erase(it->second.second);
            lru_list.push_front(key);
            it->second = {value, lru_list.begin()};
        } else {
            if (cache.size() >= capacity) {
                int lru_key = lru_list.back();
                lru_list.pop_back();
                cache.erase(lru_key);
            }
            lru_list.push_front(key);
            cache[key] = {value, lru_list.begin()};
        }
    }

    void remove(int key) {
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
    condition_variable cv;
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
        
        if (!connections.empty()) {
            PGconn* conn = connections[0];
            const char* create_table = "CREATE TABLE IF NOT EXISTS kv_store (key INTEGER PRIMARY KEY, value TEXT)";
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
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]{ return !connections.empty(); });
        PGconn* conn = connections.back();
        connections.pop_back();
        return conn;
    }

    void releaseConnection(PGconn* conn) {
        lock_guard<mutex> lock(mtx);
        connections.push_back(conn);
        cv.notify_one();    
    }
};


int main() {

    string db_conn = "host=localhost port=5432 dbname=decs user=postgres password=postgres";
    
    auto db_pool = make_shared<DBPool>(db_conn, 500);
    LRUCache cache(1000); 

    httplib::Server srv;
    
    srv.new_task_queue = [] { return new httplib::ThreadPool(500); };

    // create / update
    srv.Post("/create", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("key") || !req.has_param("value")) {
            res.status = 400;
            res.set_content("Missing key or value parameter", "text/plain");
            return;
        }

        cout << "create";
        string key_str = req.get_param_value("key");
        string value = req.get_param_value("value");
        

        int key = stoi(key_str); 
        PGconn* conn = db_pool->getConnection();
        if (!conn) {
            res.status = 503; 
            res.set_content("Database connection failed", "text/plain");
            return;
        }

        const char* paramValues[2] = {key_str.c_str(), value.c_str()};
        PGresult* db_res = PQexecParams(conn,
            "INSERT INTO kv_store (key, value) VALUES ($1, $2) ON CONFLICT (key) DO UPDATE SET value = $2",
            2, nullptr, paramValues, nullptr, nullptr, 0);

        bool success = (PQresultStatus(db_res) == PGRES_COMMAND_OK);
        string error = success ? "" : PQerrorMessage(conn);
        
        PQclear(db_res);
        db_pool->releaseConnection(conn);

        if (success) {
            cache.put(key, value);
            res.status = 200;
            res.set_content("Created", "text/plain");
        } else {
            res.status = 500;
            res.set_content("Error: " + error, "text/plain");
        }
    });

    // read
    srv.Get("/read", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("key")) {
            res.status = 400;
            res.set_content("Missing key parameter", "text/plain");
            return;
        }

        string key_str = req.get_param_value("key");
        
        int key = stoi(key_str);
        string value;
        if (cache.get(key, value)) {
            res.status = 200;
            res.set_content(value, "text/plain");
            return;
        }

        // Cache miss
        PGconn* conn = db_pool->getConnection();
        if (!conn) {
            res.status = 503;
            res.set_content("Database connection failed", "text/plain");
            return;
        }

        const char* paramValues[1] = {key_str.c_str()};
        PGresult* db_res = PQexecParams(conn,
            "SELECT value FROM kv_store WHERE key = $1",
            1, nullptr, paramValues, nullptr, nullptr, 0);

        bool success = false;
        if (PQresultStatus(db_res) == PGRES_TUPLES_OK && PQntuples(db_res) > 0) {
            value = PQgetvalue(db_res, 0, 0);
            success = true;
            cache.put(key, value);
        } else {
            value = "Key not found";
        }

        PQclear(db_res);
        db_pool->releaseConnection(conn);

        if (success) {
            res.status = 200;
            res.set_content(value, "text/plain");
        } else {
            res.status = 404;
            res.set_content(value, "text/plain");
        }
    });

    // DELETE 
    srv.Delete("/delete", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("key")) {
            res.status = 400;
            res.set_content("Missing key parameter", "text/plain");
            return;
        }

        string key_str = req.get_param_value("key");
        
        int key = stoi(key_str);

        PGconn* conn = db_pool->getConnection();
        if (!conn) {
            res.status = 503;
            res.set_content("Database connection failed", "text/plain");
            return;
        }

        const char* paramValues[1] = {key_str.c_str()};
        PGresult* db_res = PQexecParams(conn,
            "DELETE FROM kv_store WHERE key = $1",
            1, nullptr, paramValues, nullptr, nullptr, 0);

        bool success = (PQresultStatus(db_res) == PGRES_COMMAND_OK);
        string error = success ? "" : PQerrorMessage(conn);
        
        PQclear(db_res);
        db_pool->releaseConnection(conn);

        if (success) {
            cache.remove(key);
            res.status = 200;
            res.set_content("Deleted", "text/plain");
        } else {
            res.status = 500;
            res.set_content("Error: " + error, "text/plain");
        }
    });

    cout << "Server starting on http://localhost:8080" << endl;
    srv.listen("0.0.0.0", 8080);

    return 0;
}
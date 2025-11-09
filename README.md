# HTTP-based Key-Value Store Server

## 1. Overview
* **RESTful API** with Create, Read, Delete operations
* **PostgreSQL** backend for persistent storage
* **LRU Cache** for improved performance
* **Connection** pooling for database efficiency


## 2. Requirements
* C++17 Compiler (g++)
* PostgreSQL Server
* `cpp-httplib` 
* `libpq-dev` (The PostgreSQL C client library)

## 3. API Endpoints

The server has the following three endpoints on `http://localhost:8000`:

* **CREATE**: `POST /create?key=<key>&value=<value>`
    * Inserts a new key-value pair. If the key already exists, it updates the value.
* **READ**: `GET /read?key=<key>`
    * Retrieves the value for a given key. It first checks the LRU cache. If not found (cache miss), it fetches from the database and populates the cache.
* **DELETE**: `DELETE /delete?key=<key>`
    * Deletes a key-value pair from both the database and the LRU cache.

### 4. Compilation
    g++ -std=c++17 server.cpp -o server $(pkg-config --cflags libpq) -lpq -lssl -lcrypto -lpthread
    g++ client.cpp -o client

=======
# HTTP-based Key-Value Store Server

## 1. Overview
* **RESTful API** with Create, Read, Delete operations
* **PostgreSQL** backend for persistent storage
* **LRU Cache** for improved performance
* **Connection** pooling for database efficiency


## 2. Requirements
* C++17 Compiler (g++)
* PostgreSQL Server
* `cpp-httplib` 
* `libpq-dev` (The PostgreSQL C client library)

## 3. API Endpoints

The server has the following three endpoints on `http://localhost:8000`:

* **CREATE**: `POST /create?key=<key>&value=<value>`
    * Inserts a new key-value pair. If the key already exists, it updates the value.
* **READ**: `GET /read?key=<key>`
    * Retrieves the value for a given key. It first checks the LRU cache. If not found (cache miss), it fetches from the database and populates the cache.
* **DELETE**: `DELETE /delete?key=<key>`
    * Deletes a key-value pair from both the database and the LRU cache.

### 4. Compilation
    g++ -std=c++17 server.cpp -o server $(pkg-config --cflags libpq) -lpq -lssl -lcrypto -lpthread
    g++ client.cpp -o client

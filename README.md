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

The server has the following three endpoints on `http://localhost:8080`:

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
* `k6` (Third party library for load test)

## 3. API Endpoints

The server has the following three endpoints on `http://localhost:8080`:

* **CREATE**: `POST /create?key=<key>&value=<value>`
    * Inserts a new key-value pair. If the key already exists, it updates the value.
* **READ**: `GET /read?key=<key>`
    * Retrieves the value for a given key. It first checks the LRU cache. If not found (cache miss), it fetches from the database and populates the cache.
* **DELETE**: `DELETE /delete?key=<key>`
    * Deletes a key-value pair from both the database and the LRU cache.

### 4. Compilation
    g++ -std=c++17 server.cpp -o server $(pkg-config --cflags libpq) -lpq -lssl -lcrypto -lpthread
    g++ client.cpp -o client

### 5.Run Commands
*  **Run Server** : Here we are pinning the server to core 1
    taskset -c 1 ./server
*  **Pin postgres** : Here we are pinning the db to core 0
    pgrep -f postgres | xargs -I {} sudo taskset -cp 0 {}
* **Run run.sh script** : Here we are pinning the load generator to cores - 7,8,9
    taskset -c 7-9 ./run.sh

### 6.Plot Generation Command
    python3 plot_generator.py



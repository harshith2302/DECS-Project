#include "httplib.h"
#include <iostream>
#include <string>


using namespace std;

int main() {
    
    httplib::Client cli("http://127.0.0.1:8000");

    

    cout << "Connected to KV Store at http://127.0.0.1:8000" << endl;

    while (true) {
        cout << "1) Create \n"
             << "2) Read\n"
             << "3) Delete\n"
             << "4) Exit\n"
             << "Choice: ";
        
        int ch;
        cin >> ch;

        
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

        if (ch == 4) {
            break;
        }

        string key, value;

        if (ch == 1) {
            cout << "Enter key : ";
            getline(cin, key);
            cout << "Enter value : ";
            getline(cin, value);

            auto res = cli.Post("/create?key="+key+"&value="+value);

            if (res) {
                cout << "Response: " << res->body << endl;
            } else {
                auto err = res.error();
                cout << "Request failed: " << httplib::to_string(err) << endl;
            }

        } else if (ch == 2) {
            cout << "Enter key : ";
            getline(cin, key);

            string path = "/read?key=" + key;

            auto res = cli.Get(path);

            if (res) {
                cout << "Response: " << res->body << endl;
            } else {
                auto err = res.error();
                cout << "Request failed: " << httplib::to_string(err) << endl;
            }

        } else if (ch == 3) {
            cout << "Enter key : ";
            getline(cin, key);

            string path = "/delete?key=" + key;

            auto res = cli.Delete(path);

            if (res) {
                cout << "Response: " << res->body << endl;
            } else {
                auto err = res.error();
                cout << "Request failed: " << httplib::to_string(err) << endl;
            }

        } else {
            cout << "Invalid choice..." << endl;
        }
    }

    return 0;
}
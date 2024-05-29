#include <iostream>
#include <pqxx/pqxx> 
#include "websocket_server.h" 

void clearDatabase(pqxx::connection& conn) {
    pqxx::work txn(conn);
    txn.exec("TRUNCATE TABLE messages RESTART IDENTITY CASCADE;");
    txn.commit();
}

int main() {
    try {
        pqxx::connection conn("dbname=chatapp user=postgres password=****** hostaddr=127.0.0.1 port=5432");

        if (conn.is_open()) {
            std::cout << "Connected to database successfully." << std::endl;
        }
        else {
            std::cerr << "Failed to connect to database." << std::endl;
            return 1;
        }

        WebSocketServer server(conn);
    
        clearDatabase(conn);

        server.start();

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

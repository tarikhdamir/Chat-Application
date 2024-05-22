#include "websocket_server.h"
#include <iostream>
#include <sstream>

WebSocketServer::WebSocketServer(pqxx::connection& db_conn)
    : db_conn_(db_conn) {
    ws_server_.init_asio();

    ws_server_.set_open_handler(bind(&WebSocketServer::on_open, this, std::placeholders::_1));
    ws_server_.set_close_handler(bind(&WebSocketServer::on_close, this, std::placeholders::_1));
    ws_server_.set_message_handler(bind(&WebSocketServer::on_message, this, std::placeholders::_1, std::placeholders::_2));

    db_conn_.prepare("insert_message", "INSERT INTO messages (room, username, message) VALUES ($1, $2, $3)");
    db_conn_.prepare("select_messages", "SELECT username, message FROM messages WHERE room = $1 ORDER BY id ASC");
}

void WebSocketServer::start() {
    ws_server_.listen(9002);
    ws_server_.start_accept();
    ws_server_.run();
}

void WebSocketServer::on_open(websocketpp::connection_hdl hdl) {
    std::cout << "New connection opened" << std::endl;
}

void WebSocketServer::on_close(websocketpp::connection_hdl hdl) {
    std::cout << "Connection closed" << std::endl;
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::string message = msg->get_payload();
    std::cout << "Received message: " << message << std::endl;

    if (message.substr(0, 8) == "CONNECT:") {
        std::string room = message.substr(8);
        joinRoom(room, hdl);
        sendMessageHistory(room, hdl); 
    }
    else {
        std::istringstream iss(message);
        std::string username, room, msg_content;
        if (std::getline(iss, username, ':') && std::getline(iss, room, ':') && std::getline(iss, msg_content)) {
            joinRoom(room, hdl);
            std::string broadcast_message = username + ": " + msg_content;
            broadcastMessage(broadcast_message, room);

            
            try {
                pqxx::work txn(db_conn_);
                txn.exec_prepared("insert_message", room, username, msg_content);
                txn.commit();
            }
            catch (const std::exception& e) {
                std::cerr << "Error saving message to database: " << e.what() << std::endl;
            }
        }
        else {
            std::cerr << "Invalid message format received: " << message << std::endl;
        }
    }
}

void WebSocketServer::broadcastMessage(const std::string& message, const std::string& room) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (chatRooms_.find(room) != chatRooms_.end()) {
        std::cout << "Broadcasting message to room " << room << ": " << message << std::endl;
        for (auto hdl : chatRooms_[room]) {
            ws_server_.send(hdl, message, websocketpp::frame::opcode::text);
        }
    }
    else {
        std::cout << "Room " << room << " not found" << std::endl;
    }
}

void WebSocketServer::joinRoom(const std::string& room, websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(mutex_);
    chatRooms_[room].insert(hdl);
    std::cout << "Client joined room " << room << std::endl;
}

void WebSocketServer::leaveRoom(const std::string& room, websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (chatRooms_.find(room) != chatRooms_.end()) {
        chatRooms_[room].erase(hdl);
        std::cout << "Client left room " << room << std::endl;
    }
}

void WebSocketServer::sendMessageHistory(const std::string& room, websocketpp::connection_hdl hdl) {
    try {
        pqxx::work txn(db_conn_);
        pqxx::result result = txn.exec_prepared("select_messages", room);

        for (const auto& row : result) {
            std::string username = row["username"].c_str();
            std::string message = row["message"].c_str();
            std::string full_message = username + ": " + message;
            ws_server_.send(hdl, full_message, websocketpp::frame::opcode::text);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error fetching message history: " << e.what() << std::endl;
    }
}

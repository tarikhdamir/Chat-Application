#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <pqxx/pqxx>
#include <string>
#include <map>
#include <set>
#include <thread>
#include <mutex>

typedef websocketpp::server<websocketpp::config::asio> server;

class WebSocketServer {
public:
    WebSocketServer(pqxx::connection& db_conn);
    void start();
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

private:
    server ws_server_;
    pqxx::connection& db_conn_;
    std::map<std::string, std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>> chatRooms_;
    std::mutex mutex_;

    void broadcastMessage(const std::string& message, const std::string& room);
    void joinRoom(const std::string& room, websocketpp::connection_hdl hdl);
    void leaveRoom(const std::string& room, websocketpp::connection_hdl hdl);
    void sendMessageHistory(const std::string& room, websocketpp::connection_hdl hdl);
};

#endif // WEBSOCKET_SERVER_H

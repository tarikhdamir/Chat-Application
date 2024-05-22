#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <string>
#include <map>
#include <set>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#endif

class ChatServer {
public:
    ChatServer();
    ~ChatServer();
    void start();
    void stop();
    void broadcastMessage(const std::string& message, const std::string& room);

private:
    socket_t server_socket_;
    std::map<std::string, std::set<socket_t>> chatRooms_;
    std::mutex mutex_;
    bool running_;

    void handleClient(socket_t clientSocket);
    void joinRoom(const std::string& room, socket_t clientSocket);
    void leaveRoom(const std::string& room, socket_t clientSocket);
    void cleanup();
};

#endif // CHAT_SERVER_H

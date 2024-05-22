#include "chat_server.h"
#include <iostream>
#include <cstring>

ChatServer::ChatServer()
    : running_(true) {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

ChatServer::~ChatServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void ChatServer::start() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR,
#ifdef _WIN32
    (const char*)&opt, sizeof(opt)
#else
        & opt, sizeof(opt)
#endif
    ) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_socket_, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started, waiting for connections..." << std::endl;

    while (running_) {
        socket_t client_socket;
        if ((client_socket = accept(server_socket_, (struct sockaddr*)&address,
            (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        std::thread(&ChatServer::handleClient, this, client_socket).detach();
    }

    cleanup();
}

void ChatServer::stop() {
    running_ = false;
    shutdown(server_socket_, 2); 
#ifdef _WIN32
    closesocket(server_socket_);
#else
    close(server_socket_);
#endif
}

void ChatServer::broadcastMessage(const std::string& message, const std::string& room) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (chatRooms_.find(room) != chatRooms_.end()) {
        for (socket_t clientSocket : chatRooms_[room]) {
            send(clientSocket, message.c_str(), message.size(), 0);
        }
    }
}

void ChatServer::handleClient(socket_t clientSocket) {
    char buffer[1024] = { 0 };
    while (true) {
        int valread = recv(clientSocket, buffer, 1024, 0);
        if (valread <= 0) {
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            break;
        }
        std::string message(buffer, valread);
        auto delimiterPos = message.find(':');
        std::string room = message.substr(0, delimiterPos);
        std::string msg = message.substr(delimiterPos + 1);
        broadcastMessage(msg, room);
    }
}

void ChatServer::joinRoom(const std::string& room, socket_t clientSocket) {
    std::lock_guard<std::mutex> lock(mutex_);
    chatRooms_[room].insert(clientSocket);
}

void ChatServer::leaveRoom(const std::string& room, socket_t clientSocket) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (chatRooms_.find(room) != chatRooms_.end()) {
        chatRooms_[room].erase(clientSocket);
    }
}

void ChatServer::cleanup() {
#ifdef _WIN32
    closesocket(server_socket_);
#else
    close(server_socket_);
#endif
}

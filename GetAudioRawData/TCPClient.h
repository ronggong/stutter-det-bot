#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")  // Link with Ws2_32.lib

class TCPClient {
private:
    SOCKET s;
    sockaddr_in server;

public:
    TCPClient(const std::string& ip, int port) {
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            exit(EXIT_FAILURE);
        }

        // Create socket
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        // Setup server address
        server.sin_addr.s_addr = inet_addr(ip.c_str());
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        // Connect to server
        if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
            std::cerr << "Connection failed\n";
            closesocket(s);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        std::cout << "Connected to server " << ip << " on port " << port << "\n";
    }

    ~TCPClient() {
        closesocket(s);  // Close socket on destruction
        WSACleanup();
    }

    void sendMessage(const std::string& message) {
        if (send(s, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Send failed\n";
        }
        else {
            std::cout << "Message sent: " << message << "\n";
        }
    }
};
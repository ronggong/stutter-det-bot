#pragma once

#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>

// Alias for WebSocket++ client
typedef websocketpp::client<websocketpp::config::asio_client> client;

class WebSocketSender {
public:
    WebSocketSender();

    void run(const std::string& uri);
    void sendMessage(const std::string& message);
    void start(const std::string& uri);
    void join();

private:
    client wsClient;
    websocketpp::connection_hdl connectionHandle;
    std::mutex mutex_;  // Mutex to protect shared resources
    bool isConnected;
    std::thread wsThread;

    // Function called when WebSocket connection is opened
    void handleOpen(websocketpp::connection_hdl hdl);
};

//int main() {
//    std::string uri = "ws://localhost:8080";

//    WebSocketSender senderClient;
//    senderClient.run(uri);

//    return 0;
//}

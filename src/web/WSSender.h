#pragma once

#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>

// Alias for WebSocket++ client
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

class WebSocketSender {
public:
    WebSocketSender();
    ~WebSocketSender();

    void run(const std::string& uri);
    void setHandlers(const std::string& pairId);
    void sendMessage(const std::string& message);
    void start(const std::string& uri);
    void stop();

private:
    void handleClose(websocketpp::connection_hdl hdl);
    void reconnectLoop();
    // Function called when WebSocket connection is opened
    void handleOpen(websocketpp::connection_hdl hdl, const std::string& pairId);

private:
    std::unique_ptr<client> wsClient;
    websocketpp::connection_hdl connectionHandle;
    std::atomic<bool> needReconnect_;
    std::atomic<bool> stopReconnection_;
    std::atomic<bool> isConnected;
    std::mutex mutex_;  // Mutex to protect shared resources
    std::thread wsThread;
    std::thread reconnectThread;
    std::string pairId_ = "0";
	std::string uri_;
	std::atomic<size_t> reconnectAttempts_;
    static const size_t maxReconnectAttempts_ = 5;
    static const size_t reconnectDelay_ = 1;

};

//int main() {
//    std::string uri = "ws://localhost:8080";

//    WebSocketSender senderClient;
//    senderClient.run(uri);

//    return 0;
//}

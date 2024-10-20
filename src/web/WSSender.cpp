#include "WSSender.h"
#include <thread>
#include <mutex>

WebSocketSender::WebSocketSender() {
    // Initialize ASIO transport
    wsClient.init_asio();

    // Set up connection handler
    wsClient.set_open_handler([this](websocketpp::connection_hdl hdl) {
        handleOpen(hdl);
        });

    // Set up fail handler
    wsClient.set_fail_handler([](websocketpp::connection_hdl hdl) {
        std::cerr << "Connection failed." << std::endl;
        });

    // Set up close handler
    wsClient.set_close_handler([](websocketpp::connection_hdl hdl) {
        std::cout << "Connection closed." << std::endl;
        });
}

void WebSocketSender::run(const std::string& uri) {
    websocketpp::lib::error_code ec;

    // Create a connection to the given URI
    client::connection_ptr con = wsClient.get_connection(uri, ec);

    if (ec) {
        std::cerr << "Could not create connection: " << ec.message() << std::endl;
        return;
    }

    // Start the connection
    wsClient.connect(con);
    wsClient.run();
}

// Start the WebSocket thread
void WebSocketSender::start(const std::string& uri) {
    wsThread = std::thread(&WebSocketSender::run, this, uri);
}

void WebSocketSender::join() {
    if (wsThread.joinable()) {
        wsThread.join();
    }
}

// Send a message to the server
void WebSocketSender::sendMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);  // Lock the mutex to ensure thread safety
    if (isConnected) {
        // Send the message to the server
        wsClient.send(connectionHandle, message, websocketpp::frame::opcode::text);
        std::cout << "Message sent: " << message << std::endl;
    }
    else {
        std::cerr << "Cannot send message, WebSocket is not connected yet." << std::endl;
    }
}

// Function called when WebSocket connection is opened
void WebSocketSender::handleOpen(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "Connected to the server, registering as sender..." << std::endl;
	connectionHandle = hdl;
	isConnected = true;

    // Register as a sender
    wsClient.send(hdl, "sender", websocketpp::frame::opcode::text);
}
#include "WSSender.h"
#include <thread>
#include <mutex>

WebSocketSender::WebSocketSender() {
    // Initialize ASIO transport
    wsClient.init_asio();
}

void WebSocketSender::setHandlers(const std::string& pairId) {
	pairId_ = pairId;
    // Set up TLS handler to manage SSL context
    wsClient.set_tls_init_handler([](websocketpp::connection_hdl) {
        auto ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(
            boost::asio::ssl::context::tlsv12_client
        );

        // Configure SSL context as needed (e.g., for certificates)
        //ctx->set_verify_mode(boost::asio::ssl::verify_peer);
        ctx->set_verify_mode(boost::asio::ssl::verify_none); // Not recommended for production
        ctx->set_default_verify_paths();

        return ctx;    
    });
    // Set up connection handler
    wsClient.set_open_handler([this, pairId](websocketpp::connection_hdl hdl) {
        handleOpen(hdl, pairId);
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
        auto msg = message + ":" + pairId_;
        wsClient.send(connectionHandle, msg, websocketpp::frame::opcode::text);
        std::cout << "Message sent: " << msg << std::endl;
    }
    else {
        std::cerr << "Cannot send message, WebSocket is not connected yet." << std::endl;
    }
}

// Function called when WebSocket connection is opened
void WebSocketSender::handleOpen(websocketpp::connection_hdl hdl, const std::string& pairId) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "Connected to the server, registering as sender..." << std::endl;
	connectionHandle = hdl;
	isConnected = true;

	// Register as a sender, with pairId = 0, as the server can handle multiple senders
    wsClient.send(hdl, "sender:" + pairId, websocketpp::frame::opcode::text);
}
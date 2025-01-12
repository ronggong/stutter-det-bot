#include "WSSender.h"
#include <thread>
#include <mutex>

WebSocketSender::WebSocketSender() : isConnected(false), reconnectAttempts_(0), needReconnect_(false), stopReconnection_(false) {
    // Initialize ASIO transport
	wsClient = std::make_unique<client>();
    wsClient->init_asio();
    // Start the reconnection thread
    reconnectThread = std::thread(&WebSocketSender::reconnectLoop, this);
}

WebSocketSender::~WebSocketSender() {
    stopReconnection_ = true; // Signal reconnection thread to stop
    if (reconnectThread.joinable()) {
        reconnectThread.join();
    }
    stop(); // Ensure WebSocket thread is stopped
    if (wsThread.joinable()) {
        wsThread.join();
    }
}

void WebSocketSender::setHandlers(const std::string& pairId) {
	pairId_ = pairId;
    // Set up TLS handler to manage SSL context
    wsClient->set_tls_init_handler([](websocketpp::connection_hdl) {
        std::cout << "TLS initialization handler set" << std::endl;

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
    wsClient->set_open_handler([this, pairId](websocketpp::connection_hdl hdl) {
        handleOpen(hdl, pairId);
        });

    // Set up fail handler
    wsClient->set_fail_handler([this](websocketpp::connection_hdl hdl) {
        std::cerr << "Connection failed." << std::endl;
        handleClose(hdl);
    });

    // Set up close handler
    wsClient->set_close_handler([this](websocketpp::connection_hdl hdl) {
        std::cout << "Connection closed." << std::endl;
        handleClose(hdl);
    });
}

void WebSocketSender::run(const std::string& uri) {
    websocketpp::lib::error_code ec;

    // Create a connection to the given URI
    client::connection_ptr con = wsClient->get_connection(uri, ec);

    if (!ec) {
        websocketpp::session::state::value state = con->get_state();
        if (state == websocketpp::session::state::closed) {
            std::cout << "The WebSocket client is stopped (closed)." << std::endl;
        }
        else {
            std::cout << "The WebSocket client state: " << state << std::endl;
        }
    }

    if (ec) {
        std::cerr << "Could not create connection: " << ec.message() << std::endl;
        return;
    }

    std::cout << "TLS initialization should now be triggered if set" << std::endl;


    // Start the connection
    wsClient->connect(con);
    std::cout << "Running WebSocket client..." << std::endl;
    wsClient->run(); // Run the ASIO event loop
    std::cout << "WebSocket run() completed" << std::endl;

}

// Start the WebSocket thread
void WebSocketSender::start(const std::string& uri) {
	uri_ = uri;
    wsThread = std::thread(&WebSocketSender::run, this, uri);
}

void WebSocketSender::stop() {
    wsClient->stop();
}

// Send a message to the server
void WebSocketSender::sendMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);  // Lock the mutex to ensure thread safety
    if (isConnected) {
        // Send the message to the server
        auto msg = message + ":" + pairId_;
        wsClient->send(connectionHandle, msg, websocketpp::frame::opcode::text);
        std::cout << "Message sent: " << msg << std::endl;
    }
    else {
        std::cerr << "Cannot send message, WebSocket is not connected yet." << std::endl;
    }
}

// Function called when WebSocket connection is opened
void WebSocketSender::handleOpen(websocketpp::connection_hdl hdl, const std::string& pairId) {
    std::lock_guard<std::mutex> lock(mutex_);

	connectionHandle = hdl;
	isConnected = true;
    reconnectAttempts_ = 0;

    std::cout << "Connected to the server, registering as sender... Reconnect attemps reset to " << reconnectAttempts_ << std::endl;

	// Register as a sender, with pairId = 0, as the server can handle multiple senders
    wsClient->send(hdl, "sender:" + pairId, websocketpp::frame::opcode::text);
}

void WebSocketSender::handleClose(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(mutex_);
    isConnected = false;
    needReconnect_ = true; // Trigger reconnection
    stop();
}

void WebSocketSender::reconnectLoop() {
    while (!stopReconnection_) {
        if (needReconnect_) {
            // Ensure the previous WebSocket thread is stopped
            if (wsThread.joinable()) {
                wsThread.join();
            }

            // Attempt to reconnect
            ++reconnectAttempts_;
            if (reconnectAttempts_ <= maxReconnectAttempts_) {
                std::cout << "Reconnecting attempt " << reconnectAttempts_ << "..." << std::endl;
				wsClient = std::make_unique<client>();
				wsClient->init_asio();
				setHandlers(pairId_);
                start(uri_); // Restart WebSocket thread
                needReconnect_ = false; // Clear reconnection flag
            }
            else {
                std::cerr << "Max reconnect attempts reached. Stopping reconnection attempts." << std::endl;
                needReconnect_ = false;
                stopReconnection_ = true; // Stop further reconnection attempts
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(reconnectDelay_));
    }
}
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <signal.h>

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "\n🛑 Stopping monitor..." << std::endl;
    running = false;
}

// Monitor OSC messages on a specific port
void monitorPort(int port, const std::string& deviceName) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "❌ Failed to create socket for " << deviceName << std::endl;
        return;
    }
    
    // Allow reuse of address
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
    
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "❌ Bind failed for " << deviceName << " on port " << port << std::endl;
        close(sockfd);
        return;
    }
    
    std::cout << "✅ Monitoring " << deviceName << " on port " << port << std::endl;
    
    char buffer[1024];
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    
    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity > 0 && FD_ISSET(sockfd, &readfds)) {
            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, 
                            (struct sockaddr *)&cliaddr, &len);
            if (n > 0) {
                // Get sender info
                char senderIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cliaddr.sin_addr, senderIP, INET_ADDRSTRLEN);
                int senderPort = ntohs(cliaddr.sin_port);
                
                // Parse OSC address
                std::string oscAddress = "?";
                float value = 0.0f;
                
                if (n >= 8 && buffer[0] == '/') {
                    oscAddress = std::string(buffer);
                    
                    // Find float value
                    for (int i = 4; i < n - 3; i++) {
                        if (buffer[i] == ',') {
                            i += 4;
                            if (i + 4 <= n) {
                                unsigned char* bytes = (unsigned char*)&buffer[i];
                                unsigned int intValue = (bytes[0] << 24) | (bytes[1] << 16) | 
                                                       (bytes[2] << 8) | bytes[3];
                                value = *(float*)&intValue;
                                break;
                            }
                        }
                    }
                }
                
                std::cout << "🎯 [" << deviceName << "] " 
                          << senderIP << ":" << senderPort 
                          << " → " << oscAddress 
                          << " = " << value << std::endl;
            }
        }
    }
    
    close(sockfd);
}

int main() {
    std::cout << "🔍 OSC Message Monitor" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << "Monitoring OSC messages on different ports..." << std::endl;
    std::cout << "Press Ctrl+C to stop.\n" << std::endl;
    
    // Register signal handler
    signal(SIGINT, signalHandler);
    
    // Start monitoring threads for different devices
    std::vector<std::thread> monitors;
    
    // Monitor common OSC ports
    monitors.emplace_back(monitorPort, 9000, "TouchDesigner/Default");
    monitors.emplace_back(monitorPort, 9001, "Ableton Live");
    monitors.emplace_back(monitorPort, 8000, "TouchOSC");
    monitors.emplace_back(monitorPort, 7000, "Max/MSP");
    monitors.emplace_back(monitorPort, 8001, "VCV Rack");
    
    // Wait for all threads
    for (auto& t : monitors) {
        t.join();
    }
    
    std::cout << "\n✅ Monitor stopped." << std::endl;
    return 0;
}

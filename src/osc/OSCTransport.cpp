#include "OSCTransport.h"
#include "OSCUDPTransport.h"
#include "OSCTCPTransport.h"

std::unique_ptr<OSCTransport> OSCTransportFactory::create(OSCTransport::Protocol protocol) {
    switch (protocol) {
        case OSCTransport::Protocol::UDP:
            return std::make_unique<OSCUDPTransport>();
            
        case OSCTransport::Protocol::TCP:
            return std::make_unique<OSCTCPTransport>();
            
        case OSCTransport::Protocol::MULTICAST:
            // TODO: Implement multicast transport
            return nullptr;
            
        case OSCTransport::Protocol::BROADCAST:
            // TODO: Implement broadcast transport
            return nullptr;
            
        default:
            return nullptr;
    }
}

std::vector<OSCTransport::Protocol> OSCTransportFactory::getSupportedProtocols() {
    return {
        OSCTransport::Protocol::UDP,
        OSCTransport::Protocol::TCP
        // Future: MULTICAST, BROADCAST
    };
}

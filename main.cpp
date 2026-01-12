#include <iostream>
#include <string>
#include "includes/router/ClientManager.hpp"

// Helper: feed data in small chunks (simulate TCP)
void feedInChunks(ClientManager &cm, int fd, const std::string &data, size_t chunkSize) {
    for (size_t i = 0; i < data.size(); i += chunkSize) {
        cm.onData(fd, data.substr(i, chunkSize));
    }
}

int main() {
    ClientManager cm;

    // 1️⃣ Add multiple clients
    cm.addClient(1);
    cm.addClient(2);
    cm.addClient(3);
    cm.addClient(4);

    std::cout << "Added 4 clients.\n";

    // 2️⃣ Good GET request (client 1)
    std::string goodGET = "GET / HTTP/1.0\r\nHost: a\r\n\r\n";
    feedInChunks(cm, 1, goodGET, 5);

    std::cout << "Client 1: " << (cm.hasRequest(1) ? "complete ✅" : "not complete ❌") << "\n";

    // 3️⃣ Good POST request (client 2)
    std::string goodPOST = "POST /submit HTTP/1.0\r\nHost: b\r\nContent-Length: 5\r\n\r\nhello";
    feedInChunks(cm, 2, goodPOST, 7);

    std::cout << "Client 2: " << (cm.hasRequest(2) ? "complete ✅" : "not complete ❌") << "\n";

    // 4️⃣ Malformed request (client 3) → should stay incomplete or error
    std::string badReq = "DELETE /resource HTTP/1.0 hello\r\nHost:\r\n\r\n";
    feedInChunks(cm, 3, badReq, 6);

    std::cout << "Client 3 (bad request): " 
              << (cm.hasRequest(3) ? "complete ❌ (should fail!)" : "not complete ✅") << "\n";

    // 5️⃣ POST request with wrong Content-Length (client 4)
    std::string wrongCL = "POST /upload HTTP/1.0\r\nHost: d\r\nContent-Length: 10\r\n\r\nabc";
    feedInChunks(cm, 4, wrongCL, 5);

    std::cout << "Client 4 (wrong CL): "
              << (cm.hasRequest(4) ? "complete ❌ (should fail!)" : "not complete ✅") << "\n";

    // 6️⃣ Reset client 2
    cm.reset(2);
    std::cout << "Client 2 reset: " << (!cm.hasRequest(2) ? "works ✅" : "failed ❌") << "\n";

    // 7️⃣ Remove client 1
    cm.removeClient(1);
    std::cout << "Client 1 removed: " << (!cm.hasRequest(1) ? "works ✅" : "failed ❌") << "\n";

    // 8️⃣ Feed unknown FD (99)
    cm.onData(99, "GET / HTTP/1.0\r\n\r\n");
    std::cout << "Unknown FD ignored ✅\n";

    // 9️⃣ Check parsed requests
    if (cm.hasRequest(2)) {
        Request r2 = cm.getRequest(2);
        std::cout << "Client 2 parsed path: " << r2.path << "\n";
    }
    if (cm.hasRequest(1)) {
        Request r1 = cm.getRequest(1);
        std::cout << "Client 1 parsed path: " << r1.path << "\n";
    }

    std::cout << "ClientManager + Parser end-to-end test completed.\n";
    return 0;
}

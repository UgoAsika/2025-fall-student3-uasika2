#include <iostream>
#include <string>
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
        return 1;
    }

    std::string host   = argv[1];
    int port           = std::stoi(argv[2]);
    std::string user   = argv[3];
    std::string room   = argv[4];

    Connection c;

    // connect
    c.connect(host, port);

    // --- rlogin ---
    Message loginMsg(TAG_RLOGIN, user);
    if (!c.send(loginMsg)) {
        std::cerr << "Failed to send rlogin";
        return 1;
    }

    Message loginResp;
    if (!c.receive(loginResp)) {
        std::cerr << "No response after rlogin";
        return 1;
    }

    if (loginResp.tag == TAG_ERR) {
        std::cerr << loginResp.data << std::endl;
        return 1;
    }

    if (loginResp.tag != TAG_OK) {
        std::cerr << loginResp.tag << std::endl;
        return 1;
    }

    // --- join room ---
    Message joinMsg(TAG_JOIN, room);
    if (!c.send(joinMsg)) {
        std::cerr << "Failed to send join";
        return 1;
    }

    Message joinResp;
    if (!c.receive(joinResp)) {
        std::cerr << "No response after join";
        return 1;
    }

    if (joinResp.tag == TAG_ERR) {
        std::cerr << joinResp.data << std::endl;
        return 1;
    }

    if (joinResp.tag != TAG_OK) {
        std::cerr << joinResp.tag << std::endl;
        return 1;
    }

    // --- receive loop ---
    Message incoming;
    while (c.receive(incoming)) {

        if (incoming.tag == TAG_DELIVERY) {
            // expected format: room:sender:message
            size_t a = incoming.data.find(':');
            size_t b = incoming.data.find(':', a + 1);

            std::string snd = incoming.data.substr(a + 1, b - a - 1);
            std::string text = incoming.data.substr(b + 1);

            std::cout << snd << ": " << text << std::endl;
        }
        else if (incoming.tag == TAG_ERR) {
            std::cerr << "Server message error: " << incoming.data;
            return 1;
        }
    }

    return 0;
}

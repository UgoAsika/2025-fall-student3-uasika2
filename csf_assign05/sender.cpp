#include <iostream>
#include <string>
#include <sstream>
#include "message.h"
#include "connection.h"
#include "client_util.h"

using std::string;
using std::cout;
using std::cerr;
using std::cin;
using std::getline;
using std::istringstream;
using std::stoi;

static Message interpret(const string &line, bool &ok) {
    Message m;
    ok = true;

    istringstream ss(line);
    string cmd;
    ss >> cmd;

    if (cmd.size() > 1 && cmd[0] == '/') {
        char c = cmd[1];

        if (c == 'q' && cmd == "/quit") {
            m.tag = TAG_QUIT;
            m.data = "bye";
            return m;
        }

        if (c == 'j' && cmd == "/join") {
            string room;
            if (!(ss >> room)) {
                cerr << "Invalid room\n";
                ok = false;
                return m;
            }
            m.tag = TAG_JOIN;
            m.data = room;
            return m;
        }

        if (c == 'l' && cmd == "/leave") {
            m.tag = TAG_LEAVE;
            m.data = "";
            return m;
        }

        cerr << "Invalid command\n";
        ok = false;
        return m;
    }

    // normal message
    if (line.size() > Message::MAX_LEN) {
        cerr << "Message exceeds max length\n";
        ok = false;
    } else {
        m.tag = TAG_SENDALL;
        m.data = line;
    }

    return m;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage: ./sender [server_address] [port] [username]\n";
        return 1;
    }

    string host   = argv[1];
    int port      = stoi(argv[2]);
    string user   = argv[3];

    Connection conn;
    conn.connect(host, port);

    if (!conn.is_open()) {
        cerr << "Failed to connect to server\n";
        return 1;
    }

    // login
    Message login(TAG_SLOGIN, user);
    if (!conn.send(login)) {
        cerr << "Failed to send login message\n";
        return 1;
    }

    Message loginReply;
    if (!conn.receive(loginReply)) {
        cerr << loginReply.data;
        return 1;
    }

    if (loginReply.tag == TAG_ERR) {
        cerr << loginReply.data << "\n";
        return 1;
    }

    if (loginReply.tag != TAG_OK) {
        cerr << loginReply.tag << "\n";
        return 1;
    }

    // main loop
    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) break;

        if (line.empty()) continue;

        bool ok = true;
        Message outgoing = interpret(line, ok);
        if (!ok) continue;

        if (!conn.send(outgoing)) {
            cerr << "Failed to send message\n";
            continue;
        }

        Message serverResp;
        if (!conn.receive(serverResp)) {
            cerr << serverResp.data << "\n";
            continue;
        }

        if (serverResp.tag == TAG_ERR) {
            cerr << serverResp.data << "\n";
            continue;
        }

        if (outgoing.tag == TAG_QUIT && serverResp.tag == TAG_OK) {
            return 0;
        }
    }

    return 0;
}

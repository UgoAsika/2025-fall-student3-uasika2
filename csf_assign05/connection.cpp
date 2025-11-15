#include <iostream>
#include <string>
#include <cctype>
#include <cassert>

#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
    : m_fd(-1),
      m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
    : m_fd(fd),
      m_last_result(SUCCESS) {
    rio_readinitb(&m_fdbuf, m_fd);
}

void Connection::connect(const std::string &hostname, int port) {
    // convert port to string
    char port_str[6];
    std::snprintf(port_str, sizeof(port_str), "%d", port);

    // open socket
    int fd = open_clientfd(const_cast<char*>(hostname.c_str()), port_str);
    if (fd < 0) {
        std::cerr << "Could not connect" << std::endl;
        std::exit(1);
    }

    m_fd = fd;

    // initialize rio buffer
    rio_readinitb(&m_fdbuf, m_fd);
}

Connection::~Connection() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool Connection::is_open() const {
    return m_fd >= 0;
}

void Connection::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

/*
 * Send a message in protocol format:
 *     tag:data\n
 */
bool Connection::send(const Message &msg) {
    std::string line = msg.tag + ":" + msg.data + "\n";

    ssize_t written = rio_writen(m_fd, line.c_str(), line.size());
    if (written < 0 || static_cast<size_t>(written) != line.size()) {
        m_last_result = EOF_OR_ERROR;
        return false;
    }

    m_last_result = SUCCESS;
    return true;
}

/*
 * Receive a message:
 * read a line "tag:data"
 */
bool Connection::receive(Message &msg) {
    char buf[Message::MAX_LEN + 2];

    ssize_t n = rio_readlineb(&m_fdbuf, buf, sizeof(buf));
    if (n <= 0) {
        m_last_result = EOF_OR_ERROR;
        return false;
    }

    std::string line(buf);

    // strip newline + CR characters
    while (!line.empty() &&
           (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }

    // find first colon
    size_t pos = line.find(':');
    if (pos == std::string::npos) {
        m_last_result = EOF_OR_ERROR;
        return false;
    }

    msg.tag  = line.substr(0, pos);
    msg.data = line.substr(pos + 1);

    m_last_result = SUCCESS;
    return true;
}

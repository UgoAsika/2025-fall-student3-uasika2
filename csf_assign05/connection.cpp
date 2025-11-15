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
    char port_buf[6];
    std::snprintf(port_buf, sizeof(port_buf), "%d", port);

    m_fd = open_clientfd(const_cast<char*>(hostname.c_str()), port_buf);
    if (m_fd < 0) {
        std::cerr << "Could not connect" << std::endl;
        std::exit(1);
    }

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

bool Connection::send(const Message &msg) {
    std::string wire = msg.encode();
    size_t len = wire.size();

    ssize_t wrote = rio_writen(m_fd, wire.c_str(), len);
    if (wrote < 0 || static_cast<size_t>(wrote) != len) {
        m_last_result = EOF_OR_ERROR;
        return false;
    }

    m_last_result = SUCCESS;
    return true;
}

bool Connection::receive(Message &msg) {
    char buf[Message::MAX_LEN + 1];

    ssize_t nread = rio_readlineb(&m_fdbuf, buf, sizeof(buf));
    if (nread <= 0) {
        m_last_result = EOF_OR_ERROR;
        return false;
    }

    msg = Message::decode(buf);
    m_last_result = SUCCESS;
    return true;
}

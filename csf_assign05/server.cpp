#include <pthread.h>
#include <cctype>
#include <cassert>

#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Helper types
////////////////////////////////////////////////////////////////////////

// Little struct to pass stuff into each worker thread
struct WorkerArg {
    Server *server;
    int client_fd;
};

////////////////////////////////////////////////////////////////////////
// Client thread helpers
////////////////////////////////////////////////////////////////////////

namespace {

// Handles a sender client after slogin. Basically reads
// sender commands forever until they quit or something breaks.
void chat_with_sender(Server *server, Connection &conn, const std::string &username) {
    Room *current = nullptr;

    for (;;) {  // infinite loop until they quit or disconnect
        Message req;

        try {
            if (!conn.receive(req)) {
                break;  // if receive fails, they're basically gone
            }
        } catch (const std::exception &e) {
            // just tell the client something went wrong and keep going
            conn.send(Message(TAG_ERR, e.what()));
            continue;
        }

        if (req.tag == TAG_JOIN) {
            // join/create room
            current = server->find_or_create_room(req.data);
            conn.send(Message(TAG_OK, ""));

        } else if (req.tag == TAG_SENDALL) {
            if (!current) {
                conn.send(Message(TAG_ERR, "Not in a room"));
            } else {
                // broadcast msg to whoever’s chillin in the room
                current->broadcast_message(username, req.data);
                conn.send(Message(TAG_OK, ""));
            }

        } else if (req.tag == TAG_LEAVE) {
            if (!current) {
                conn.send(Message(TAG_ERR, "Not in a room"));
            } else {
                current = nullptr;
                conn.send(Message(TAG_OK, ""));
            }

        } else if (req.tag == TAG_QUIT) {
            // sender wants out — send ok and bail
            conn.send(Message(TAG_OK, ""));
            break;

        } else {
            // literally anything else is wrong
            conn.send(Message(TAG_ERR, "Invalid command"));
        }
    }
}

// Handles a receiver client after rlogin. They must immediately
// send a join, and then they just sit there and get messages forever.
void chat_with_receiver(Server *server, Connection &conn, User *user) {
    Message join_msg;

    try {
        // they HAVE to send join first — no join = no party
        if (!conn.receive(join_msg) || join_msg.tag != TAG_JOIN) {
            conn.send(Message(TAG_ERR, "Expected join"));
            delete user;
            return;
        }
    } catch (const std::exception &e) {
        conn.send(Message(TAG_ERR, e.what()));
        delete user;
        return;
    }

    Room *room = server->find_or_create_room(join_msg.data);
    room->add_member(user);
    conn.send(Message(TAG_OK, ""));

    // Now the receiver just waits for queued messages forever
    for (;;) {
        Message *delivery = user->mqueue.dequeue();

        if (!delivery) {
            // nothing ready yet, just keep looping
            continue;
        }

        // try sending the delivery
        if (!conn.send(*delivery)) {
            // sending failed → client gone
            room->remove_member(user);
            delete user;
            delete delivery;
            return;
        }

        delete delivery;  // done with it
    }
}

// The thread that handles each connected client
void *worker(void *arg) {
    pthread_detach(pthread_self());  // so we don’t have to join later

    WorkerArg *warg = static_cast<WorkerArg*>(arg);
    Server *server = warg->server;
    int fd = warg->client_fd;
    delete warg;

    Connection conn(fd);
    Message login_msg;

    try {
        // first message MUST be slogin or rlogin
        if (!conn.receive(login_msg)) {
            conn.send(Message(TAG_ERR, "Invalid login"));
            return nullptr;
        }
    } catch (const std::exception &e) {
        conn.send(Message(TAG_ERR, e.what()));
        return nullptr;
    }

    // figure out if they’re a sender or receiver
    if (login_msg.tag == TAG_SLOGIN) {
        std::string username = login_msg.data;
        conn.send(Message(TAG_OK, ""));
        chat_with_sender(server, conn, username);

    } else if (login_msg.tag == TAG_RLOGIN) {
        std::string username = login_msg.data;
        conn.send(Message(TAG_OK, ""));
        User *user = new User(username);
        chat_with_receiver(server, conn, user);

    } else {
        conn.send(Message(TAG_ERR, "Expected slogin or rlogin"));
    }

    return nullptr;
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////
// Server class functions
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
    : m_port(port), m_ssock(-1)
{
    // main server mutex for room map
    pthread_mutex_init(&m_lock, nullptr);
}

Server::~Server() {
    {
        // delete all rooms — otherwise huge leak
        Guard g(m_lock);
        for (auto &pair : m_rooms) {
            delete pair.second;
        }
        m_rooms.clear();
    }

    pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
    // open listening socket on the given port
    m_ssock = open_listenfd(std::to_string(m_port).c_str());
    return (m_ssock >= 0);
}

void Server::handle_client_requests() {
    // infinite accept loop. Didn’t want to use while(true) so this works too.
    for (;;) {
        sockaddr_storage client_addr;
        socklen_t len = sizeof(client_addr);

        int fd = Accept(m_ssock,
                        reinterpret_cast<sockaddr*>(&client_addr),
                        &len);

        auto *arg = new WorkerArg{ this, fd };
        pthread_t tid;
        Pthread_create(&tid, nullptr, worker, arg);
    }
}

Room *Server::find_or_create_room(const std::string &room_name) {
    Guard lock(m_lock);

    // check if it already exists
    auto it = m_rooms.find(room_name);
    if (it != m_rooms.end()) {
        return it->second;
    }

    // otherwise make a new room
    Room *new_room = new Room(room_name);
    m_rooms[room_name] = new_room;
    return new_room;
}

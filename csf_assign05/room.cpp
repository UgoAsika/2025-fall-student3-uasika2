#include "room.h"
#include "user.h"
#include "guard.h"
#include "message.h"

Room::Room(const std::string &nm)
    : room_name(nm)
{
    // initialize mutex for protecting member set
    pthread_mutex_init(&lock, nullptr);
}

Room::~Room() {
    // release mutex resources
    pthread_mutex_destroy(&lock);
}

void Room::add_member(User *u) {
    // add a User* into the membership set
    Guard acquire(lock);
    members.insert(u);
}

void Room::remove_member(User *u) {
    // erase a User* from the membership set
    Guard acquire(lock);
    members.erase(u);
}

void Room::broadcast_message(const std::string &sender, const std::string &text) {
    std::string combined = room_name;
    combined.append(":").append(sender).append(":").append(text);

    // iterate safely over receivers and enqueue messages
    Guard acquire(lock);
    for (User *usr : members) {
        // allocate a delivery packet for each receiver
        Message *m = new Message(TAG_DELIVERY, combined);
        usr->mqueue.enqueue(m);
    }
}

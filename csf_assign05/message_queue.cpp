#include <ctime>
#include <cassert>
#include "guard.h"
#include "message_queue.h"

MessageQueue::MessageQueue() {
    // initialize semaphore (0 initial count) and mutex
    int r1 = pthread_mutex_init(&m_lock, nullptr);
    int r2 = sem_init(&m_avail, 0, 0);
    assert(r1 == 0 && r2 == 0);
}

MessageQueue::~MessageQueue() {
    // cleanup synchronization primitives
    pthread_mutex_destroy(&m_lock);
    sem_destroy(&m_avail);
}

void MessageQueue::enqueue(Message *msg) {
    // critical section: modify queue
    {
        Guard lock_guard(m_lock);
        m_messages.push_back(msg);
    }

    // one more message available → wake waiting consumers
    sem_post(&m_avail);
}

Message *MessageQueue::dequeue() {
    // compute timeout = now + 1 second
    timespec timeout_spec{};
    clock_gettime(CLOCK_REALTIME, &timeout_spec);
    timeout_spec.tv_sec += 1;

    // wait up to 1 second for an available message
    if (sem_timedwait(&m_avail, &timeout_spec) != 0) {
        return nullptr;   // nothing ready
    }

    // remove next message (protected by mutex)
    Guard lock_guard(m_lock);
    if (m_messages.empty()) {
        return nullptr;
    }

    Message *next = m_messages.front();
    m_messages.pop_front();
    return next;
}


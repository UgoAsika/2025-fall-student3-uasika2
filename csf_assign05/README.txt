Ugo Asika (uasika2)

OverView:
This is a multi threaded chat server, it supports senders and receivers, rooms, messages queues etc.
A bit like slack but worse.

1. How the server works:
THe server has one main thread that sits there doing:
accept -> make a worker thread -> repeat forever
Each client connection gets its own pthread, and that thread handles 100% of that client's messages until the client quits or disconnects.
Workers are detached, so the server never waits for them.

2. Sender vs receivers
Sender:
After slogin, the sender can type things like:
/join roomname
hello guys
/leave
/quit
The server replies ok or err to every command so the sender knows what happened.
Broadcasting does not block the sender, it just pushes messages into queues and keeps going.

Receiver:
A receiver logs in with rlogin, then MUST send a /join ropm
After that, the receiver does nothing except sit there and wait to get messages from its own personal queue.
When something shows up, the receiver thread sends it to the client.
If the receiver disconnects, the server kicks it out of the room and deletes its User object.

3. Rooms and Users
A room has:
    a set of users inside it  
    Its own mutex
    a function that broadcasts messages to everyone in there

A user has:
    a username
    a messagequeue where senders drop messages for receivers

Rooms get created on demand. If you join a room that does not exist, it gets made.

4. synchronization (how i stopped everything from breaking)
This assignment was basically one big dont race condition yourself challenge, so this is what i locked:

A) Server mutex
    This protexts the global m_rooms map.
    Anytime i look up or create a room, I lock this.

B) Room mutex
    Every room has its own lock for:
        join, leave, broadcasts
    THis makes sure two senders dont mess with the set of members at the same time

C) Messagequeue lock + semaphore
    Each user's queue has:
        a mutex so enqueue/dequeue is safe
        a semaphore that tells receivers when messages exist

        Senders do: lock -> push message -> unlock -> sem_post
        Receivers do: sem_timedwait -> lock -> pop -> unlock -> send
        THis keeps receibers from busy waiting while still not freezing forever

5. Why this design actually works:
    Every client is handled by its own thread so no blocking other clients
    No shared data structure is ever touched without holding the right lock
    There are no nested locks, so no deadlocks
    Messagequeue + semaphores -> receivers block cleanly
    Deleting users/rooms is safe
    Has correct slogin/rlogin behaviour
    
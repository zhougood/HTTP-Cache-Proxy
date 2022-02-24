#ifndef __FIFO_H__
#define __FIFO_H__


#include <mutex>
#include <condition_variable>
#include <queue>

#include "socket.hpp"

class fifo {
private:
    std::queue<connSocket *> buffer;
    std::mutex m;
    std::condition_variable unempty;
    std::condition_variable unfull;
    size_t capacity;

public:
    fifo(int capacity): capacity(capacity) {}

    void produce(connSocket * socketfd) {
        std::unique_lock<std::mutex> lk(m);
        
        unfull.wait(lk, [this]() {
            return buffer.size() != capacity;
        });

        buffer.push(socketfd);

        lk.unlock();

        unempty.notify_one();

    }

    connSocket * consume() {
        std::unique_lock<std::mutex> lk(m);

        unempty.wait(lk, [this]() {
            return !buffer.empty();
        });

        connSocket * re = buffer.front();

        buffer.pop();

        lk.unlock();

        unfull.notify_one();

        return re;
    }

};

#endif
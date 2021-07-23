#ifndef BOA_UTL_DELETION_QUEUE_H
#define BOA_UTL_DELETION_QUEUE_H

#include <deque>
#include <functional>

namespace boa {

class DeletionQueue {
public:
    void enqueue(std::function<void()> &&func);
    void flush();

private:
    std::deque<std::function<void()>> m_deletors;
};

}

#endif

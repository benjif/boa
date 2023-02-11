#ifndef BOA_UTL_DELETION_QUEUE_H
#define BOA_UTL_DELETION_QUEUE_H

#include <deque>
#include <functional>
#include <cstdint>

namespace boa {

class DeletionQueue {
public:
    void enqueue(std::function<void()> &&func, uint32_t tags = 0);
    void flush();
    void flush_tags(uint32_t tags);

private:
    struct Deletor {
        Deletor(uint32_t t, std::function<void()> f)
            : tags(t),
              deletor(f)
        {
        }

        uint32_t tags;
        std::function<void()> deletor;
    };

    std::deque<Deletor> m_deletors;
};

}

#endif

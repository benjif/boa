#include "boa/utl/deletion_queue.h"

void boa::DeletionQueue::enqueue(std::function<void()> &&func, uint32_t tags) {
    m_deletors.emplace_back(tags, func);
}

void boa::DeletionQueue::flush() {
    for (auto &deletor : m_deletors)
        deletor.deletor();
    m_deletors.clear();
}

void boa::DeletionQueue::flush_tags(uint32_t tags) {
    for (auto it = m_deletors.begin(); it != m_deletors.end();) {
        if (it->tags & tags) {
            it->deletor();
            it = m_deletors.erase(it);
        } else {
            it++;
        }
    }
}

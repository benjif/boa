#include "boa/deletion_queue.h"

void boa::DeletionQueue::enqueue(std::function<void()> &&func) {
    m_deletors.push_back(func);
}

void boa::DeletionQueue::flush() {
    for (auto &deletor : m_deletors)
        deletor();
    m_deletors.clear();
}
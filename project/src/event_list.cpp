#include "event_list.h"

namespace nano_edr {
    
void EventList::PushBack(const Event& event) {
    EventNode* creating = new EventNode(event);
    
    if (capacity_ > 0 && size_ >= capacity_) {
        PopFront();
    }

    if (tail_) {
        tail_->next = creating;
    } else {
        head_ = creating; 
    }
    tail_ = creating;
    size_++;
}

void EventList::PopFront() {
    if (!size_) {
        return;
    }
    EventNode* deleted = head_;
    head_ = deleted->next;
    delete deleted;
    size_--;
    if (!size_) {
        tail_ = nullptr;
    }
}

EventList::~EventList() {
    Clear();
}

void EventList::Clear() {
    while (head_) {
        PopFront();
    }
}

}
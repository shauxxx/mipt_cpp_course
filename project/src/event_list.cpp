#include "../kit/include/l2.1/event_list.h"

namespace nano_edr {

EventList::~EventList() {
    Clear();
}

void EventList::PushBack(const Event& event) {
    if (capacity_ > 0 && size_ >= capacity_) {
        PopFront();
    }
    
    EventNode* new_node = new EventNode(event);
    new_node->next = nullptr;
    
    if (tail_) {
        tail_->next = new_node;
    } else {
        head_ = new_node;
    }
    
    tail_ = new_node;
    size_++;
}

void EventList::PopFront() {
    if (size_ == 0) {
        return;
    }
    
    EventNode* to_delete = head_;
    head_ = head_->next;
    delete to_delete;
    size_--;
    
    if (size_ == 0) {
        tail_ = nullptr;
    }
}

void EventList::Clear() {
    while (head_) {
        PopFront();
    }
}

}
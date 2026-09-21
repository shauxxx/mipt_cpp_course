#include "../kit/include/l1.2/event_list.h"

namespace nano_edr {

void ListPushBack(EventList* list, const Event* event) {
    if (list->capacity != 0 && list->size >= list->capacity) {
        ListPopFront(list);
    }

    EventNode* fresh = new EventNode;
    fresh->event = *event;
    fresh->next  = nullptr;

    if (list->tail == nullptr) {
        list->head = fresh;
    } else {
        list->tail->next = fresh;
    }
    list->tail = fresh;
    ++list->size;
}

void ListPopFront(EventList* list) {
    EventNode* victim = list->head;
    if (victim == nullptr) {
        return;
    }

    list->head = victim->next;
    delete victim;
    --list->size;

    if (list->size == 0) {
        list->tail = nullptr;
    }
}

void ListClear(EventList* list) {
    while (list->head != nullptr) {
        ListPopFront(list);
    }
}

EventList::~EventList() {
    ListClear(this);
}

}
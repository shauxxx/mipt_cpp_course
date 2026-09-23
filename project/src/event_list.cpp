#include "../kit/include/l1.2/event_list.h"

namespace nano_edr {

    void ListPushBack(EventList* list, const Event* event) {
        if (list->capacity > 0 && list->size >= list->capacity) {
            ListPopFront(list);
        }

        EventNode* creating = new EventNode;
        creating->event = *event;
        creating->next = nullptr;

        if (list->tail) {
            list->tail->next = creating;

        } else list->head = creating;
        list->tail = creating;
        list->size++; 
    }

    void ListPopFront(EventList* list) {
        if (!list->size) {
            return;
        }
        EventNode* deleted = list->head;
        list->head = deleted->next;
        delete deleted;
        list->size--;
        if (!list->size) {
            list->tail = nullptr;
        }

    }



    void ListClear(EventList* list) {
        while (list->head) {
            ListPopFront(list);
        }
    }

    EventList::~EventList() {
        ListClear(this);
    }
}
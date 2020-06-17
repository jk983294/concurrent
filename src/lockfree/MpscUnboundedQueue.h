#ifndef CONCURRENT_MPSC_UNBOUNDED_QUEUE_H
#define CONCURRENT_MPSC_UNBOUNDED_QUEUE_H

#include <atomic>

namespace frenzy {

template <typename T>
class MpscUnboundedQueue {
public:
    class Node {
    public:
        friend class MpscUnboundedQueue<T>;
        Node* volatile next;

    public:
        Node() : next(nullptr){};
    };

private:
    std::atomic<Node*> tail;
    Node stub;
    Node* head;

    void insert(Node* first, Node* last) {
        last->next = nullptr;
        Node* prev = tail.exchange(last, std::memory_order_relaxed);
        prev->next = first;
    }

public:
    MpscUnboundedQueue() : tail(&stub), head(&stub) {}

    // Push a single element
    void push(T* elem) { insert(elem, elem); }

    // Push multiple elements in a form of a linked list, linked by next
    void push(T* first, T* last) { insert(first, last); }

    // pop operates in chunk of elements and re-inserts stub after each chunk
    T* pop() {
        if (head == &stub) {                    // current chunk empty
            if (tail == &stub) return nullptr;  // add CAS for block here
            // wait for producer in insert()
            while (stub.next == nullptr) asm volatile("pause");
            head = stub.next;      // remove stub
            insert(&stub, &stub);  // re-insert stub at end
        }
        // wait for producer in insert()
        while (head->next == nullptr) asm volatile("pause");
        // retrieve and return first element
        Node* l = head;
        head = head->next;
        return (T*)l;
    }
};
}

#endif

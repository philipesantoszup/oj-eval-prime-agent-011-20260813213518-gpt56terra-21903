#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/**
 * A max priority queue implemented as a persistent leftist heap.
 *
 * Nodes are reference counted.  Meld never changes a node that already
 * belongs to a heap; it builds only the (logarithmic) right spine which it
 * needs.  Besides making copying inexpensive, this is useful here because a
 * throwing comparison leaves all existing roots completely untouched.
 */
template<typename T, class Compare = std::less<T> >
class priority_queue {
private:
    struct Node {
        T value;
        Node *left;
        Node *right;
        std::size_t dist;
        std::size_t refs;
        /* Used only after refs becomes zero, as an iterative delete stack. */
        Node *next;

        Node(const T &v, Node *l, Node *r)
            : value(v), left(l), right(r), dist(1), refs(1), next(NULL) {}
    };

    Node *root;
    std::size_t count;

    static std::size_t distance(Node *p) {
        return p == NULL ? 0 : p->dist;
    }

    static void retain(Node *p) {
        if (p != NULL) ++p->refs;
    }

    /*
     * Destroying can encounter a very long left spine.  It is deliberately
     * iterative, so clearing a large heap does not consume the call stack.
     */
    static void release(Node *p) {
        if (p == NULL || --p->refs != 0) return;
        p->next = NULL;
        Node *pending = p;
        while (pending != NULL) {
            Node *cur = pending;
            pending = pending->next;
            Node *l = cur->left;
            Node *r = cur->right;
            delete cur;
            if (l != NULL && --l->refs == 0) {
                l->next = pending;
                pending = l;
            }
            if (r != NULL && --r->refs == 0) {
                r->next = pending;
                pending = r;
            }
        }
    }

    static Node *new_node(const T &v, Node *l, Node *r) {
        if (distance(l) < distance(r)) {
            Node *tmp = l;
            l = r;
            r = tmp;
        }
        Node *p = new Node(v, l, r);
        retain(l);
        retain(r);
        p->dist = distance(r) + 1;
        return p;
    }

    static bool lower_priority(const Node *a, const Node *b) {
        try {
            return Compare()(a->value, b->value);
        } catch (...) {
            throw runtime_error();
        }
    }

    /* Return a new owning reference; neither input is changed. */
    static Node *meld(Node *a, Node *b) {
        if (a == NULL) {
            retain(b);
            return b;
        }
        if (b == NULL) {
            retain(a);
            return a;
        }
        if (lower_priority(a, b)) {
            Node *joined = meld(a, b->right);
            try {
                Node *ans = new_node(b->value, b->left, joined);
                release(joined);
                return ans;
            } catch (...) {
                release(joined);
                throw;
            }
        }
        Node *joined = meld(a->right, b);
        try {
            Node *ans = new_node(a->value, a->left, joined);
            release(joined);
            return ans;
        } catch (...) {
            release(joined);
            throw;
        }
    }

public:
    priority_queue() : root(NULL), count(0) {}

    priority_queue(const priority_queue &other) : root(other.root), count(other.count) {
        retain(root);
    }

    ~priority_queue() {
        release(root);
    }

    priority_queue &operator=(const priority_queue &other) {
        if (this != &other) {
            retain(other.root);
            release(root);
            root = other.root;
            count = other.count;
        }
        return *this;
    }

    const T &top() const {
        if (root == NULL) throw container_is_empty();
        return root->value;
    }

    void push(const T &e) {
        Node *single = new_node(e, NULL, NULL);
        Node *result = NULL;
        try {
            result = meld(root, single);
        } catch (...) {
            release(single);
            throw;
        }
        release(root);
        release(single);
        root = result;
        ++count;
    }

    void pop() {
        if (root == NULL) throw container_is_empty();
        /* Meld first: comparison failure must not remove the old root. */
        Node *result = meld(root->left, root->right);
        release(root);
        root = result;
        --count;
    }

    std::size_t size() const {
        return count;
    }

    bool empty() const {
        return root == NULL;
    }

    void merge(priority_queue &other) {
        if (this == &other) return;
        /* The old roots are retained throughout meld, so both queues have
           their original contents if Compare throws. */
        Node *result = meld(root, other.root);
        release(root);
        release(other.root);
        root = result;
        count += other.count;
        other.root = NULL;
        other.count = 0;
    }
};

}

#endif

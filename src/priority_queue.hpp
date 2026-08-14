#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/* A max leftist heap.  The right spine of a leftist heap is logarithmic, so
   meld (and therefore push and pop) is logarithmic as well. */
template<typename T, class Compare = std::less<T> >
class priority_queue {
private:
    struct Node {
        T value;
        Node *left, *right;
        std::size_t dist;
        /* Temporary link used only by the iterative destructor. */
        Node *next;
        Node(const T &v) : value(v), left(NULL), right(NULL), dist(1), next(NULL) {}
    };

    Node *root;
    std::size_t count;

    static std::size_t distance(const Node *p) { return p == NULL ? 0 : p->dist; }

    static void destroy(Node *p) {
        Node *todo = p;
        while (todo != NULL) {
            Node *cur = todo;
            todo = cur->next;
            if (cur->left != NULL) {
                cur->left->next = todo;
                todo = cur->left;
            }
            if (cur->right != NULL) {
                cur->right->next = todo;
                todo = cur->right;
            }
            delete cur;
        }
    }

    static Node *clone(const Node *p) {
        if (p == NULL) return NULL;
        Node *q = new Node(p->value);
        try {
            q->left = clone(p->left);
            q->right = clone(p->right);
            q->dist = p->dist;
        } catch (...) {
            destroy(q);
            throw;
        }
        return q;
    }

    static bool less(const Node *a, const Node *b) {
        try {
            return Compare()(a->value, b->value);
        } catch (...) {
            throw runtime_error();
        }
    }

    /*
     * No node is modified until the recursive call has returned.  Hence if a
     * comparison in that call throws, no frame has changed anything.  On the
     * successful return path all comparisons have already happened, so the
     * ordinary destructive leftist-heap linking is safe and allocation-free.
     */
    static Node *meld(Node *a, Node *b) {
        if (a == NULL) return b;
        if (b == NULL) return a;
        if (less(a, b)) {
            b->right = meld(a, b->right);
            if (distance(b->left) < distance(b->right)) {
                Node *t = b->left;
                b->left = b->right;
                b->right = t;
            }
            b->dist = distance(b->right) + 1;
            return b;
        }
        a->right = meld(a->right, b);
        if (distance(a->left) < distance(a->right)) {
            Node *t = a->left;
            a->left = a->right;
            a->right = t;
        }
        a->dist = distance(a->right) + 1;
        return a;
    }

public:
    priority_queue() : root(NULL), count(0) {}

    priority_queue(const priority_queue &other) : root(NULL), count(other.count) {
        root = clone(other.root);
    }

    ~priority_queue() { destroy(root); }

    priority_queue &operator=(const priority_queue &other) {
        if (this != &other) {
            Node *new_root = clone(other.root);
            destroy(root);
            root = new_root;
            count = other.count;
        }
        return *this;
    }

    const T &top() const {
        if (root == NULL) throw container_is_empty();
        return root->value;
    }

    void push(const T &e) {
        Node *p = new Node(e);
        try {
            root = meld(root, p);
        } catch (...) {
            delete p;
            throw;
        }
        ++count;
    }

    void pop() {
        if (root == NULL) throw container_is_empty();
        Node *old = root;
        Node *new_root = meld(old->left, old->right);
        root = new_root;
        delete old;
        --count;
    }

    std::size_t size() const { return count; }
    bool empty() const { return root == NULL; }

    void merge(priority_queue &other) {
        if (this == &other) return;
        Node *new_root = meld(root, other.root);
        root = new_root;
        count += other.count;
        other.root = NULL;
        other.count = 0;
    }
};

}

#endif

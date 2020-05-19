#include <iterator> // std::iterator
#include <utility>  // std::pair
#include <cassert>  // ::assert
#include <vector>   // std::vector
#include <random>   // std::mt19937

namespace skiplist {

template<class Key, class T>
class skiplist {
    private:
        template<class Ptr, class Ref>
        class Iterator;

        class Node;

    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<const Key, T>;
        using size_type = size_t;
        using iterator = Iterator<value_type*, value_type&>;
        using const_iterator = Iterator<const value_type*, const value_type&>;
        using skiplist_type = skiplist<Key, T>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        friend class Iterator<value_type*, value_type&>;
        friend class Iterator<const value_type*, const value_type&>;

        explicit skiplist();
        ~skiplist();

        // noncopyable
        skiplist(const skiplist&) = delete;
        skiplist& operator=(const skiplist&) = delete;

        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

        reverse_iterator rbegin();
        reverse_iterator rend();
        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;

        std::pair<skiplist::iterator, bool> insert(const value_type &value);
        mapped_type & operator[](const key_type &key);

        // erase removes specified element from skiplist
        void erase(const key_type &key);
        // erase removes specified element from skiplist and returns iterator following the removed element
        iterator erase(iterator it);

        // find finds element with specific key and returns an iterator to the element
        iterator find(const key_type &key);
        const_iterator find(const key_type &key) const;

        // lower_bound returns an iterator to the first element not less than the given key
        iterator lower_bound(const key_type &key);
        const_iterator lower_bound(const key_type &key) const;

        // upper_bound returns an iterator to the first element greater than the given key
        iterator upper_bound(const key_type &key);
        const_iterator upper_bound(const key_type &key) const;

        // emplace inserts a new element into the skiplist constructed in-place with the given args
        // if there is no element with the key in the container.
        template<class ...Args>
        std::pair<skiplist::iterator, bool> emplace(Args&&... args);

        // size returns the number of elements
        size_type size() const;

        // clear removes all elements from skiplist
        void clear();

        // empty checks whether the skiplist is empty
        bool empty() const;

    private:
        enum { kMaxHeight = 20 };

        // find_greater_or_equal finds the first node which is greater or equal than key
        // and previous nodes before the return node at every level.
        Node *find_greater_or_equal(const key_type &key, std::vector<Node *> &forwards) const;

        // random_height generates a height for skiplist node randomly.
        size_t random_height();

        template<class ...Args>
        std::pair<skiplist::iterator, bool> emplace_impl(Args&&... args);
 
    private:
        // head_ is a begin sentinel of skiplist.
        Node *head_;
        // tail_ is a end sentinel of skiplist.
        Node *tail_;
        // size_ indicates the current size of skiplist.
        size_type size_;
        // max_height_ indicates the max height of all nodes of skiplist, except head_ node and tail_ node.
        // TODO(optimize): ajust max_height_ after inserting or erasing element
        size_t const max_height_;
        // rnd_ uses to generate height
        std::mt19937 rnd_;
};

template<class Key, class T>
class skiplist<Key, T>::Node {
    public:
        template<class ...Args>
        Node(size_t height, Args&& ...args) 
            : value_(std::forward<Args>(args)...),
            height_(height),
            forwards_(height, nullptr),
            backward_(nullptr) {
        }

        Node *forward(size_t height) const {
            assert(height < forwards_.size());
            return forwards_[height];
        }

        void set_forward(size_t height, Node *p) {
            assert(height < forwards_.size());
            forwards_[height] = p;
        }

        Node *backward() const {
            return backward_;
        }

        void set_backward(skiplist::Node *p) {
            backward_ = p;
        }

        // next returns the next node after this node
        Node *next() const {
            return forwards_[0];
        }

        // height returns the node's height
        size_t height() const {
            return height_;
        }

        const key_type &key() const {
            return value_.first;
        }

        // value_ stores the data content attached this node.
        value_type value_;
        // height_ indicates node's height. it is immutable after construction.
        const size_t height_;
        // forwards_[i] points to the next node after this node at i-th level.
        std::vector<Node *> forwards_;
        // backward_ points to the previous node.
        Node *backward_;
};

template<class Key, class T>
template<class Ptr, class Ref>
class skiplist<Key, T>::Iterator: public std::iterator<std::bidirectional_iterator_tag, skiplist::value_type> {
    private:
        using iterator_type = Iterator<Ptr, Ref>;

        explicit Iterator(const skiplist_type *sl, skiplist::Node *node) 
            : skiplist_(sl),
            node_(node) {
            }

    public:
        Iterator() 
            : skiplist_(nullptr),
            node_(nullptr) {
        }

        ~Iterator() = default;
        Iterator(const Iterator &it) = default;
        Iterator &operator=(const Iterator &it) = default;

        Ptr operator ->() {
            return &(node_->value_);
        }

        Ref operator*() {
            return *operator->();
        }

        iterator_type &operator++() {
            assert(node_ != nullptr);
            assert(node_ != skiplist_->tail_);
            node_ = node_->next();
            return *this;
        }
        iterator_type operator++(int) {
            assert(node_ != nullptr);
            assert(node_ != skiplist_->tail_);
            skiplist::Node *tmp = node_;
            operator++();
            return iterator_type{skiplist_, tmp};
        }

        iterator_type &operator--() {
            assert(node_ != nullptr);
            assert(node_->backward() != skiplist_->head_);
            node_ = node_->backward();
            return *this;
        }
        iterator_type operator--(int) {
            assert(node_ != nullptr);
            assert(node_->backward() != skiplist_->head_);
            skiplist::Node *tmp = node_;
            operator--();
            return iterator_type{skiplist_, tmp};
        }

        bool operator==(const iterator_type &it) const {
            assert(skiplist_ == it.skiplist_);
            return node_ == it.node_;
        }
        bool operator!=(const iterator_type &it) const {
            assert(skiplist_ == it.skiplist_);
            return !operator==(it);
        }

    private:
        friend class skiplist;

    private:
        const skiplist_type *skiplist_;
        Node *node_;
};

template<class Key, class T>
skiplist<Key, T>::skiplist() 
    : head_(new Node(kMaxHeight, value_type())),
    tail_(new Node(kMaxHeight, value_type())),
    size_(0),
    max_height_(kMaxHeight),
    rnd_(0xdeadbeef) {
        assert(head_ != nullptr);
        assert(tail_ != nullptr);

        for (int i = 0; i < kMaxHeight; ++i) {
            head_->set_forward(i, tail_);
            tail_->set_forward(i, nullptr);
        }
        head_->set_backward(nullptr);
        tail_->set_backward(head_);
}

template<class Key, class T>
skiplist<Key, T>::~skiplist() {

    // clears all node
    clear();

    // destroy head_ and tail_
    delete head_;
    delete tail_;
}

template<class Key, class T>
typename skiplist<Key, T>::iterator skiplist<Key, T>::begin() {
    return iterator(this, head_->next());
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::begin() const {
    return const_iterator(this, head_->next());
}

template<class Key, class T>
typename skiplist<Key, T>::reverse_iterator skiplist<Key, T>::rbegin() {
    return reverse_iterator(end());
}

template<class Key, class T>
typename skiplist<Key, T>::const_reverse_iterator skiplist<Key, T>::rbegin() const {
    return const_reverse_iterator(end());
}

template<class Key, class T>
typename skiplist<Key, T>::iterator skiplist<Key, T>::end() {
    return iterator(this, tail_);
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::end() const {
    return const_iterator(this, tail_);
}

template<class Key, class T>
typename skiplist<Key, T>::reverse_iterator skiplist<Key, T>::rend() {
    return reverse_iterator(begin());
}

template<class Key, class T>
typename skiplist<Key, T>::const_reverse_iterator skiplist<Key, T>::rend() const {
    return const_reverse_iterator(begin());
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::cbegin() const {
    return begin();
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::cend() const {
    return end();
}

template<class Key, class T>
std::pair<typename skiplist<Key, T>::iterator, bool> skiplist<Key, T>::insert(const value_type &value) {
    std::vector<Node *> prevs;
    Node *node = find_greater_or_equal(value.first, prevs);

    // we have the same key already
    if (node != tail_ && node->key() == value.first) {
        return {iterator(this, node), false};
    }

    const size_t height = random_height();
    node = new Node(height, value);
    assert(node != nullptr);

    node->set_backward(prevs[0]);
    prevs[0]->next()->set_backward(node);

    for (int i = 0; i < height; ++i) {
        node->set_forward(i, prevs[i]->forward(i));
        prevs[i]->set_forward(i, node);
    }

    ++size_;

    return {iterator{this, node}, true};
}

template<class Key, class T>
typename skiplist<Key, T>::mapped_type &skiplist<Key, T>::operator[](const key_type &key) {
    auto it = insert(value_type{key, mapped_type()});
    return it.first->second;
}

template<class Key, class T>
void skiplist<Key, T>::erase(const key_type &key) {
    std::vector<Node *> prevs;
    Node *p = find_greater_or_equal(key, prevs);

    // not found the key
    if (p == tail_ || p->key() > key) {
        return;
    }

    Node *next = p->next();
    next->set_backward(p->backward());
    for (int i = 0; i < p->height(); i++) {
        prevs[i]->set_forward(i, p->forward(i));
    }

    --size_;

    // release deleted node memory
    delete p;
}

template<class Key, class T>
typename skiplist<Key, T>::iterator skiplist<Key, T>::erase(iterator it) {
    assert(it != end());
    assert(it.skiplist_ == this);
    Node *node = it->node_->next();
    erase(it->first);
    return iterator{this, node};
}

template<class Key, class T>
typename skiplist<Key, T>::iterator skiplist<Key, T>::find(const key_type &key) {
    iterator it = lower_bound(key);
    if (it != end() && it->first == key) {
        return it;
    }

    return end();
}

template<class Key, class T>
typename skiplist<Key, T>::iterator skiplist<Key, T>::lower_bound(const key_type &key) {
    std::vector<Node *> prevs;
    Node *p = find_greater_or_equal(key, prevs);
    return iterator(this, p);
}

template<class Key, class T>
typename skiplist<Key, T>::iterator skiplist<Key, T>::upper_bound(const key_type &key) {
    iterator it = lower_bound(key);
    // skip the equal key
    if (it != end() && it->first == key) {
        ++it;
    }

    return it;
}

template<class Key, class T>
template<class ...Args>
std::pair<typename skiplist<Key, T>::iterator, bool> skiplist<Key, T>::emplace_impl(Args&&... args) {

    // create a new node with args
    const size_t height = random_height();
    Node *node = new Node(height, std::forward<Args>(args)...);
    assert(node != nullptr);

    std::vector<Node *> prevs;
    Node *p = find_greater_or_equal(node->key(), prevs);
    // have the same key already
    if (p != tail_ && p->key() == node->key()) {
        // release new node
        delete node;
        return {iterator(this, p), false};
    }

    node->set_backward(prevs[0]);
    prevs[0]->next()->set_backward(node);

    for (int i = 0; i < height; ++i) {
        node->set_forward(i, prevs[i]->forward(i));
        prevs[i]->set_forward(i, node);
    }

    ++size_;

    return {iterator{this, node}, true};
}

template<class Key, class T>
template<class ...Args>
std::pair<typename skiplist<Key, T>::iterator, bool> skiplist<Key, T>::emplace(Args&&... args) {
    return emplace_impl(std::forward<Args>(args)...);
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::find(const key_type &key) const {
    const_iterator it = lower_bound(key);
    if (it != end() && it->first == key) {
        return it;
    }

    return end();
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::lower_bound(const key_type &key) const {
    std::vector<Node *> prevs;
    Node *p = find_greater_or_equal(key, prevs);
    return const_iterator(this, p);
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::upper_bound(const key_type &key) const {
    const_iterator it = lower_bound(key);
    // skip the equal key
    if (it != end() && it->first == key) {
        ++it;
    }

    return it;
}


template<class Key, class T>
typename skiplist<Key, T>::size_type skiplist<Key, T>::size() const {
    return size_;
}

template<class Key, class T>
void skiplist<Key, T>::clear() {
    Node *node = head_->next();
    while (node != tail_) {
        Node *p = node->next();
        delete node;
        node = p;
    }

    for (int i = 0; i < kMaxHeight; i++) {
        head_->set_forward(i, tail_);
    }
    tail_->set_backward(head_);

    size_ = 0;
}

template<class Key, class T>
bool skiplist<Key, T>::empty() const {
    return size_ == 0;
}

template<class Key, class T>
typename skiplist<Key, T>::Node *skiplist<Key, T>::find_greater_or_equal(const key_type &key, std::vector<Node *> &prevs) const {
    prevs.resize(max_height_);
    Node *p = head_;
    size_t cur_height = max_height_ - 1;

    // go from highest level to lowest level
    for (int i = max_height_ - 1; i >= 0; i--) {
        // go forward until encouter tail_ or node whose key is greater or equal than key
        while (p->forward(i) != tail_ && p->forward(i)->key() < key) {
            p = p->forward(i);
        }

        prevs[i] = p;
    }

    return p->next();
}

template<class Key, class T>
size_t skiplist<Key, T>::random_height() {
    static const size_t kBranching = 4;
    size_t height = 1;
    while (height < kMaxHeight && ((size_t)rnd_() % kBranching == 0)) {
        height++;
    }

    assert(height > 0);
    assert(height < kMaxHeight);

    return height;
}

}

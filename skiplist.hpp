#include <iterator>
#include <algorithm>
#include <utility>
#include <cassert>
#include <vector>
#include <random>

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
        reverse_iterator rbegin();
        reverse_iterator rend();

        std::pair<skiplist::iterator, bool> insert(const value_type &value);
        mapped_type & operator[](const key_type &key);
        void erase(const key_type &key);
        void erase(iterator it);
        iterator find(const key_type &key);
        iterator lower_bound(const key_type &key);
        iterator upper_bound(const key_type &key);

        // access functions for immutable skiplist
        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;
        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;

        const_iterator find(const key_type &key) const;
        const_iterator lower_bound(const key_type &key) const;
        const_iterator upper_bound(const key_type &key) const;

        size_type size() const;

    private:
        enum { kMaxHeight = 20 };

        Node *find_greater_or_equal(const key_type &key, std::vector<Node *> &forwards) const;

        // random_height generate a height for skiplist node randomly.
        size_t random_height();

    private:
        Node *head_;
        Node *tail_;
        size_type size_;
        size_t const max_height_;
        // rnd_ use to generate height
        std::mt19937 rnd_;
};

template<class Key, class T>
class skiplist<Key, T>::Node {
    public:
        Node(const value_type &value, size_t height) 
            : value_(value),
            height_(height),
            forwards_(height, nullptr),
            backward_(nullptr) {
        }

        Node *forward(size_t height) const {
            assert(height < forwards_.size());
            return forwards_[height];
        }
        void set_forward(size_t height, skiplist::Node *p) {
            assert(height < forwards_.size());
            forwards_[height] = p;
        }

        Node *backward() const {
            return backward_;
        }
        void set_backward(skiplist::Node *p) {
            assert(p != nullptr);
            backward_ = p;
        }

        size_t height() const {
            return height_;
        }

        const key_type &key() const {
            return value_.first;
        }

        value_type value_;
        const size_t height_;
        std::vector<Node *> forwards_;
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
            node_ = node_->forward(0);
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
    : head_(new Node(value_type(), kMaxHeight)),
    tail_(new Node(value_type(), kMaxHeight)),
    size_(0),
    max_height_(kMaxHeight),
    rnd_(0xdeadbeef) {
        assert(head_ != nullptr);
        assert(tail_ != nullptr);
        for (int i = 0; i < kMaxHeight; ++i) {
            head_->set_forward(i, tail_);
            tail_->set_forward(i, nullptr);
        }
        //head_->set_backward(nullptr);
        tail_->set_backward(head_);
    }

template<class Key, class T>
skiplist<Key, T>::~skiplist() {
    // release all nodes
    Node *p = head_;
    while (p != nullptr) {
        Node *next = p->forward(0);
        delete p;
        p = next;
    }
}

template<class Key, class T>
typename skiplist<Key, T>::iterator skiplist<Key, T>::begin() {
    return iterator(this, head_->forward(0));
}

template<class Key, class T>
typename skiplist<Key, T>::const_iterator skiplist<Key, T>::begin() const {
    return const_iterator(this, head_->forward(0));
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
    skiplist::Node *p = find_greater_or_equal(value.first, prevs);

    // we have the same key already
    if (p != tail_ && p->key() == value.first) {
        return {iterator(this, p), false};
    }

    size_t height = random_height();
    skiplist::Node *node = new skiplist::Node(value, height);
    assert(node != nullptr);

    node->set_backward(prevs[0]);
    prevs[0]->forward(0)->set_backward(node);

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

    Node *next = p->forward(0);
    next->set_backward(p->backward());
    for (int i = 0; i < p->height(); i++) {
        prevs[i]->set_forward(i, p->forward(i));
    }

    --size_;

    delete p;
}

template<class Key, class T>
void skiplist<Key, T>::erase(iterator it) {
    assert(it != end());
    assert(it.skiplist_ == this);
    return erase(it->first);
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
typename skiplist<Key, T>::Node *skiplist<Key, T>::find_greater_or_equal(const key_type &key, std::vector<Node *> &prevs) const {
    prevs.resize(max_height_);
    Node *p = head_;
    size_t cur_height = max_height_ - 1;

    while (true) {
        Node *next = p->forward(cur_height);
        if (next == tail_ || next->key() >= key) {
            prevs[cur_height] = p;

            // already at the lowest level
            if (cur_height == 0) {
                break;
            }

            // go down
            cur_height--;
        } else {
            // move forward
            p = next;
        }
    }

    return p->forward(0);
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

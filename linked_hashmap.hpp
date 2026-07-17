/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */
    
template<
    class Key,
    class T,
    class Hash = std::hash<Key>, 
    class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
    typedef pair<const Key, T> value_type;
    
private:
    struct Node {
        value_type *data;
        Node *prev;  // prev in linked list
        Node *next;  // next in linked list
        Node *hash_next;  // next in hash bucket
        
        Node() : data(nullptr), prev(nullptr), next(nullptr), hash_next(nullptr) {}
        Node(const Key &k, const T &v) : prev(nullptr), next(nullptr), hash_next(nullptr) {
            data = new value_type(k, v);
        }
        ~Node() {
            if (data) {
                delete data;
                data = nullptr;
            }
        }
    };
    
    Node *head;  // dummy head of linked list
    Node *tail;  // dummy tail of linked list
    Node **buckets;  // hash table buckets
    size_t bucket_count;
    size_t num_elements;
    double max_load_factor;
    
    Hash hash_func;
    Equal equal_func;
    
    void init() {
        head = new Node();
        tail = new Node();
        head->next = tail;
        tail->prev = head;
        bucket_count = 16;
        buckets = new Node*[bucket_count]();
        num_elements = 0;
        max_load_factor = 0.75;
    }
    
    void clear_buckets() {
        if (buckets) {
            for (size_t i = 0; i < bucket_count; ++i) {
                Node *node = buckets[i];
                while (node) {
                    Node *next = node->hash_next;
                    delete node;
                    node = next;
                }
            }
            delete[] buckets;
            buckets = nullptr;
        }
    }
    
    size_t get_bucket_index(const Key &key) const {
        return hash_func(key) % bucket_count;
    }
    
    Node* find_in_bucket(size_t idx, const Key &key) const {
        Node *node = buckets[idx];
        while (node) {
            if (equal_func(node->data->first, key)) {
                return node;
            }
            node = node->hash_next;
        }
        return nullptr;
    }
    
    void remove_from_bucket(Node *node, size_t idx) {
        if (buckets[idx] == node) {
            buckets[idx] = node->hash_next;
        } else {
            Node *prev = buckets[idx];
            while (prev && prev->hash_next != node) {
                prev = prev->hash_next;
            }
            if (prev) {
                prev->hash_next = node->hash_next;
            }
        }
        node->hash_next = nullptr;
    }
    
    void add_to_bucket(Node *node, size_t idx) {
        node->hash_next = buckets[idx];
        buckets[idx] = node;
    }
    
    void remove_from_list(Node *node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    
    void add_to_list_tail(Node *node) {
        node->prev = tail->prev;
        node->next = tail;
        tail->prev->next = node;
        tail->prev = node;
    }
    
    void rehash(size_t new_bucket_count) {
        Node **old_buckets = buckets;
        size_t old_bucket_count = bucket_count;
        
        bucket_count = new_bucket_count;
        buckets = new Node*[bucket_count]();
        
        for (size_t i = 0; i < old_bucket_count; ++i) {
            Node *node = old_buckets[i];
            while (node) {
                Node *next = node->hash_next;
                size_t new_idx = hash_func(node->data->first) % bucket_count;
                node->hash_next = buckets[new_idx];
                buckets[new_idx] = node;
                node = next;
            }
        }
        
        delete[] old_buckets;
    }
    
    void copy_from(const linked_hashmap &other) {
        init();
        hash_func = other.hash_func;
        equal_func = other.equal_func;
        for (Node *node = other.head->next; node != other.tail; node = node->next) {
            insert(*node->data);
        }
    }

public:
    class const_iterator;
    class iterator {
    private:
        Node *node;
        const linked_hashmap *map;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator() : node(nullptr), map(nullptr) {}
        iterator(Node *n, const linked_hashmap *m) : node(n), map(m) {}
        iterator(const iterator &other) : node(other.node), map(other.map) {}
        
        iterator operator++(int) {
            if (node == nullptr || node == map->tail) {
                throw invalid_iterator();
            }
            iterator tmp(*this);
            node = node->next;
            return tmp;
        }
        
        iterator & operator++() {
            if (node == nullptr || node == map->tail) {
                throw invalid_iterator();
            }
            node = node->next;
            return *this;
        }
        
        iterator operator--(int) {
            if (node == nullptr || node == map->head->next || node == map->head) {
                throw invalid_iterator();
            }
            iterator tmp(*this);
            node = node->prev;
            return tmp;
        }
        
        iterator & operator--() {
            if (node == nullptr || node == map->head->next || node == map->head) {
                throw invalid_iterator();
            }
            node = node->prev;
            return *this;
        }
        
        value_type & operator*() const {
            return *(node->data);
        }
        
        bool operator==(const iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator!=(const iterator &rhs) const {
            return node != rhs.node;
        }
        
        bool operator!=(const const_iterator &rhs) const {
            return node != rhs.node;
        }

        value_type* operator->() const noexcept {
            return node->data;
        }
        
        friend class const_iterator;
        friend class linked_hashmap;
    };
 
    class const_iterator {
    private:
        Node *node;
        const linked_hashmap *map;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = const typename linked_hashmap::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;

        const_iterator() : node(nullptr), map(nullptr) {}
        const_iterator(Node *n, const linked_hashmap *m) : node(n), map(m) {}
        const_iterator(const const_iterator &other) : node(other.node), map(other.map) {}
        const_iterator(const iterator &other) : node(other.node), map(other.map) {}
        
        const_iterator operator++(int) {
            if (node == nullptr || node == map->tail) {
                throw invalid_iterator();
            }
            const_iterator tmp(*this);
            node = node->next;
            return tmp;
        }
        
        const_iterator & operator++() {
            if (node == nullptr || node == map->tail) {
                throw invalid_iterator();
            }
            node = node->next;
            return *this;
        }
        
        const_iterator operator--(int) {
            if (node == nullptr || node == map->head->next || node == map->head) {
                throw invalid_iterator();
            }
            const_iterator tmp(*this);
            node = node->prev;
            return tmp;
        }
        
        const_iterator & operator--() {
            if (node == nullptr || node == map->head->next || node == map->head) {
                throw invalid_iterator();
            }
            node = node->prev;
            return *this;
        }
        
        const value_type & operator*() const {
            return *(node->data);
        }
        
        bool operator==(const iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator==(const const_iterator &rhs) const {
            return node == rhs.node;
        }
        
        bool operator!=(const iterator &rhs) const {
            return node != rhs.node;
        }
        
        bool operator!=(const const_iterator &rhs) const {
            return node != rhs.node;
        }

        const value_type* operator->() const noexcept {
            return node->data;
        }
        
        friend class iterator;
        friend class linked_hashmap;
    };
 
    linked_hashmap() {
        init();
    }
    
    linked_hashmap(const linked_hashmap &other) {
        init();
        *this = other;
    }
 
    linked_hashmap & operator=(const linked_hashmap &other) {
        if (this == &other) {
            return *this;
        }
        clear();
        delete head;
        delete tail;
        delete[] buckets;
        
        init();
        hash_func = other.hash_func;
        equal_func = other.equal_func;
        for (Node *node = other.head->next; node != other.tail; node = node->next) {
            Node *new_node = new Node(node->data->first, node->data->second);
            add_to_list_tail(new_node);
            size_t idx = get_bucket_index(node->data->first);
            add_to_bucket(new_node, idx);
            num_elements++;
        }
        return *this;
    }
 
    ~linked_hashmap() {
        clear();
        if (head) delete head;
        if (tail) delete tail;
        if (buckets) delete[] buckets;
    }
 
    T & at(const Key &key) {
        size_t idx = get_bucket_index(key);
        Node *node = find_in_bucket(idx, key);
        if (node == nullptr) {
            throw index_out_of_bound();
        }
        return node->data->second;
    }
    
    const T & at(const Key &key) const {
        size_t idx = get_bucket_index(key);
        Node *node = find_in_bucket(idx, key);
        if (node == nullptr) {
            throw index_out_of_bound();
        }
        return node->data->second;
    }
 
    T & operator[](const Key &key) {
        size_t idx = get_bucket_index(key);
        Node *node = find_in_bucket(idx, key);
        if (node == nullptr) {
            if (num_elements + 1 > bucket_count * max_load_factor) {
                rehash(bucket_count * 2);
                idx = get_bucket_index(key);
            }
            node = new Node(key, T());
            add_to_list_tail(node);
            add_to_bucket(node, idx);
            num_elements++;
        }
        return node->data->second;
    }
 
    const T & operator[](const Key &key) const {
        return at(key);
    }
 
    iterator begin() {
        return iterator(head->next, this);
    }
    
    const_iterator cbegin() const {
        return const_iterator(head->next, this);
    }
 
    iterator end() {
        return iterator(tail, this);
    }
    
    const_iterator cend() const {
        return const_iterator(tail, this);
    }
 
    bool empty() const {
        return num_elements == 0;
    }
 
    size_t size() const {
        return num_elements;
    }
 
    void clear() {
        Node *node = head->next;
        while (node != tail) {
            Node *next = node->next;
            delete node;
            node = next;
        }
        for (size_t i = 0; i < bucket_count; ++i) {
            buckets[i] = nullptr;
        }
        head->next = tail;
        tail->prev = head;
        num_elements = 0;
    }
 
    pair<iterator, bool> insert(const value_type &value) {
        size_t idx = get_bucket_index(value.first);
        Node *existing = find_in_bucket(idx, value.first);
        if (existing != nullptr) {
            return pair<iterator, bool>(iterator(existing, this), false);
        }
        
        if (num_elements + 1 > bucket_count * max_load_factor) {
            rehash(bucket_count * 2);
            idx = get_bucket_index(value.first);
        }
        
        Node *node = new Node(value.first, value.second);
        add_to_list_tail(node);
        add_to_bucket(node, idx);
        num_elements++;
        
        return pair<iterator, bool>(iterator(node, this), true);
    }
 
    void erase(iterator pos) {
        if (pos == end()) {
            throw invalid_iterator();
        }
        if (pos.node == nullptr || pos.node == head || pos.node == tail) {
            throw invalid_iterator();
        }
        
        Node *node = pos.node;
        size_t idx = get_bucket_index(node->data->first);
        remove_from_bucket(node, idx);
        remove_from_list(node);
        delete node;
        num_elements--;
    }
 
    size_t count(const Key &key) const {
        size_t idx = get_bucket_index(key);
        Node *node = find_in_bucket(idx, key);
        return node != nullptr ? 1 : 0;
    }
 
    iterator find(const Key &key) {
        size_t idx = get_bucket_index(key);
        Node *node = find_in_bucket(idx, key);
        if (node == nullptr) {
            return end();
        }
        return iterator(node, this);
    }
    
    const_iterator find(const Key &key) const {
        size_t idx = get_bucket_index(key);
        Node *node = find_in_bucket(idx, key);
        if (node == nullptr) {
            return cend();
        }
        return const_iterator(node, this);
    }
};

}

#endif

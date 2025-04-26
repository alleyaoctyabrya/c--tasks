#include <algorithm>
#include <concepts>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

template <typename Key, typename Value, typename Hash, typename EqualTo,
          typename Alloc>
class UnorderedMap;

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename EqualTo = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 private:
  [[no_unique_address]] Hash hasher;
  using T = std::pair<const Key, Value>;
  [[no_unique_address]] EqualTo key_equal;
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;

    BaseNode() : prev(this), next(this) {}

    BaseNode(const BaseNode& node) = default;

    BaseNode(BaseNode* first, BaseNode* second) : prev(first), next(second) {}

    BaseNode& operator=(const BaseNode& node) = default;
  };
  struct Node : public BaseNode {
    std::pair<const Key, Value> value;
    Node() = default;

    template <typename... Args>
    Node(Args&&... args) : value(std::forward<Args>(args)...) {}
    Node(const Node& node) = default;
    Node(Node&& node) = default;

    Node& operator=(const Node& node) = default;
    Node& operator=(Node&& node) = default;
  };
  class List {
   private:
    template <typename K, typename V, typename Hash_, typename EqualTo_,
              typename Alloc>
    friend class UnorderedMap;
    using NodeAlloc =
        typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeTraits = std::allocator_traits<NodeAlloc>;
    [[no_unique_address]] NodeAlloc alloc;

   public:
    template <bool is_const>
    struct common_iterator {
      using iterator_category = std::bidirectional_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using kind_of_type =
          typename std::conditional<is_const, const BaseNode, BaseNode>::type;
      using kind_of_type_2_attempt =
          typename std::conditional<is_const, const Node, Node>::type;
      using value_type = typename std::conditional<is_const, const T, T>::type;
      using pointer = value_type*;
      using reference = value_type&;
      kind_of_type* cur_node_ = nullptr;

      common_iterator(kind_of_type* ptr) : cur_node_(ptr) {}

      common_iterator& operator=(const common_iterator& iter) {
        if (this == &iter) {
          return *this;
        }
        cur_node_ = iter.cur_node_;
        return *this;
      }

      ~common_iterator() = default;

      reference operator*() const noexcept {
        return reinterpret_cast<kind_of_type_2_attempt*>(cur_node_)->value;
      }

      pointer operator->() const noexcept { return &(operator*()); }

      common_iterator& operator++() noexcept {
        cur_node_ = cur_node_->next;
        return *this;
      }

      common_iterator operator++(int) noexcept {
        common_iterator temp = *this;
        ++(*this);
        return temp;
      }

      common_iterator& operator--() noexcept {
        cur_node_ = cur_node_->prev;
        return *this;
      }

      common_iterator operator--(int) noexcept {
        common_iterator temp = *this;
        --(*this);
        return temp;
      }
      common_iterator& operator-(int value) noexcept {
        while (value-- > 0) {
          operator--();
        }
        return *this;
      }

      common_iterator& operator+(int value) noexcept {
        while (value-- > 0) {
          operator++();
        }
        return *this;
      }

      bool operator==(const common_iterator& iter) const {
        return (cur_node_ == iter.cur_node_);
      }

      bool operator!=(const common_iterator& iter) const {
        return !operator==(iter);
      }

      operator common_iterator<true>() const {
        return common_iterator<true>(static_cast<Node*>(cur_node_));
      }
    };

    using const_iterator = common_iterator<true>;
    using iterator = common_iterator<false>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    BaseNode fakenode_;
    uint_fast64_t sz_;

    List() : fakenode_(&fakenode_, &fakenode_), sz_(0) {}

    List(uint_fast64_t count) : List() {
      try {
        for (uint_fast64_t i = 0; i < count; ++i) {
          push_back();
        }
      } catch (...) {
        if (size() <= 1) {
          throw;
        }
        throw;
      }
    }

    List(uint_fast64_t count, const T& value) : List(count) {
      List::iterator it = begin();
      for (; it != end(); ++it) {
        *it = value;
      }
    }

    void cls(size_t allocated) {
      while (allocated > 0) {
        pop_back();
        allocated--;
      }
    }

    ~List() { cls(sz_); sz_ = 0;}

    List(const Allocator& allocator) : List() { alloc = allocator; }

    List(uint_fast64_t count, const Allocator& allocator)
        : alloc(allocator), fakenode_(&fakenode_, &fakenode_), sz_(0) {
      BaseNode* last = &fakenode_;
      uint_fast64_t allocated = 0;
      try {
        for (uint_fast64_t i = 0; i < count; ++i) {
          Node* node = NodeTraits::allocate(alloc, 1);
          NodeTraits::construct(alloc, &static_cast<Node*>(node)->value);
          allocated++;
          BaseNode* prev = last->prev;
          BaseNode* next = static_cast<BaseNode*>(last);
          node->next = next;
          node->prev = prev;
          prev->next = node;
          next->prev = node;
          last = last->next;
          sz_++;
        }
        fakenode_.prev = static_cast<BaseNode*>(last);
        last->next = &fakenode_;
      } catch (...) {
        cls(allocated);
      }
    }

    List([[maybe_unused]] uint_fast64_t count, const T& value,
         const Allocator& allocator)
        : alloc(allocator) {
      List::iterator it = begin();
      for (; it != end(); ++it) {
        *it = value;
      }
    }

    List(const List& list, const Allocator& allocator)
        : alloc(allocator), fakenode_(&fakenode_, &fakenode_), sz_(0) {
      uint_fast64_t allocated = 0;
      try {
        auto it_ = list.begin();
        iterator it(const_cast<BaseNode*>(it_.cur_node_));
        for (uint_fast64_t i = 0; i < list.size(); ++i, ++it_) {
          push_back(*it_);
          allocated++;
        }
      } catch (...) {
        cls(allocated);
        throw;
      }
    }

    List(List&& list)
        : alloc(list.alloc), fakenode_(&fakenode_, &fakenode_) {
      if (list.sz_ == 0) {
        fakenode_.next = &fakenode_;
        fakenode_.prev = &fakenode_;
        return;
      }
      std::swap(fakenode_, list.fakenode_);
      fakenode_.next->prev = &fakenode_;
      fakenode_.prev->next = &fakenode_;
      std::swap(sz_, list.sz_);
    }

    List(const List& list)
        : List(NodeTraits::select_on_container_copy_construction(list.alloc)) {
      List::const_iterator it = list.begin();
      for (; it != list.end(); ++it) {
        try {
          push_back(*it);
        } catch (...) {
          throw;
        }
      }
    }

    List& operator=(const List& list) {
      NodeAlloc new_alloc =
          NodeTraits::propagate_on_container_copy_assignment::value ? list.alloc
                                                                    : alloc;
      try {
        List listik = List(list, new_alloc);
        std::swap(sz_, listik.sz_);
        if (size() == 0) {
          std::swap(fakenode_, listik.fakenode_);
          listik.fakenode_.next->prev = &listik.fakenode_;
          listik.fakenode_.prev->next = &listik.fakenode_;
        } else {
          std::swap(fakenode_, listik.fakenode_);
          fakenode_.next->prev = &fakenode_;
          fakenode_.prev->next = &fakenode_;
          listik.fakenode_.next->prev = &listik.fakenode_;
          listik.fakenode_.prev->next = &listik.fakenode_;
        }
        if constexpr (NodeTraits::propagate_on_container_copy_assignment::
                          value) {
          listik.alloc = alloc;
          alloc = new_alloc;
        }
        return *this;
      } catch (...) {
        throw;
      }
    }

    List& operator=(List&& list) {
      if (this == &list) {
        return *this;
      }
      cls(sz_);
      sz_ = 0;
      if (NodeTraits::propagate_on_container_move_assignment::value &&
          alloc != list.alloc) {
        alloc = list.alloc;
      }

      fakenode_.next = list.fakenode_.next;
      fakenode_.prev = list.fakenode_.prev;

      list.fakenode_.next->prev = &fakenode_;
      list.fakenode_.prev->next = &fakenode_;

      list.fakenode_.prev = &list.fakenode_;
      list.fakenode_.next = &list.fakenode_;
      sz_ = list.sz_;
      list.sz_ = 0;
      return *this;
    }

    Allocator get_allocator() const noexcept { return alloc; }

    template <typename... Args>
    iterator insert(const_iterator it_, Args&&... args) {
      iterator it(const_cast<BaseNode*>(it_.cur_node_));
      BaseNode* node = NodeTraits::allocate(alloc, 1);
      try {
        NodeTraits::construct(alloc, static_cast<Node*>(node),
                              std::forward<Args>(args)...);
      } catch (...) {
        throw;
      }
      BaseNode* prev = (it.cur_node_)->prev;
      BaseNode* next = static_cast<BaseNode*>(it.cur_node_);
      node->next = next;
      node->prev = prev;
      prev->next = node;
      next->prev = node;
      ++sz_;
      return iterator(node);
    }
    iterator erase(const_iterator it_) {
      iterator it(const_cast<BaseNode*>(it_.cur_node_));
      BaseNode* node = it.cur_node_;
      BaseNode* prev = (it.cur_node_)->prev;
      BaseNode* next = (it.cur_node_)->next;
      prev->next = next;
      next->prev = prev;
      NodeTraits::destroy(alloc, static_cast<Node*>(node));
      NodeTraits::deallocate(alloc, static_cast<Node*>(node), 1);
      --sz_;
      return iterator(next);
    }

    void initialise(iterator it, BaseNode* node) {

      if (it == end()) {
        static_cast<Node*>(node)->next = it.cur_node_;
        static_cast<Node*>(node)->prev = it.cur_node_;
        it.cur_node_->next = node;
        it.cur_node_->prev = node;
      } else {
        static_cast<Node*>(node)->next = it.cur_node_;
        static_cast<Node*>(node)->prev = it.cur_node_->prev;

        it.cur_node_->prev->next = node;
        it.cur_node_->prev = node;
      }
      sz_++;
    }

    template <typename... Args>
    iterator move_initialise(iterator it, Args&&... args) {
      Node* node_ = NodeTraits::allocate(alloc, 1);
      try {
        NodeTraits::construct(alloc, &node_->value,
                              std::forward<Args>(args)...);
      } catch (...) {
        throw;
      }
      initialise(it, node_);
      return iterator(node_);
    }

    void push_back(const T& value) { insert(end(), value); }

    void push_back(T&& value) { insert(end(), std::move(value)); }

    void push_back() { insert(end()); }

    void push_front(const T& value) { insert(begin(), value); }

    void push_front(T&& value) { insert(begin(), std::move(value)); }

    void push_front() { insert(begin()); }

    void pop_back() {
      auto deleter = end();
      --deleter;
      erase(deleter);
    }

    void pop_front() { erase(begin()); }

    uint_fast64_t size() const noexcept { return sz_; }

    iterator end() noexcept { return iterator(&fakenode_); }

    iterator begin() noexcept { return iterator(fakenode_.next); }

    const_iterator begin() const noexcept {
      return const_iterator(fakenode_.next);
    }

    const_iterator end() const noexcept { return const_iterator(&fakenode_); }

    const_iterator cbegin() const noexcept { return const_iterator(begin()); }

    const_iterator cend() const noexcept { return const_iterator(end()); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const noexcept {
      return const_reverse_iterator(cend());
    }

    const_reverse_iterator rend() const noexcept {
      return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator crbegin() const noexcept {
      return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const noexcept {
      return const_reverse_iterator(cbegin());
    }
  };
  using value_type = std::pair<const Key, Value>;

  float max_load_factor_ = 1;
  uint_fast64_t bucket_count_ = 0;

 public:
  List list;
  using NodeAlloc = typename std::allocator_traits<
      decltype(list.get_allocator())>::template rebind_alloc<Node>;
  using VectorAlloc = typename std::allocator_traits<
      Allocator>::template rebind_alloc<typename List::iterator>;
  using VectorTraits = std::allocator_traits<VectorAlloc>;
  using NodeTraits = std::allocator_traits<NodeAlloc>;
  [[no_unique_address]] NodeAlloc alloc;

  using iterator = typename List::iterator;
  using const_iterator = typename List::const_iterator;
  std::vector<typename List::iterator, typename VectorTraits::allocator_type>
      mapahash;
//Why mapahash? the answer is here: https://youtu.be/5bId3N7QZec?si=hABGO-W5U_GepjCf
  void swap(UnorderedMap& other) noexcept(
      std::allocator_traits<Allocator>::propagate_on_container_swap::value &&
      std::is_nothrow_swappable<Hash>::value &&
      std::is_nothrow_swappable<EqualTo>::value) {
    std::swap(max_load_factor_, other.max_load_factor_);
    std::swap(hasher, other.hasher);
    std::swap(key_equal, other.key_equal);
    std::swap(mapahash, other.mapahash);
    std::swap(list, other.list);
    std::swap(bucket_count_, other.bucket_count_);
  }

  UnorderedMap(const Allocator& alloc = Allocator())
      :  list(alloc), alloc(alloc), mapahash(alloc) {
    reserve(1);
  }

  UnorderedMap(const UnorderedMap& other)
      : list(other.list),
        alloc(NodeTraits::select_on_container_copy_construction(other.alloc)),
        mapahash(other.mapahash) {}

  UnorderedMap(UnorderedMap&& other)
      : list(std::move(other.list)),
        alloc(other.alloc),
        mapahash(std::move(other.mapahash)) {other.list.sz_ = 0;}

  UnorderedMap& operator=(const UnorderedMap& other) {
    if (this == &other) {
      return *this;
    }
    mapahash = other.mapahash;
    list = other.list;
    alloc = NodeTraits::propagate_on_container_copy_assignment::value
                ? other.alloc
                : alloc;
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) {
    if (this == &other) {
      return *this;
    }
    alloc = NodeTraits::propagate_on_container_move_assignment::value
                ? std::move(other.alloc)
                : alloc;
    mapahash = std::move(other.mapahash);
    list = std::move(other.list);
    rehash(mapahash.size());
    return *this;
  }

  float max_load_factor() const { return max_load_factor_; }

  void max_load_factor(float ml) { max_load_factor_ = ml; }

  Allocator get_allocator() const noexcept { return list.get_allocator(); }

  iterator find(const Key& key) {
    size_t element_hash = hasher(key) % mapahash.size();
    auto it = mapahash[element_hash];
    while (it != end()) {

      if (hasher((*it).first) % mapahash.size() != element_hash) {
        return end();
      }

      if (key_equal(key, (*it).first)) {
        return iterator{it};
      }

      ++it;
    }
    return end();
  }

  const_iterator find(const Key& key) const {
    size_t element_hash = hasher(key) % mapahash.size();
    auto it = mapahash[element_hash];
    while (it != end()) {

      if (hasher((*it).first) % mapahash.size() != element_hash) {
        return end();
      }

      if (key_equal(key, (*it).first)) {
        return iterator{it};
      }

      ++it;
    }
    return end();
  }

  iterator end() noexcept { return list.end(); }

  iterator begin() noexcept { return list.begin(); }

  const_iterator begin() const noexcept { return cbegin(); }

  const_iterator cbegin() const noexcept { return list.cbegin(); }

  const_iterator cend() const noexcept { return list.end(); }

  const_iterator end() const noexcept { return cend(); }

  uint_fast64_t size() const noexcept { return list.size(); }


  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    Node* node = NodeTraits::allocate(alloc, 1);
    NodeTraits::construct(alloc, &node->value, std::forward<Args>(args)...);

    iterator it = find(static_cast<Node*>(node)->value.first);
    if (it != end()) {
      return std::make_pair(it, false);
    }
    if (load_factor() >= max_load_factor()) {
      rehash(2 * mapahash.size());
    }
    list.move_initialise(list.begin(),
                         std::move(*const_cast<Key*>(&node->value.first)),
                         std::move(node->value.second));

    auto iter = list.begin();
    auto second_iter = find_with_node(node);

    NodeTraits::destroy(alloc, &node->value);
    NodeTraits::deallocate(alloc, node, 1);

    if (second_iter == end()) {
      mapahash[hasher(iter->first) % mapahash.size()] = list.begin();
      return {{iter}, true};
    } else {
      list.pop_front();
      return {second_iter, false};
    }
  }

  std::pair<iterator, bool> insert(const value_type& value) {
    return emplace(value);
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    return emplace(std::move(*const_cast<Key*>(&value.first)),
                   std::move(value.second));
  }

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  void insert(std::initializer_list<value_type> ilist) {
    for (auto x : ilist) {
      emplace(x);
    }
  }

  Value& operator[](const Key& key) {
    iterator it = find(key);
    if (it == end()) {
      return (*(emplace(std::make_pair(key, Value())).first)).second;
    }
    return (*it).second;
  }

  Value& at(const Key& key) {
    size_t hash = hasher(key) % mapahash.size();
    iterator it = mapahash[hash];
    for (; it != end() && (hasher(it->first) % mapahash.size()) == hash; ++it) {
      if (key_equal(it->first, key)) {
        return it->second;
      }
    }
    throw std::runtime_error("No value found");
  }
  Value& operator[](Key&& key) {
    iterator it = find(key);
    if (it == end()) {
      return (*(emplace(std::make_pair(key, Value()))).first).second;
    }
    return (*it).second;
  }

  float load_factor() const {
    return static_cast<float>(size()) / mapahash.size();
  }

  void rehash(size_t count) {

    std::vector<typename List::iterator, typename VectorTraits::allocator_type>
        mapahash_(count, list.end());
    List list_;
    auto temp_elem = list.begin();

    for (; temp_elem != list.end();) {
      auto temp = temp_elem;
      ++temp_elem;
      temp.cur_node_->next->prev = temp.cur_node_->prev;
      temp.cur_node_->prev->next = temp.cur_node_->next;
      --list.sz_;
      size_t hash_ = (list.sz_ == 0 ? 0 : hasher((*temp).first) % count);
      if (mapahash_[hash_] != list.end()) {
        list_.initialise(mapahash_[hash_], static_cast<Node*>(temp.cur_node_));
      }

      else {
        list_.initialise(list_.begin(), static_cast<Node*>(temp.cur_node_));
      }

      mapahash_[hash_] = temp;
    }

    list = std::move(list_);
    mapahash = std::move(mapahash_);
  }
  void reserve(uint_fast64_t count) {
    if (static_cast<double>(count) / mapahash.size() <= max_load_factor()) {
      return;
    }
    rehash(count);
  }
  iterator erase(iterator pos) {
    if (pos == end()) {
      return end();
    }

    const Key& key = pos->first;

    uint_fast64_t hash = hasher(key) % mapahash.size();

    if (mapahash[hash] == pos) {
      iterator next_it = pos;
      ++next_it;
      if (next_it != end() &&
          hasher(next_it->first) % mapahash.size() == hash) {
        mapahash[hash] = next_it;
      } else {
        mapahash[hash] = end();
      }
    }

    return list.erase(pos);
  }

  iterator erase(const_iterator pos) {
    if (pos == cend()) {
      return end();
    }

    iterator it = list.erase(pos);
    return it;
  }

  const_iterator erase(const_iterator first, const_iterator last) {
    while (first != last) {
      first = erase(first);
    }
    return first;
  }

  size_t erase(const Key& key) {
    iterator it = find(key);
    if (it == end()) {
      return 0;
    }
    erase(it);
    return 1;
  }

 private:
  iterator find_with_node(Node* node_) {

    auto it = mapahash[hasher((*node_).value.first) % mapahash.size()];
    while (it != list.end() && it.cur_node_ != NULL) {
      if (hasher((*it).first) % mapahash.size() !=
          hasher((*node_).value.first) % mapahash.size()) {
        return end();
      }
      if (key_equal(node_->value.first, it->first)) {
        return {it};
      }
      ++it;
    }
    return end();
  }
};

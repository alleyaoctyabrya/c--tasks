#include <iostream>
#include <memory>

template <size_t N>
class StackStorage {
 public:
  alignas(max_align_t) char arr_[N];
  size_t sz_ = 0;

  StackStorage() = default;

  ~StackStorage() = default;

  StackStorage(const StackStorage&) = delete;

  StackStorage& operator=(const StackStorage&) = delete;

  template <typename T>
  T* allocate(size_t count) {
    size_t size = count * sizeof(T);
    sz_ = sz_ + size + alignof(T) - sz_ % alignof(T);
    return reinterpret_cast<T*>(arr_ + sz_ - size);
  }
};

template <typename T, size_t N>
class StackAllocator {
 private:
  StackStorage<N>* storage_;

 public:
  using value_type = T;

  StackAllocator() = default;

  ~StackAllocator() = default;

  StackStorage<N>* GetStorage() const noexcept { return storage_; }

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& allocator)
      : storage_(allocator.GetStorage()) {}

  StackAllocator& operator=(const StackAllocator& allocator) {
    storage_ = allocator.GetStorage();
    return *this;
  }

  template <typename U>
  struct rebind {
    typedef StackAllocator<U, N> other;
  };

  StackAllocator(StackStorage<N>& storage) : storage_(&storage) {}

  value_type* allocate(size_t cnt) {
    return reinterpret_cast<value_type*>(
        storage_->template allocate<value_type>(cnt));
  }

  template <typename U>
  bool operator==(const StackAllocator<U, N>& allocator) {
    return storage_ == allocator.storage_;
  }

  template <typename U>
  bool operator!=(const StackAllocator<U, N>& allocator) {
    return StackAllocator::operator==(allocator);
  }

  void deallocate(value_type*, size_t) {}
};

template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;

    BaseNode() : prev(this), next(this) {}

    BaseNode(const BaseNode& node) = default;

    BaseNode(BaseNode* first, BaseNode* second) : prev(first), next(second) {}

    BaseNode& operator=(const BaseNode& node) = default;
  };

  struct Node : BaseNode {
    T value;

    Node() : value() {}

    Node(const T& value) : value(value) {}

    Node(const Node& node) = default;

    Node& operator=(const Node& node) = default;
  };

 public:
  using NodeAlloc =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using NodeTraits = std::allocator_traits<NodeAlloc>;
  [[no_unique_address]] NodeAlloc alloc;

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
    //        common_iterator(const common_iterator &iter) : cur_node_(iter.cur_node_) {}

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
  size_t sz_;

  List() : fakenode_(&fakenode_, &fakenode_), sz_(0) {}

  List(size_t count) : List() {
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back();
      }
    } catch (...) {
      if (size() <= 1) {
        throw;
      }
      throw;
    }
  }

  List(size_t count, const T& value) : List(count) {
    List<T, Allocator>::iterator it = begin();
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

  ~List() { cls(sz_); }

  List(const Allocator& allocator) : List() { alloc = allocator; }

  List(size_t count, const Allocator& allocator)
      : alloc(allocator), fakenode_(&fakenode_, &fakenode_), sz_(0) {
    BaseNode* last = &fakenode_;
    size_t allocated = 0;
    try {
      for (size_t i = 0; i < count; ++i) {
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

  List([[maybe_unused]] size_t count, const T& value, const Allocator& allocator)
      : alloc(allocator) {
    List<T, Allocator>::iterator it = begin();
    for (; it != end(); ++it) {
      *it = value;
    }
  }

  List(const List& list, const Allocator& allocator)
      : alloc(allocator), fakenode_(&fakenode_, &fakenode_), sz_(0) {
    size_t allocated = 0;
    try {
      auto it_ = list.begin();
      iterator it(const_cast<BaseNode*>(it_.cur_node_));
      for (size_t i = 0; i < list.size(); ++i, ++it_) {
        push_back(*it_);
        allocated++;
      }
    } catch (...) {
      cls(allocated);
      throw;
    }
  }

  List(const List& list)
      : List(NodeTraits::select_on_container_copy_construction(list.alloc)) {
    List<T, Allocator>::const_iterator it = list.begin();
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
      if constexpr (NodeTraits::propagate_on_container_copy_assignment::value) {
        listik.alloc = alloc;
        alloc = new_alloc;
      }
    } catch (...) {
      throw;
    }
    return *this;
  }

  Allocator get_allocator() const noexcept { return alloc; }

  template <typename... Args>
  iterator insert(const_iterator it_, const Args&... args) {
    iterator it(const_cast<BaseNode*>(it_.cur_node_));
    BaseNode* node = NodeTraits::allocate(alloc, 1);
    try {
      NodeTraits::construct(alloc, static_cast<Node*>(node), args...);
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

  void push_back(const T& value) { insert(end(), value); }

  void push_back() { insert(end()); }

  void push_front(const T& value) { insert(begin(), value); }

  void push_front() { insert(begin()); }

  void pop_back() {
    auto deleter = end();
    --deleter;
    erase(deleter);
  }

  void pop_front() { erase(begin()); }

  size_t size() const noexcept { return sz_; }

  iterator end() noexcept { return iterator(&fakenode_); }

  iterator begin() noexcept { return iterator(fakenode_.next); }

  const_iterator begin() const noexcept {
    return const_iterator(fakenode_.next);
  }

  const_iterator end() const noexcept { return const_iterator(&fakenode_); }

  const_iterator cbegin() const noexcept {
    return const_iterator(fakenode_.next);
  }

  const_iterator cend() const noexcept { return const_iterator(&fakenode_); }

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

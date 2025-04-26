#include <iostream>
#include <vector>
#include <memory>

struct BaseControlBlock {
  size_t shared_count;
  size_t weak_count;

  virtual void use_deleter() = 0;
  virtual void dealloc(BaseControlBlock* block) = 0;
  virtual void* get_pointer() noexcept = 0;

  virtual ~BaseControlBlock() = default;
  BaseControlBlock() = default;
  BaseControlBlock(int shared_count, int weak_count)
      : shared_count(shared_count), weak_count(weak_count) {}
};

template <typename T>
class EnableSharedFromThis;

template <typename U, typename Deleter = std::default_delete<U>,
          typename Alloc = std::allocator<U>>
struct ControlBlockRegular : BaseControlBlock {
  U* ptr;
  [[no_unique_address]] Deleter deleter;
  [[no_unique_address]] Alloc alloc;
  virtual void use_deleter() override { deleter(ptr); }

  template <typename F>
  void __destroy(F* ptr) {
    ptr->~F();
  }

  virtual void dealloc(BaseControlBlock* block) override {
    using alloc_block = typename std::allocator_traits<
        Alloc>::template rebind_alloc<ControlBlockRegular<U, Deleter, Alloc>>;
    using alloc_traits = std::allocator_traits<alloc_block>;
    alloc_block new_alloc(alloc);
    __destroy(reinterpret_cast<ControlBlockRegular*>(block));
    alloc_traits::deallocate(new_alloc,
                             reinterpret_cast<ControlBlockRegular*>(block), 1);
  }

  virtual void* get_pointer() noexcept override { return ptr; }


  ControlBlockRegular() = default;
  ControlBlockRegular(int shared_count, int weak_count, const Deleter& deleter,
                      Alloc alloc, U* ptr)
      : BaseControlBlock(shared_count, weak_count),
        ptr(ptr),
        deleter(deleter),
        alloc(alloc) {}

  ~ControlBlockRegular() {
  }
};

template <typename U, typename Alloc = std::allocator<U>>
struct ControlBlockMakeShared : BaseControlBlock {
  [[no_unique_address]] Alloc alloc;
  U object;
  ControlBlockMakeShared() = default;

  template <typename SomeAlloc, typename... Args>
  ControlBlockMakeShared(int shared_count, int weak_count, SomeAlloc alloc,
                         Args&&... args)
      : BaseControlBlock(shared_count, weak_count),
        alloc(alloc),
        object(std::forward<Args>(args)...) {}

  virtual void use_deleter() override { object.~U(); }

  virtual void dealloc(BaseControlBlock* block) override {
    using alloc_block = typename std::allocator_traits<
        Alloc>::template rebind_alloc<ControlBlockMakeShared<U, Alloc>>;
    using fake_alloc =
        typename std::allocator_traits<Alloc>::template rebind_alloc<Alloc>;
    using alloc_traits = std::allocator_traits<alloc_block>;
    using fake_traits = std::allocator_traits<fake_alloc>;
    alloc_block new_alloc = alloc;
    fake_alloc fake_alloc_ = alloc;
    fake_traits ::destroy(fake_alloc_, &alloc);
    alloc_traits::deallocate(
        new_alloc, reinterpret_cast<ControlBlockMakeShared<U, Alloc>*>(block),
        1);
  }

  virtual void* get_pointer() noexcept override { return &object; }
};

template <typename T>
class SharedPtr {
 private:
  BaseControlBlock* pcb;

  SharedPtr(BaseControlBlock* ptr, bool from_weak) : pcb(ptr) {
    if (from_weak) {
      ++(pcb->shared_count);
    }
  }

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc, Args&&... args);

  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&... args);

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

 public:
  SharedPtr() : pcb(nullptr) {}

  using basic_deleter = decltype(std::default_delete<T>());
  using basic_alloc = decltype(std::allocator<T>());

  template <typename U>
  explicit SharedPtr(U* ptr)
      : pcb(::new ControlBlockRegular<T, basic_deleter, basic_alloc>(
            1, 0, basic_deleter(), basic_alloc(), ptr)) { }

  template <typename U, typename Deleter>
  SharedPtr(U* ptr, const Deleter& deleter)
      : SharedPtr(ptr, deleter, std::allocator<T>()) {}

  template <typename U, typename Deleter, typename Alloc>
  SharedPtr(U* ptr, const Deleter& deleter, Alloc alloc) {
    using new_alloc_control_block = typename std::allocator_traits<
        Alloc>::template rebind_alloc<ControlBlockRegular<T, Deleter, Alloc>>;
    using new_alloc_traits = std::allocator_traits<new_alloc_control_block>;
    new_alloc_control_block new_alloc(alloc);
    auto new_cb(new_alloc_traits::allocate(new_alloc, 1));
    new (new_cb)
        ControlBlockRegular<T, Deleter, Alloc>(1, 0, deleter, alloc, ptr);
    pcb = new_cb;
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other) : pcb(other.pcb) {
    ++(pcb->shared_count);
  }

  template <typename U>
  SharedPtr(SharedPtr<T>&& other) : pcb(other.pcb) {
    other.pcb = nullptr;
  }

  SharedPtr(SharedPtr&& other) : pcb(other.pcb) { other.pcb = nullptr; }

  SharedPtr(const SharedPtr& other) : pcb(other.pcb) {
    if (!pcb) {
      return;
    }

    ++(pcb->shared_count);
  }

  SharedPtr& operator=(const SharedPtr& other) {
    if (pcb == other.pcb) {
      return *this;
    }
    handler();
    pcb = other.pcb;
    ++(pcb->shared_count);
    return *this;
  }
  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other) {
    handler();
    pcb = other.pcb;
    other.pcb = nullptr;
    return *this;
  }
  template <typename U>
  SharedPtr<T>& operator=(const SharedPtr<U>& other) {
    SharedPtr(other).swap(*this);
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) {
    handler();
    pcb = other.pcb;
    other.pcb = nullptr;
    return *this;
  }
  size_t use_count() const noexcept { return (!pcb ? 0 : pcb->shared_count); }

  T& operator*() const noexcept { return *(operator->()); }

  T* operator->() const noexcept {
    return reinterpret_cast<T*>(pcb->get_pointer());
  }

  T* get() const noexcept {
    return (!pcb ? nullptr : reinterpret_cast<T*>(pcb->get_pointer()));
  }

  template <class U>
  void reset(U* ptr) {
    SharedPtr(ptr).swap(*this);
  }
  void reset() noexcept { SharedPtr().swap(*this); }
  void swap(SharedPtr& other) noexcept { std::swap(pcb, other.pcb); }
  void handler() noexcept {
    if (!pcb) {
      return;
    }
    --pcb->shared_count;
    if (pcb->shared_count == 0) {
      pcb->use_deleter();
      if (pcb->weak_count == 0) {
        pcb->dealloc(pcb);
      }
    }
  }
  ~SharedPtr() { handler(); }
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using alloc_block = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
  using alloc_traits = std::allocator_traits<alloc_block>;

  alloc_block new_alloc = alloc;
  auto block = alloc_traits::allocate(new_alloc, 1);
  alloc_traits::construct(new_alloc, block, 1, 0, alloc,
                          std::forward<Args>(args)...);
  return SharedPtr<T>(reinterpret_cast<BaseControlBlock*>(block), false);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {

  template <typename U>
  friend class EnableSharedFromThis;

  template <class U>
  friend class WeakPtr;

 protected:
  BaseControlBlock* cb;

 public:
  WeakPtr() : cb(nullptr) {}

  WeakPtr(std::nullptr_t) : cb(nullptr) {}

  WeakPtr(const WeakPtr& other) : cb(other.cb) {
    if (cb != nullptr) {
      ++(cb->weak_count);
    }
  }
  template <typename U>
  WeakPtr(const WeakPtr<U>& other) noexcept : cb(other.cb) {
    if (cb != nullptr) {
      ++(cb->weak_count);
    }
  }
  WeakPtr(WeakPtr&& other) noexcept : cb(other.cb) { other.cb = nullptr; }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) : cb(other.cb) {
    other.cb = nullptr;
  }
  WeakPtr& operator=(const WeakPtr& other) {
    handler(true);
    cb = other.cb;
    ++(cb->weak_count);
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    handler(true);
    cb = other.cb;
    ++(cb->weak_count);
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) {
    handler(true);
    cb = other.cb;
    other.cb = nullptr;
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) {
    handler(true);
    cb = other.cb;
    other.cb = nullptr;
    return *this;
  }
  size_t use_count() const noexcept {
    return (cb == nullptr ? 0 : cb->shared_count);
  }

  WeakPtr(const SharedPtr<T>& other) : cb(other.pcb) {
    if (cb != nullptr) {
      ++(cb->weak_count);
    }
  }
  template <typename U>
  WeakPtr(const SharedPtr<U>& other) : cb(other.pcb) {
    if (cb != nullptr) {
      ++(cb->weak_count);
    }
  }
  bool expired() const noexcept {
    if (cb == nullptr) {
      return false;
    }
    return (cb->shared_count == 0);
  }
  SharedPtr<T> lock() const noexcept {
    if (expired()) {
      return SharedPtr<T>();
    }
    return SharedPtr<T>(cb, true);
  }
  void handler(bool decrement) noexcept {
    if (cb == nullptr) {
      return;
    }
    if (decrement) {
      --(cb->weak_count);
    }
    if (cb->shared_count == 0 && cb->weak_count == 0) {
      cb->dealloc(cb);

    }
  }
  ~WeakPtr() { handler(true); }
  void swap(WeakPtr& other) noexcept { std::swap(cb, other.cb); }
  void reset() noexcept {
    handler(false);
    cb = nullptr;
  }
};

template <typename T>
class EnableSharedFromThis {
  template <typename U>
  friend class SharedPtr;

 private:
  WeakPtr<T> wptr = nullptr;

 protected:
  SharedPtr<T> shared_from_this() {
    if (wptr.cb == nullptr || wptr.expired()) {
      throw std::exception();
    }
    return wptr.lock();
  };
};

template<typename T>
SharedPtr(WeakPtr<T>) ->  SharedPtr<T>;

template<typename T>
WeakPtr(SharedPtr<T>) ->  WeakPtr<T>;

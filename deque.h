#include <cmath>
#include <string>
#include <iostream>
#include <iterator>
#include <memory>

template <typename T>
class Deque {
 private:
  static const int kBlockSize = 16;
  T** wrapper_ = nullptr;
  int outer_size_ = 0;
  std::pair<int, int> first_ptrs_;
  std::pair<int, int> second_ptrs_;

  template <bool is_const>
  struct CommonIterator {
   private:
    std::ptrdiff_t position_ = 0;
    T** wrapper_ = nullptr;

   public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using pointer = value_type*;
    using reference = value_type&;

    CommonIterator(T** wrapper, int pos) : position_(pos), wrapper_(wrapper) {}

    CommonIterator(const CommonIterator& iter) = default;

    CommonIterator& operator=(const CommonIterator& object) = default;

    ~CommonIterator() = default;

    reference operator*() const noexcept {
      return wrapper_[position_ / kBlockSize][position_ % kBlockSize];
    }

    pointer operator->() const noexcept { return &(operator*()); }

    CommonIterator& operator++() noexcept {
      ++position_;
      return *this;
    }

    CommonIterator operator++(int) noexcept {
      CommonIterator temp = *this;
      ++(*this);
      return temp;
    }

    CommonIterator& operator--() noexcept {
      --position_;
      return *this;
    }

    CommonIterator operator--(int) noexcept {
      CommonIterator temp = *this;
      --(*this);
      return temp;
    }

    CommonIterator& operator+=(difference_type val) noexcept {
      position_ += val;
      return *this;
    }

    CommonIterator& operator-=(difference_type val) noexcept {
      position_ -= val;
      return *this;
    }

    CommonIterator operator+(difference_type val) const noexcept {
      CommonIterator iter = *this;
      iter += val;
      return iter;
    }

    CommonIterator operator-(difference_type val) const noexcept {
      CommonIterator iter = *this;
      iter -= val;
      return iter;
    }

    difference_type operator-(const CommonIterator& iter) const noexcept {
      return std::abs(position_ - iter.position_);
    }

    friend bool operator==(const CommonIterator& iter,
                           const CommonIterator& iter1) noexcept {
      return (iter.position_ == iter1.position_);
    }

    friend bool operator!=(const CommonIterator& iter,
                           const CommonIterator& iter1) noexcept {
      return !operator==(iter, iter1);
    }

    friend bool operator>=(const CommonIterator& iter1,
                           const CommonIterator& iter2) noexcept {
      return (iter1.position_ >= iter2.position_);
    }

    friend bool operator<=(const CommonIterator& iter1,
                           const CommonIterator& iter2) noexcept {
      return (iter1.position_ <= iter2.position_);
    }

    friend bool operator>(const CommonIterator& iter,
                          const CommonIterator& iter1) noexcept {
      return !operator<=(iter, iter1);
    }

    friend bool operator<(const CommonIterator& iter,
                          const CommonIterator& iter1) noexcept {
      return !operator>=(iter, iter1);
    }

    operator CommonIterator<true>() const noexcept {
      return CommonIterator<true>(wrapper_, position_);
    }
  };

 public:
  using const_iterator = CommonIterator<true>;
  using iterator = CommonIterator<false>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  Deque() noexcept
      : outer_size_(1),
        first_ptrs_(0, kBlockSize / 2),
        second_ptrs_(0, kBlockSize / 2) {
    wrapper_ = reinterpret_cast<T**>(new char[outer_size_ * sizeof(T*)]);
    wrapper_[0] = reinterpret_cast<T*>(new char[kBlockSize * sizeof(T)]);
  }

  Deque(const Deque& deque)
      : outer_size_(deque.outer_size_),
        first_ptrs_(deque.first_ptrs_),
        second_ptrs_(deque.second_ptrs_) {
    wrapper_ = reinterpret_cast<T**>(new char[outer_size_ * sizeof(T*)]);
    for (int i = 0; i < outer_size_; ++i) {
      T* temp = reinterpret_cast<T*>(new char[kBlockSize * sizeof(T)]);
      wrapper_[i] = temp;
    }
    size_t sz = 0;
    for (int i = 0; i < outer_size_; ++i) {
      if (i >= first_ptrs_.first && i <= second_ptrs_.first) {
        for (int j = (i == first_ptrs_.first ? first_ptrs_.second : 0);
             j < (i == second_ptrs_.first ? second_ptrs_.second : kBlockSize);
             ++j) {
          try {
            new (wrapper_[i] + j) T(deque.wrapper_[i][j]);
            sz++;
          } catch (...) {
            while (sz-- > 0) {
              pop_back();
            }
            throw;
          }
        }
      }
    }
  }

  explicit Deque(int size)
      : outer_size_((size + kBlockSize - 1) / kBlockSize) {
    if (outer_size_ == size / kBlockSize) {
      outer_size_++;
    }
    wrapper_ = reinterpret_cast<T**>(new char[outer_size_ * sizeof(T*)]);
    int temporary_size = size;
    size_t sz = 0;
    for (int i = 0; i < outer_size_; ++i) {
      wrapper_[i] = reinterpret_cast<T*>(new char[kBlockSize * sizeof(T)]);
      if constexpr (std::is_default_constructible_v<T>) {
        try {
          for (int j = 0; j < (i == outer_size_ - 1 ? size % kBlockSize : kBlockSize);
               ++j) {
            new (wrapper_[i] + j) T();
            sz++;
          }
        } catch (...) {
          while (sz-- > 0) {
            pop_back();
          }
          throw;
        }
      }
      temporary_size -= kBlockSize;
    }
    first_ptrs_ = {0, 0};
    second_ptrs_ = {outer_size_ - 1, temporary_size + kBlockSize};
  }

  Deque& operator=(const Deque& other) {
    Deque<T> temp = other;
    std::swap(first_ptrs_, temp.first_ptrs_);
    std::swap(second_ptrs_, temp.second_ptrs_);
    std::swap(outer_size_, temp.outer_size_);
    std::swap(wrapper_, temp.wrapper_);
    return *this;
  }

  Deque(int size, const T& var) : Deque(size) {
    for (int i = 0; i < outer_size_; ++i) {
      int sze = (i < second_ptrs_.first ? kBlockSize : second_ptrs_.second);
      for (int j = 0; j < sze; ++j) {
        wrapper_[i][j] = var;
      }
    }
  }

  void DestroyAllElementsAndDeallocate() noexcept {
    for (int i = first_ptrs_.first * kBlockSize + first_ptrs_.second;
         i < second_ptrs_.first * kBlockSize + second_ptrs_.second; ++i) {
      size_t pos_external_array = i / kBlockSize;
      size_t pos_internal_array = i % kBlockSize;
      (wrapper_[pos_external_array] + pos_internal_array)->~T();
    }
    for (int i = 0; i < outer_size_; ++i) {
      delete[] reinterpret_cast<uint8_t*>(wrapper_[i]);
    }
    delete[] wrapper_;
  }

  ~Deque() noexcept { DestroyAllElementsAndDeallocate(); }

  size_t size() const noexcept {
    return (second_ptrs_.first * kBlockSize + second_ptrs_.second) -
           (first_ptrs_.first * kBlockSize + first_ptrs_.second);
  }

  T& operator[](const size_t index) noexcept {
    return wrapper_[(index + first_ptrs_.second) / kBlockSize + first_ptrs_.first]
                   [(index + first_ptrs_.second) % kBlockSize];
  }

  const T& operator[](const size_t index) const noexcept {
    return wrapper_[(index + first_ptrs_.second) / kBlockSize + first_ptrs_.first]
                   [(index + first_ptrs_.second) % kBlockSize];
  }

  T& at(size_t index) {
    if (index >= size()) {
      throw std::out_of_range("Bad index");
    }
    return Deque::operator[](index);
  }

  const T& at(size_t index) const {
    if (index >= size()) {
      throw std::out_of_range("Bad index");
    }
    return Deque::operator[](index);
  }

  void push_back(const T& value) {
    try {
      new (wrapper_[second_ptrs_.first] + second_ptrs_.second) T(value);
    } catch (...) {
      throw;
    }

    if (second_ptrs_.second < kBlockSize - 1) {
      second_ptrs_.second++;
    } else {
      if (outer_size_ - 1 > second_ptrs_.first) {
        second_ptrs_.first++;
        second_ptrs_.second = 0;
      } else {
        second_ptrs_.second++;
        Deque temp = *this;
        delete[] wrapper_;
        outer_size_ *= 3;
        wrapper_ = reinterpret_cast<T**>(new char[outer_size_ * sizeof(T*)]);
        for (int i = 0; i < outer_size_; ++i) {
          wrapper_[i] = reinterpret_cast<T*>(new char[kBlockSize * sizeof(T)]);
        }
        second_ptrs_.second = kBlockSize - 1;
        second_ptrs_.first += outer_size_ / 3;
        first_ptrs_.first += outer_size_ / 3;
        std::copy(temp.begin(), temp.end(), begin());
        second_ptrs_.second = 0;
        second_ptrs_.first++;
      }
    }
  }

  void push_front(const T& value) {
    if (first_ptrs_.second > 0) {
      first_ptrs_.second--;
      try {
        new (wrapper_[first_ptrs_.first] + first_ptrs_.second) T(value);
      } catch (...) {
        first_ptrs_.second++;
        throw;
      }
    } else {
      if (first_ptrs_.first > 0) {
        first_ptrs_.first--;
        first_ptrs_.second = kBlockSize - 1;
        try {
          new (wrapper_[first_ptrs_.first] + first_ptrs_.second) T(value);
        } catch (...) {
          first_ptrs_.first++;
          first_ptrs_.second = 0;
          throw;
        }
      } else {
        Deque temp = *this;
        delete[] wrapper_;
        outer_size_ *= 3;
        wrapper_ = reinterpret_cast<T**>(new char[outer_size_ * sizeof(T*)]);
        for (int i = 0; i < outer_size_; ++i) {
          wrapper_[i] = reinterpret_cast<T*>(new char[kBlockSize * sizeof(T)]);
        }
        second_ptrs_.first += outer_size_ / 3;
        first_ptrs_.first += outer_size_ / 3;
        for (size_t i = 0; i < temp.size(); ++i) {
          wrapper_[(i + first_ptrs_.second) / kBlockSize + first_ptrs_.first]
                  [(i + first_ptrs_.second) % kBlockSize] = temp[i];
        }
        first_ptrs_.second = kBlockSize - 1;
        first_ptrs_.first--;
        try {
          new (wrapper_[first_ptrs_.first] + first_ptrs_.second) T(value);
        } catch (...) {
          first_ptrs_.first++;
          first_ptrs_.second = 0;
          throw;
        }
      }
    }
  }

  void pop_back() noexcept {
    (wrapper_[second_ptrs_.first] + second_ptrs_.second)->~T();
    if (second_ptrs_.second > 0) {
      second_ptrs_.second--;
    } else {
      second_ptrs_.first--;
      second_ptrs_.second = kBlockSize - 1;
    }
  }

  void pop_front() noexcept {
    if (size() == 0) {
      return;
    }
    (wrapper_[first_ptrs_.first] + first_ptrs_.second)->~T();
    if (first_ptrs_.second < kBlockSize - 1) {
      first_ptrs_.second++;
    } else {
      first_ptrs_.first++;
      first_ptrs_.second = 0;
    }
  }

  void insert(iterator it, const T& var) {
    if (size() == 0 || it == end()) {
      push_back(var);
    } else {
      Deque<T>::iterator iter = end();
      iter--;
      push_back(*iter);
      iter = end();
      iter--;
      for (; iter != it; --iter) {
        *iter = *(iter - 1);
      }
      *it = var;
    }
  }

  void erase(iterator it) {
    Deque<T>::iterator iter = end();
    iter--;
    for (; iter != it; it++) {
      *(it - 1) = *it;
    }
    pop_back();
  }

  iterator end() noexcept {
    return iterator(wrapper_, kBlockSize * (second_ptrs_.first) + second_ptrs_.second);
  }

  iterator begin() noexcept {
    return iterator(wrapper_, kBlockSize * (first_ptrs_.first) + first_ptrs_.second);
  }

  const_iterator begin() const noexcept {
    return const_iterator(wrapper_, kBlockSize * (first_ptrs_.first) + first_ptrs_.second);
  }

  const_iterator end() const noexcept {
    return const_iterator(wrapper_, kBlockSize * (second_ptrs_.first) + second_ptrs_.second);
  }

  const_iterator cbegin() const noexcept { return begin(); }

  const_iterator cend() const noexcept { return end(); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(cend());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(cbegin());
  }

  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  const_reverse_iterator crend() const noexcept { return rend(); }
};

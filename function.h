#include <iostream>
#include <functional>
#include <memory>
#include <concepts>
#include <type_traits>
#include <utility>

template<class F>
struct strip_signature;

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...)> {
    using type = Ret(Args...);
};

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...) const> {
    using type = Ret(Args...);
};

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...) &> {
    using type = Ret(Args...);
};

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...) const &> {
    using type = Ret(Args...);
};

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...) noexcept> {
    using type = Ret(Args...);
};

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...) const noexcept> {
    using type = Ret(Args...);
};

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...) & noexcept> {
    using type = Ret(Args...);
};

template<class Ret, class Pointed, class... Args>
struct strip_signature<Ret (Pointed::*)(Args...) const & noexcept> {
    using type = Ret(Args...);
};

template<bool isMoveOnly, typename Signature>
class Base_function;

template<bool isMoveOnly, typename Ret, typename... Args>
class Base_function<isMoveOnly, Ret(Args...)> {
private:
    static constexpr size_t BUFFER_SIZE = 16;

    alignas(std::max_align_t) char buffer[BUFFER_SIZE];

    void* fptr = nullptr;

    struct vtable_t {
        Ret (*invoke)(void*, Args&&...);
        void (*destroy)(void*);
        void (*copy)(void**, const void*, void*);
        void (*move)(void**, void*, void*);
    };

    const vtable_t* vtable_ptr = nullptr;

    template <typename F>
    static Ret invoker(void* fptr, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>) {
      return std::invoke(*static_cast<F*>(fptr), std::forward<Args>(args)...);
    }

    template <typename F>
    static void destroyer(void* fptr) noexcept {
      if constexpr (sizeof(F) > BUFFER_SIZE) {
        delete static_cast<F*>(fptr);
      } else {
        static_cast<F*>(fptr)->~F();
      }
    }

    template <typename F>
    static void copier(void** dest_fptr, const void* src_fptr, void* dest_buffer) {
      if constexpr (isMoveOnly) {
        *(dest_fptr) = nullptr;
      }
      else {
        if constexpr (sizeof(F) > BUFFER_SIZE) {
          *(dest_fptr) = new F(*static_cast<const F*>(src_fptr));
        }
        else {
          new (dest_buffer) F(*static_cast<const F*>(src_fptr));
          *(dest_fptr) = dest_buffer;
        }
      }
    }

    template <typename F>
    static void mover(void** dest_fptr, void* src_fptr, void* dest_buffer) noexcept {
      if constexpr (sizeof(F) > BUFFER_SIZE) {
        *(dest_fptr) = src_fptr;
        *reinterpret_cast<void**>(src_fptr) = nullptr;
      }
      else {
        new (dest_buffer) F(std::move(*static_cast<F*>(src_fptr)));
        *(dest_fptr) = dest_buffer;
      }
    }

    template <typename F>
    static const vtable_t* get_vtable() {
      static const vtable_t vt = {
              &invoker<F>,
              &destroyer<F>,
              &copier<F>,
              &mover<F>
      };
      return &vt;
    }

public:
    Base_function() noexcept = default;

    Base_function(std::nullptr_t) noexcept : fptr(nullptr), vtable_ptr(nullptr) {}

    Base_function(const Base_function& other)
    noexcept(!isMoveOnly && std::is_nothrow_copy_constructible_v<Base_function>)
    requires (!isMoveOnly)
    {
      if (other.vtable_ptr && other.fptr) {
        vtable_ptr = other.vtable_ptr;
        vtable_ptr->copy(&fptr, other.fptr, buffer);
      }
    }

    Base_function(Base_function&& other) noexcept {
      if (other.vtable_ptr && other.fptr) {
        vtable_ptr = other.vtable_ptr;
        vtable_ptr->move(&fptr, other.fptr, buffer);
        other.fptr = nullptr;
        other.vtable_ptr = nullptr;
      }
    }

    template <typename F>
    requires(std::is_invocable_r_v<Ret, F, Args...> &&
             !std::is_same_v<std::remove_cvref_t<F>, Base_function>)
    Base_function(F&& func) {
      vtable_ptr = get_vtable<std::decay_t<F>>();
      if constexpr (sizeof(std::decay_t<F>) > BUFFER_SIZE) {
        fptr = new std::decay_t<F>(std::forward<F>(func));
      }
      else {
        fptr = buffer;
        new (buffer) std::decay_t<F>(std::forward<F>(func));
      }
    }

    ~Base_function() {
      if (vtable_ptr && fptr) {
        vtable_ptr->destroy(fptr);
      }
    }

    Base_function& operator=(const Base_function& other)
    noexcept(!isMoveOnly)
    requires (!isMoveOnly)
    {
      if (this != &other) {
        if (vtable_ptr && fptr) {
          vtable_ptr->destroy(fptr);
        }
        if (other.vtable_ptr && other.fptr) {
          vtable_ptr = other.vtable_ptr;
          vtable_ptr->copy(&fptr, other.fptr, buffer);
        }
        else {
          fptr = nullptr;
          vtable_ptr = nullptr;
        }
      }
      return *this;
    }

    Base_function& operator=(Base_function&& other) noexcept {
      if (this != &other) {
        if (vtable_ptr && fptr) {
          vtable_ptr->destroy(fptr);
        }
        if (other.vtable_ptr && other.fptr) {
          vtable_ptr = other.vtable_ptr;
          vtable_ptr->move(&fptr, other.fptr, buffer);
          other.fptr = nullptr;
          other.vtable_ptr = nullptr;
        }
        else {
          fptr = nullptr;
          vtable_ptr = nullptr;
        }
      }
      return *this;
    }

    template <typename F>
    requires(std::is_invocable_r_v<Ret, F, Args...> &&
             !std::is_same_v<std::remove_cvref_t<F>, Base_function>)
    Base_function& operator=(F&& f) {
      if (vtable_ptr && fptr) {
        vtable_ptr->destroy(fptr);
      }
      vtable_ptr = get_vtable<std::decay_t<F>>();
      if constexpr (sizeof(std::decay_t<F>) > BUFFER_SIZE) {
        fptr = new std::decay_t<F>(std::forward<F>(f));
      }
      else {
        fptr = buffer;
        new (buffer) std::decay_t<F>(std::forward<F>(f));
      }
      return *this;
    }

    template <typename F>
    Base_function& operator=(std::reference_wrapper<F> f) noexcept {
      *this = [&f](Args... args) -> Ret {
          return f.get()(args...);
      };
      return *this;
    }

    void swap(Base_function& other) noexcept {
      if (this == &other) return;

      Base_function temp = std::move(*this);
      *this = std::move(other);
      other = std::move(temp);
    }

    Ret operator()(Args... args) const {
      if (!vtable_ptr || !fptr) {
        throw std::bad_function_call();
      }
      return vtable_ptr->invoke(const_cast<void*>(fptr), std::forward<Args>(args)...);
    }

    explicit operator bool() const noexcept {
      return vtable_ptr && fptr;
    }

    bool operator==(std::nullptr_t) const noexcept {
      return !(*this);
    }

    bool operator!=(std::nullptr_t) const noexcept {
      return !!(*this);
    }
};

template <typename Signature>
class Function : public Base_function<false, Signature> {
public:
    using Base_function<false, Signature>::Base_function;
};

template <typename Signature>
class MoveOnlyFunction : public Base_function<true, Signature> {
public:
    using Base_function<true, Signature>::Base_function;
};

template<class F, class Stripped = typename strip_signature<decltype(&F::operator())>::type>
Function(F) -> Function<Stripped>;

template<class F, class Stripped = typename strip_signature<decltype(&F::operator())>::type>
MoveOnlyFunction(F&&) -> MoveOnlyFunction<Stripped>;

template<class Ret, class... Args>
Function(Ret(*)(Args...)) -> Function<Ret(Args...)>;

template<class Ret, class... Args>
Function(const Ret(*)(Args...)) -> Function<Ret(Args...)>;

template<class Ret, class... Args>
MoveOnlyFunction(Ret(*)(Args...)) -> MoveOnlyFunction<Ret(Args...)>;

template<class Ret, class... Args>
MoveOnlyFunction(const Ret(*)(Args...)) -> MoveOnlyFunction<Ret(Args...)>;

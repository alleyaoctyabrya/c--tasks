//not very good implementation of stack
#include <cstring>
#include <iostream>

struct Stack {
  char** _arr = nullptr;
  size_t* _sizes = nullptr;
  size_t _size = 0;
  size_t _capacity = 0;
};

typedef void (*Function)(Stack& stack);

void get(char** temp, size_t& temp_size, size_t& temp_capacity, char flag,
         char second_flag = '\n') {
  char symbol;
  if (*temp == nullptr) {
    *temp = new char[temp_size];
  }

  while (std::cin.get(symbol) && symbol != flag && symbol != second_flag) {
    if (temp_capacity == temp_size) {
      temp_size = 2 * temp_size + 1;
      char* new_temp = new char[temp_size];
      std::memcpy(new_temp, *temp, temp_capacity * sizeof(char));
      delete[] * temp;
      *temp = new_temp;
    }
    (*temp)[temp_capacity] = symbol;
    temp_capacity++;
  }
}

void pop(Stack& stack) {
  if (stack._capacity == 0) {
    std::cout << "error\n";
  } else {
    for (size_t i = 0; i < stack._sizes[stack._capacity - 1]; ++i) {
      std::cout << stack._arr[stack._capacity - 1][i];
    }
    std::cout << "\n";
    delete[] stack._arr[stack._capacity - 1];
    --stack._capacity;
  }
}

void back(Stack& stack) {
  if (static_cast<int>(stack._capacity) - 1 < 0) {
    std::cout << "error\n";
  } else {
    for (size_t i = 0; i < stack._sizes[stack._capacity - 1]; ++i) {
      std::cout << stack._arr[stack._capacity - 1][i];
    }
    std::cout << "\n";
  }
}

void size(Stack& stack) { std::cout << stack._capacity << "\n"; }

void clear(Stack& stack) {
  for (size_t i = 0; i < stack._capacity; ++i) {
    delete[] stack._arr[i];
  }
  stack._capacity = 0;
  std::cout << "ok\n";
}

void exit(Stack& stack) {
  std::cout << "bye\n";
  for (size_t i = 0; i < stack._capacity; ++i) {
    delete[] stack._arr[i];
  }
  delete[] stack._arr;
  delete[] stack._sizes;
  ::exit(0);
}

void push(Stack& stack) {
  if (stack._capacity == stack._size) {
    stack._size = 2 * stack._size + 1;
    if (stack._size == 1) {
      stack._arr = new char*[stack._size];
      stack._sizes = new size_t[stack._size];
    } else {
      char** new_arr = new char*[stack._size];
      size_t* new_sizes = new size_t[stack._size];
      std::memcpy(new_arr, stack._arr, stack._capacity * sizeof(char*));
      std::memcpy(new_sizes, stack._sizes, stack._capacity * sizeof(size_t));
      delete[] stack._arr;
      delete[] stack._sizes;
      stack._arr = new_arr;
      stack._sizes = new_sizes;
    }
  }
  size_t temp_capacity = 0;
  size_t temp_size = 1;
  char* temp = nullptr;
  get(&temp, temp_size, temp_capacity, '\n');
  stack._sizes[stack._capacity] = temp_capacity;
  stack._arr[stack._capacity] = temp;
  stack._capacity++;
  std::cout << "ok\n";
}

bool is_equal(char* first, const char* second, size_t capacity) {
  if (capacity != strlen(second)) {
    return false;
  }
  for (size_t i = 0; i < capacity; ++i) {
    if (first[i] != second[i]) {
      return false;
    }
  }
  return true;
}

int main() {
  Stack stack;
  char* temp = nullptr;
  const size_t data_size = 6;
  const char* data[] = {"push", "pop", "back", "size", "clear", "exit"};
  Function functions[] = {push, pop, back, size, clear, exit};
  size_t temp_size = 1;
  size_t temp_capacity = 0;
  while (true) {
    get(&temp, temp_size, temp_capacity, ' ');
    for (size_t i = 0; i < data_size; ++i) {
      if (is_equal(temp, data[i], temp_capacity)) {
        functions[i](stack);
        break;
      }
    }
    temp_capacity = 0;
  }
}

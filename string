#include <iostream>
#include <cstring>
class String{
private:
  char* array = nullptr;
  size_t size_of_string = 0;
  size_t capacity_of_string = 1;
  String(size_t count) : array(new char[count + 1]), size_of_string(count), capacity_of_string(count + 1) {
    array[size_of_string] = '\0';
  }

public:
  String() = default;
  String(size_t count, const char& symbol) : String(count) {
    memset(array, symbol, count);
  }
  String(const char symbol) : String(1, symbol) {}
  String(const char string[]) : String(strlen(string)) {
    memcpy(array, string, size_of_string);
  }
  String(const String& origin) : String(origin.size_of_string) {
    memcpy(array, origin.array, size_of_string);
  }
  void swap(String& origin) {
    std::swap(array, origin.array);
    std::swap(size_of_string, origin.size_of_string);
    std::swap(capacity_of_string, origin.capacity_of_string);
  }

  String& operator+=(const String& another) {//for const string or string
    if (size_of_string + another.size_of_string + 1 <= capacity_of_string) {
      memcpy(array + size_of_string, another.array, another.size_of_string);
      size_of_string += another.size_of_string;
      array[size_of_string] = '\0';
    } else {
      capacity_of_string = (size_of_string + another.size_of_string) * 2 + 1;
      char* temp = new char[capacity_of_string];
      memcpy(temp, array, size_of_string);
      delete[] array;
      array = temp;
      memcpy(array + size_of_string, another.array, another.size_of_string);
      size_of_string += another.size_of_string;
      array[size_of_string] = '\0';
    }
    return *this;
  }
  const char& operator[](const int a) const {
    return array[a];
  }
  char& operator[](const int a) {
    return array[a];
  }
  size_t length() const {
    return size_of_string;
  }
  size_t size() const {
    return size_of_string;
  }
  size_t capacity() const {
    return capacity_of_string - 1;
  }
  String& operator=(const String& origin) {
    String copy = origin;
    swap(copy);
    return *this;
  }

  void push_back(char letter) {
    if (size_of_string == capacity_of_string - 1) {
      capacity_of_string *= 2 + 1;
      char *temp = new char[capacity_of_string];
      memcpy(temp, array, size_of_string);
      delete[] array;
      array = temp;
    }
    array[size_of_string] = letter;
    array[++size_of_string] = '\0';
  }
  void pop_back(){
    array[size_of_string--] = '\0';
  }
  String substr(size_t start, size_t count) const{
    String result;
    result.capacity_of_string = count + 1;
    result.size_of_string = count;
    result.array = new char[capacity_of_string];
    memcpy(result.array, array + start, count);
    return result;
  }
  bool empty() {
    return (size_of_string == 0);
  }
  void clear() {
    size_of_string = 0;
    array[size_of_string] = '\0';
  }
  void shrink_to_fit() {
    capacity_of_string = size_of_string + 1;
    char* temp = new char [capacity_of_string];
    memcpy(temp, array, size_of_string);
    delete[] array;
    array = temp;
    memcpy(array, temp, size_of_string);
    array[size_of_string] = '\0';
  }
  const char& front() const {
    return array[0];
  };
  char& front() {
    return array[0];
  };
  const char& back() const {
    return array[size_of_string-1];
  }
  char& back() {
    return array[size_of_string-1];
  }
  const char* data() const{
    return array;
  }
  char* data() {
    return array;
  }
  size_t find(const String& substring) const {
    for (size_t i = 0; i <= this->size() - substring.size(); ++i) {
      bool isEqual = true;
      for (size_t j = i; j < i + substring.size(); ++j) {
        if (substring[j-i] != array[j]) {
          isEqual = false;
          break;
        }
      }
      if (isEqual) {
        return i;
      }
    }
    return this->length();
  }
  size_t rfind(const String& substring) const{
    for (size_t i = this->size()-1; i >= substring.size()-1; --i) {
      bool isEqual = true;
      for (size_t j = i; j > i - substring.size(); --j) {
        if (substring[j - i + substring.size() - 1] != array[j]) {
          isEqual = false;
          break;
        }
      }
      if (isEqual) {
        return i-substring.size()+1;
      }
    }
    return this->length();
  }
  ~String() {
    delete[] array;
  }
};

bool operator<(const String& one, const String& another) {
  size_t check_size = std::min(one.size(), another.size());
  for (size_t i = 0; i < check_size; ++i) {
    if (one[i] >= another[i]) {
      return false;
    }
  }
  if (one.size() >= another.size()){
    return false;
  }
  return true;
}

bool operator<=(const String& one, const String& another) {
  size_t check_size = std::min(one.size(), another.size());
  for (size_t i = 0; i < check_size; ++i) {
    if (one[i] > another[i]) {
      return false;
    }
  }
  if (one.size() > another.size()){
    return false;
  }
  return true;
}

bool operator>(const String& one, const String& another) {
  size_t check_size = std::min(one.size(), another.size());
  for (size_t i = 0; i < check_size; ++i) {
    if (one[i] <= another[i]) {
      return false;
    }
  }
  if (one.size() <= another.size()){
    return false;
  }
  return true;
}

bool operator>=(const String& one, const String& another) {
  size_t check_size = std::min(one.size(), another.size());
  for (size_t i = 0; i < check_size; ++i) {
    if (one[i] < another[i]) {
      return false;
    }
  }
  if (one.size() < another.size()){
    return false;
  }
  return true;
}

bool operator==(const String& first, const String& second){
  if (first.size() != second.size()) {
    return false;
  }
  for (size_t i = 0; i < first.size(); ++i) {
    if (first[i] != second[i]){
      return false;
    }
  }
  return true;
}

bool operator!=( const String& first, const String& second) {
  return !(first == second);
}

std::ostream& operator<<(std::ostream& out, const String& str) {
  for (size_t i = 0; i < str.size(); ++i){
    out << str[i];
  }
  return out;
}
String operator+(const String& first, const String& second) {
  String result = first;
  result += second;
  return result;
}
std::istream& operator>>(std::istream& in, String& str) {
  char symbol;
  while (in.get(symbol) && symbol != '\n') {
    str.push_back(symbol);
  }
  return in;
}

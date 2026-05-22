#include <utility> // for std::move
// Constructors
template <typename T> DynamicArray<T>::DynamicArray() { // default constructor
  size = 0;
  capacity = 0;
  data = nullptr;
}

template <typename T> DynamicArray<T>::~DynamicArray() { // destructor
  for (size_t i = 0; i < size; i++)
    data[i].~T(); // explicitly destroy live elements
  ::operator delete(data); // free raw memory (safe if nullptr)
}

template <typename T>
DynamicArray<T>::DynamicArray(const DynamicArray &other) { // copy constructor
  size = other.size;
  capacity = other.size; // only allocate what's needed

  if (capacity == 0) {
      data = nullptr;
      return;
  }

  data = static_cast<T*>(::operator new(capacity * sizeof(T)));
  for (size_t i = 0; i < size; i++) {
    new (&data[i]) T(other.data[i]); // copy-construct each element
  }
}

template <typename T>
DynamicArray<T>::DynamicArray(DynamicArray&& other) noexcept { // move constructor
  data = other.data;
  size = other.size;
  capacity = other.capacity;
  other.data = nullptr;
  other.size = 0;
  other.capacity = 0;
}

// Assigners
template <typename T>
DynamicArray<T>& DynamicArray<T>::operator=(const DynamicArray& other) { // copy assignment
  if (this == &other) return *this;
  for (size_t i = 0; i < size; i++) data[i].~T();
  ::operator delete(data);
  size = other.size;
  capacity = other.size;
  if (capacity == 0) { data = nullptr; return *this; }
  data = static_cast<T*>(::operator new(capacity * sizeof(T)));
  for (size_t i = 0; i < size; i++)
    new (&data[i]) T(other.data[i]);
  return *this;
}

template <typename T>
DynamicArray<T>& DynamicArray<T>::operator=(DynamicArray&& other) noexcept { // move assignment
  if (this == &other) return *this;
  for (size_t i = 0; i < size; i++) data[i].~T();
  ::operator delete(data);
  data = other.data;
  size = other.size;
  capacity = other.capacity;
  other.data = nullptr;
  other.size = 0;
  other.capacity = 0;
  return *this;
}

// Access and Modifiers
template <typename T>
void DynamicArray<T>::resize(size_t new_capacity) {
  if (new_capacity == 0) {
    for (size_t i = 0; i < size; i++) data[i].~T(); // destroy elements first
    ::operator delete(data); 
    data = nullptr;
    capacity = 0;
    size = 0;
    return;
  }
  T* new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
  for (size_t i = 0; i < size; i++) {
    new (&new_data[i]) T(std::move(data[i]));
    data[i].~T();
  }
  ::operator delete(data);
  data = new_data;
  capacity = new_capacity;
}

template <typename T>
void DynamicArray<T>::push_back(const T& value) {
  if (size == capacity)
    resize(capacity == 0 ? 1 : capacity * 2);
  new (&data[size++]) T(value); // placement copy-construct
}

template <typename T>
void DynamicArray<T>::push_back(T&& value) { // move overload
  if (size == capacity)
    resize(capacity == 0 ? 1 : capacity * 2);
  new (&data[size++]) T(std::move(value)); // placement move-construct
}

template <typename T>
void DynamicArray<T>::pop_back() {
  if (size > 0) {
    data[--size].~T(); // explicitly destroy the removed element
  }
}

template <typename T>
T &DynamicArray<T>::operator[](size_t index) { // edit value with []
  return data[index];
}

template <typename T>
const T &DynamicArray<T>::operator[](
    size_t index) const { // get value with [], don't change it
  return data[index];
}

template <typename T> size_t DynamicArray<T>::get_size() const { // get size;
  return size;
}

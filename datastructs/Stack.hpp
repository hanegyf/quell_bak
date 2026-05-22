#ifndef STACK_HPP
#define STACK_HPP

#include "DynamicArray.hpp"

template <typename T> class Stack {
private:
  DynamicArray<T> data; // internal data as Dynamic Array

public:
  Stack(); // default constructor

  void push(const T &value); // push to top
  void pop();                // get rid of top

  T &peek();            // peek at top value (mutable)
  const T &peek() const; // peek at top value (const)

  bool empty() const;  // is empty or not
  size_t size() const; // get size

  // Allow indexing (0 = bottom, size-1 = top)
  T &at(size_t index);
  const T &at(size_t index) const;
};

#include "Stack.tpp"

#endif

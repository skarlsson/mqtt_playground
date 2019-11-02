#include <iostream>
#include <exception>
#include <cassert>
#include <vector>
#include <initializer_list>
#include <type_traits>

#pragma once

// TODO random code from internet - redo this if we go forward

template<class T>
class circular_buffer {
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using size_type = size_t;
  using circularBuffer = std::vector<value_type>;

  circularBuffer m_array;
  size_type m_head;
  size_type m_tail;
  size_type m_contents_size;
  const size_type m_array_size;
public:

  circular_buffer(size_type size = 8) : m_array(size), m_array_size(size), m_head(1), m_tail(0), m_contents_size(0) {
    assert(m_array_size > 1 && "size must be greater than 1");
  }

  circular_buffer(std::initializer_list <T> l) : m_array(l), m_array_size(l.size()), m_head(0), m_tail(l.size() - 1), m_contents_size(
      l.size()) {
    assert(m_array_size > 1 && "size must be greater than 1");
  }

  reference front() { return m_array[m_head]; }

  reference top() { return front(); }

  reference back() { return m_array[m_tail]; }

  const_reference front() const { return m_array[m_head]; }

  const_reference back() const { return m_array[m_tail]; }

  void clear();

  void push_back(const value_type &item);

  void push(const value_type &item) { push_back(item); }

  void pop_front() { increment_head(); }

  void pop() { pop_front(); }

  size_type size() const { return m_contents_size; }

  size_type capacity() const { return m_array_size; }

  bool empty() const;

  bool full() const;

  size_type max_size() const { return size_type(-1) / sizeof(value_type); }

  reference operator[](size_type index);

  const_reference operator[](size_type index) const;

  reference at(size_type index);

  const_reference at(size_type index) const;

  struct my_iterator;
  using const_iterator = my_iterator;

  const_iterator begin() const;

  const_iterator cbegin() const;

  const_iterator rbegin() const;

  const_iterator end() const;

  const_iterator cend() const;

  const_iterator rend() const;

private:
  void increment_tail();

  void increment_head();

public:
  struct my_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = long long;
    using reference = T const &;
    using pointer = T const *;
    using vec_pointer = std::vector < T> const *;
  private:
    vec_pointer ptrToBuffer;
    size_type offset;
    size_type index;
    bool reverse;

    bool comparable(const my_iterator &other) {
      return (reverse == other.reverse);
    }

  public:
    my_iterator() : ptrToBuffer(nullptr), offset(0), index(0), reverse(false) {}  //

    reference operator*() {
      if (reverse)
        return (*ptrToBuffer)[(ptrToBuffer->size() + offset - index) % (ptrToBuffer->size())];
      return (*ptrToBuffer)[(offset + index) % (ptrToBuffer->size())];
    }

    reference operator[](size_type index) {
      my_iterator iter = *this;
      iter.index += index;
      return *iter;
    }

    pointer operator->() { return &(operator*()); }

    my_iterator &operator++() {
      ++index;
      return *this;
    };

    my_iterator operator++(int) {
      my_iterator iter = *this;
      ++index;
      return iter;
    }

    my_iterator &operator--() {
      --index;
      return *this;
    }

    my_iterator operator--(int) {
      my_iterator iter = *this;
      --index;
      return iter;
    }

    friend my_iterator operator+(my_iterator lhs, int rhs) {
      lhs.index += rhs;
      return lhs;
    }

    friend my_iterator operator+(int lhs, my_iterator rhs) {
      rhs.index += lhs;
      return rhs;
    }

    my_iterator &operator+=(int n) {
      index += n;
      return *this;
    }

    friend my_iterator operator-(my_iterator lhs, int rhs) {
      lhs.index -= rhs;
      return lhs;
    }

    friend difference_type operator-(const my_iterator &lhs, const my_iterator &rhs) {
      lhs.index -= rhs;
      return lhs.index - rhs.index;
    }

    my_iterator &operator-=(int n) {
      index -= n;
      return *this;
    }

    bool operator==(const my_iterator &other) {
      if (comparable(other))
        return (index + offset == other.index + other.offset);
      return false;
    }

    bool operator!=(const my_iterator &other) {
      if (comparable(other)) return !this->operator==(other);
      return true;
    }

    bool operator<(const my_iterator &other) {
      if (comparable(other))
        return (index + offset < other.index + other.offset);
      return false;
    }

    bool operator<=(const my_iterator &other) {
      if (comparable(other))
        return (index + offset <= other.index + other.offset);
      return false;
    }

    bool operator>(const my_iterator &other) {
      if (comparable(other)) return !this->operator<=(other);
      return false;
    }

    bool operator>=(const my_iterator &other) {
      if (comparable(other)) return !this->operator<(other);
      return false;
    }

    friend class circular_buffer<T>;
  };
};

template<class T>
void circular_buffer<T>::push_back(const value_type &item) {
  increment_tail();
  if (m_contents_size > m_array_size) increment_head(); // > full, == comma
  m_array[m_tail] = item;
}

template<class T>
void circular_buffer<T>::clear() {
  m_head = 1;
  m_tail = m_contents_size = 0;
}

template<class T>
bool circular_buffer<T>::empty() const {
  if (m_contents_size == 0) return true;
  return false;
}

template<class T>
inline bool circular_buffer<T>::full() const {
  if (m_contents_size == m_array_size) return true;
  return false;
}

template<class T>
typename circular_buffer<T>::const_reference circular_buffer<T>::operator[](size_type index) const {
  index += m_head;
  index %= m_array_size;
  return m_array[index];
}

template<class T>
typename circular_buffer<T>::reference circular_buffer<T>::operator[](size_type index) {
  const circular_buffer <T> &constMe = *this;
  return const_cast<reference>(constMe.operator[](index));
  //  return const_cast<reference>(static_cast<const ring<T>&>(*this)[index]);
}
//*/

template<class T>
typename circular_buffer<T>::reference circular_buffer<T>::at(size_type index) {
  if (index < m_contents_size) return this->operator[](index);
  throw std::out_of_range("index too large");
}

template<class T>
typename circular_buffer<T>::const_reference circular_buffer<T>::at(size_type index) const {
  if (index < m_contents_size) return this->operator[](index);
  throw std::out_of_range("index too large");
}

template<class T>
typename circular_buffer<T>::const_iterator circular_buffer<T>::begin() const {
  const_iterator iter;
  iter.ptrToBuffer = &m_array;
  iter.offset = m_head;
  iter.index = 0;
  iter.reverse = false;
  return iter;
}

template<class T>
typename circular_buffer<T>::const_iterator circular_buffer<T>::cbegin() const {
  const_iterator iter;
  iter.ptrToBuffer = &m_array;
  iter.offset = m_head;
  iter.index = 0;
  iter.reverse = false;
  return iter;
}

template<class T>
typename circular_buffer<T>::const_iterator circular_buffer<T>::rbegin() const {
  const_iterator iter;
  iter.ptrToBuffer = &m_array;
  iter.offset = m_tail;
  iter.index = 0;
  iter.reverse = true;
  return iter;
}

template<class T>
typename circular_buffer<T>::const_iterator circular_buffer<T>::end() const {
  const_iterator iter;
  iter.ptrToBuffer = &m_array;
  iter.offset = m_head;
  iter.index = m_contents_size;
  iter.reverse = false;
  return iter;
}

template<class T>
typename circular_buffer<T>::const_iterator circular_buffer<T>::cend() const {
  const_iterator iter;
  iter.ptrToBuffer = &m_array;
  iter.offset = m_head;
  iter.index = m_contents_size;
  iter.reverse = false;
  return iter;
}

template<class T>
typename circular_buffer<T>::const_iterator circular_buffer<T>::rend() const {
  const_iterator iter;
  iter.ptrToBuffer = &m_array;
  iter.offset = m_tail;
  iter.index = m_contents_size;
  iter.reverse = true;
  return iter;
}

template<class T>
void circular_buffer<T>::increment_tail() {
  ++m_tail;
  ++m_contents_size;
  if (m_tail == m_array_size) m_tail = 0;
}

template<class T>
void circular_buffer<T>::increment_head() {
  if (m_contents_size == 0) return;
  ++m_head;
  --m_contents_size;
  if (m_head == m_array_size) m_head = 0;
}

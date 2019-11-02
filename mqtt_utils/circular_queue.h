#include <iostream>
#include <exception>
#include <cassert>
#include <vector>
#include <initializer_list>
#include <type_traits>
#include <memory>
#include "spinlock.h"

#pragma once

namespace mqtt {
  template<class T>
  class circular_queue {
  public:
    circular_queue(size_t size = 8)
        : array_(size)
          , head_(1)
          , tail_(0)
          , contents_size_(0)
          , array_size_(size){
      assert(array_size_ > 1 && "size must be greater than 1");
    }

    void clear() {
      spinlock::scoped_lock xxx(lock_);
      for (auto &i : array_)
        i.clear();
      head_ = 1;
      tail_ = 0;
      contents_size_ = 0;
    }

    void push_back(std::unique_ptr<T> item) {
      spinlock::scoped_lock xxx(lock_);
      increment_tail();
      if (contents_size_ > array_size_)
        increment_head();
      array_[tail_] = std::move(item);
    }

    std::unique_ptr<T> get_front() {
      spinlock::scoped_lock xxx(lock_);
      if (!array_[head_])
        return nullptr;
      auto p = std::move(array_[head_]);
      increment_head();
      return p;
    }

    void pop_front() {
      get_front(); // note no return - object is cleared
    }

    size_t size() const {
      spinlock::scoped_lock xxx(lock_);
      return contents_size_;
    }

    size_t capacity() const {
      return array_size_;
    }

    bool empty() const {
      spinlock::scoped_lock xxx(lock_);
      return (contents_size_ == 0);
    }

    inline bool full() const {
      spinlock::scoped_lock xxx(lock_);
      return (contents_size_ == array_size_);
    }

  private:
    void increment_tail() {
      ++tail_;
      ++contents_size_;
      if (tail_ == array_size_)
        tail_ = 0;
    }

    void increment_head() {
      if (contents_size_ == 0)
        return;
      ++head_;
      --contents_size_;
      if (head_ == array_size_)
        head_ = 0;
    }

    mutable spinlock lock_;
    std::vector<std::unique_ptr<T>> array_;
    size_t head_;
    size_t tail_;
    size_t contents_size_;
    const size_t array_size_;
  };
}
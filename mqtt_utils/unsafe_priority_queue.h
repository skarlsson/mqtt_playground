#include <memory>
#include "circular_queue.h"
#include <mqtt/thread_queue.h>
#include "mqtt_defs.h"
#pragma once

namespace mqtt {
  template <class MSG>
  class unsafe_priority_queue {
  public:
    unsafe_priority_queue(size_t nr_of_priorities, size_t capacity)
    : nr_of_priorities_(nr_of_priorities) {
        queues_.reserve(nr_of_priorities);
        lost_.reserve(nr_of_priorities);
        for(size_t i=0; i!=nr_of_priorities; ++i) {
          queues_.push_back(std::move(std::make_unique<sub_queue>(capacity)));
          lost_.push_back(0);
          total_.push_back(0);
        }
    }

    void push_back(size_t prio, std::unique_ptr<MSG> message) {
      if (queues_[prio]->full())
        ++lost_[prio];
      ++total_[prio];
      queues_[prio]->push_back(std::move(message));
    }

    std::unique_ptr<MSG> get_next(){
      for(size_t i=0; i!=nr_of_priorities_; ++i)
        if (queues_[i]->size())
          return queues_[i]->get_front();
      return nullptr;
    }

    std::vector<size_t> get_loss() const {
      return lost_;
    }

    std::vector<size_t> get_total() const {
      return total_;
    }

    class sub_queue {
    public:
      sub_queue(size_t capacity)
      : buf_(capacity){
      }

      size_t size() const{
        return buf_.size();
      }

      bool empty() const{
        return buf_.empty();
      }

      bool full() const{
        return buf_.full();
      }

      std::unique_ptr<MSG> get_front() {
        return buf_.get_front();
      }

      void push_back(std::unique_ptr<MSG> message) {
        buf_.push_back(std::move(message));
      }

      circular_queue<MSG> buf_;
    };

  private:
    std::vector<std::unique_ptr<sub_queue>> queues_;
    std::vector<size_t> total_;
    std::vector<size_t> lost_;
    const size_t nr_of_priorities_;
  };
}
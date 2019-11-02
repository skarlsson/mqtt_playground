#include <chrono>
#include <atomic>
#include <thread>
#include <mqtt/async_client.h>
#include "unsafe_priority_queue.h"
#pragma once

namespace mqtt{

  class mqtt_handler;

  class action_listener : public virtual mqtt::iaction_listener
  {
  public:
    action_listener(class mqtt_handler* handler);
  protected:
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;
  private:
    mqtt_handler* handler_=nullptr;
  };

  class mqtt_handler{
  public:
    mqtt_handler(mqtt::async_client* client, size_t priority_levels, size_t capacity);
    ~mqtt_handler();
    void close();

    void push_back(size_t prio, std::unique_ptr<message2> message) {
      queue_.push_back(prio, std::move(message));
    }

    void init();
    void pop_pending_tx();
    std::vector<size_t> get_loss();
    std::vector<size_t> get_total();

  private:
    void thread();
    bool start_=false;
    bool exit_=false;
    unsafe_priority_queue<message2> queue_;
    mqtt::async_client* client_=nullptr;
    action_listener action_listener_;
    std::atomic<size_t> tx_unacked_bytes_;
    mqtt::thread_queue<size_t> pending_tx_;
    std::thread thread_;
  };
}

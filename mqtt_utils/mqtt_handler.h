#include <chrono>
#include <atomic>
#include <thread>
#include <mqtt/async_client.h>
#include "unsafe_priority_queue.h"
#pragma once

namespace mqtt{

  class mqtt_handler;

  //class action_listener : public virtual mqtt::callback, public virtual mqtt::iaction_listener


  class mqtt_handler{
  public:
    mqtt_handler(mqtt::async_client* client, size_t priority_levels, size_t capacity);
    ~mqtt_handler();
    void close();

    void push_back(size_t prio, std::unique_ptr<message2> message) {
      queue_.push_back(prio, std::move(message));
    }

    void init();

    std::vector<size_t> get_loss();
    std::vector<size_t> get_total();

  private:
    class publish_listener : public virtual mqtt::iaction_listener{
    public:
      publish_listener(class mqtt_handler* handler)
          : handler_(handler){
      }
      void on_success(const mqtt::token& tok) override;
      void on_failure(const mqtt::token& tok) override;
    private:
      mqtt_handler* handler_=nullptr;
    };

    class callback : public virtual mqtt::callback
    {
    public:
      callback(class mqtt_handler* handler);
    protected:
      // (Re)connection success
      void connected(const std::string& cause) override;

      // Callback for when the connection is lost.
      void connection_lost(const std::string& cause) override;

      // when a message arrives.
      void message_arrived(mqtt::const_message_ptr msg) override;

      // what does this one do?? - doe not seem to be called - we use publish listener instead
      void delivery_complete(mqtt::delivery_token_ptr token) override;

    private:
      mqtt_handler* handler_=nullptr;
    };

    void on_connected(); // from callback
    void on_connection_lost(); // from callback
    void on_publish_success(); // used from publish_listener

    void reconnect();
    void pop_pending_tx();

    void thread();
    bool start_=false;
    bool exit_=false;
    unsafe_priority_queue<message2> queue_;
    mqtt::async_client* client_=nullptr;
    callback action_listener_;
    publish_listener publish_listener_;

    std::atomic<size_t> tx_unacked_bytes_;
    mqtt::thread_queue<size_t> pending_tx_;
    std::thread thread_;
  };
}

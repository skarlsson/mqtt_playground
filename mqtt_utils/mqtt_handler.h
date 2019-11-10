#include <chrono>
#include <atomic>
#include <thread>
#include <functional>
#include <map>
#include <mqtt/async_client.h>
#include "unsafe_priority_queue.h"
#pragma once

namespace mqtt{

  class mqtt_handler;

  //class action_listener : public virtual mqtt::callback, public virtual mqtt::iaction_listener


  class mqtt_handler{
  public:
    typedef std::function<void(const std::string& topic, const std::string value)> on_message_callback;

    mqtt_handler(std::string mqtt_endpoint, mqtt::connect_options options, size_t priority_levels, size_t capacity);
    ~mqtt_handler();
    void close();

    void push_back(size_t prio, std::unique_ptr<message2> message) {
      queue_.push_back(prio, std::move(message));
    }

    void subscribe(std::string topic, on_message_callback cb){
      message_callbacks_[topic]=cb;
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
    void on_message_arrived(mqtt::const_message_ptr msg); // used from callback

    //void reconnect();
    void pop_pending_tx();

    void thread();
    bool start_=false;
    bool exit_=false;
    unsafe_priority_queue<message2> queue_;
    std::unique_ptr<mqtt::async_client> client_;
    std::string mqtt_endpoint_;
    mqtt::connect_options connect_options_;
    callback action_listener_;
    publish_listener publish_listener_;

    std::atomic<size_t> tx_unacked_bytes_;
    mqtt::thread_queue<size_t> pending_tx_;
    std::map<std::string, on_message_callback> message_callbacks_;
    std::thread thread_;
  };
}

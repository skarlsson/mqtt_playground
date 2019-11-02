#include <string>
#include <mqtt/async_client.h>
#include <mqtt/thread_queue.h>
#include <atomic>
#include "circular_buffer.h"
#pragma once

// use case
// "ftp" (no loss but chunked in "small" pieces to allow priority messages to pass before
// stream of different metrics over same topic
//https://spindance.com/2018/03/15/pattern-secure-uploads-downloads-aws-iot/

class mqtt_mux;

/*
class callback : public virtual mqtt::callback
{
public:
callback(mqtt_mux* mux);
void connection_lost(const std::string& cause) override;
void delivery_complete(mqtt::delivery_token_ptr tok) override;

private:
mqtt_mux* mux_;
};
*/


// Callbacks for the success or failures of requested actions.
// This could be used to initiate further action, but here we just log the


// results to the console.

class action_listener : public virtual mqtt::iaction_listener
{
public:
  action_listener(class mqtt_mux* mux);
protected:
  void on_failure(const mqtt::token& tok) override;
  void on_success(const mqtt::token& tok) override;
private:
  mqtt_mux* mux_;
};

class mqtt_mux{
public:
  class quable {
  public:
    virtual void pop_front()=0;
    virtual size_t front_size() const=0;
  };

  class circular_queue : public quable {
  public:
    circular_queue(std::string topic_name, size_t max_queue_size)
    : buf_(max_queue_size)
    , topic_name_(topic_name){
    }

    void push_back(mqtt::binary_ref payload){
      buf_.push_back(std::move(payload));
    }

    mqtt::binary_ref front(){
      return buf_.front();
    }

    size_t front_size() const override {
      return buf_.front().size();
    };


    void pop_front() override {
      buf_.pop_front();
    }

    inline bool empty() const {
      return buf_.empty();
    }

    std::string topic_name() const {
      return topic_name_;
    }

  private:
    std::string topic_name_;
    circular_buffer<mqtt::binary_ref> buf_;
    size_t outstanding_tx__=0;
    int priority_=7;
  };

  class chunked_transfer : public quable {
  public:
    struct chunk {
      const char* data=nullptr;
      size_t size=0;
      size_t ix=0;
    };

    enum { CHUNK_SIZE = 10000 };
    chunked_transfer(std::string topic_name, mqtt::binary_ref payload)
    : topic_name_(topic_name)
    , payload_(std::move(payload)){
    }

    void pop_front() override {
      cursor_ += front_size_;
      ++chunk_ix_;
      front_size_ = 0;
    }

    size_t front_size() const override {
      return front_size_;
    };

    chunk get_next_chunk(){
      front_size_ = std::min<size_t>(payload_.size() - cursor_,  CHUNK_SIZE);
      if (front_size_) {
        chunk data = { payload_.data() + cursor_, front_size_, chunk_ix_};
        return data;
      }
      return {nullptr, 0};
      }

    std::string topic_name() const {
      return topic_name_;
    }

  private:
    std::string topic_name_;
    mqtt::binary_ref payload_;
    std::atomic<size_t> chunk_ix_=0;
    std::atomic<size_t> cursor_=0;
    std::atomic<size_t> front_size_=0;  // handles only one outstanding send

  };


  mqtt_mux(mqtt::async_client* client);
  std::shared_ptr<circular_queue> create_topic(std::string topic_name, size_t max_queue_size);
  std::shared_ptr<chunked_transfer> create_stream(std::string topic_name, mqtt::binary_ref payload);

  void init();

  void pop_pending_tx();

private:
  void thread();

  bool start_=false;
  bool exit_=false;

  mqtt::async_client* client_;
  action_listener action_listener_;

  std::thread thread_;
  std::vector<std::shared_ptr<circular_queue>> queues_;
  std::deque<std::shared_ptr<chunked_transfer>> streams_;
  mqtt::thread_queue<quable*> pending_tx_;
  std::atomic<size_t> tx_unacked_bytes_=0;
};
#include "mqtt_handler.h"
#include <glog/logging.h>

#define KEEP_ALIVE_INTERVALL 1000
#define MAX_PAYLOAD_IN_FLIGHT 100

namespace mqtt{
  action_listener::action_listener(class mqtt_handler* handler)
      :handler_(handler){
  }
  void action_listener::on_failure(const mqtt::token& tok) {
    LOG(INFO) << "Listener failure for token: "   << tok.get_message_id();
  }

  void action_listener::on_success(const mqtt::token& tok) {
    LOG(INFO) << "Listener success for token: "   << tok.get_message_id();
    handler_->pop_pending_tx();
  }

  mqtt_handler::mqtt_handler(mqtt::async_client* client, size_t priority_levels, size_t capacity)
  : queue_(priority_levels, capacity)
  , client_(client)
  , action_listener_(this)
  , tx_unacked_bytes_(0)
    , thread_([this](){ thread(); }){
  }

  mqtt_handler::~mqtt_handler(){
    exit_=true;
    thread_.join();
    LOG(INFO) << "exiting hanbdler";
  }

  void mqtt_handler::pop_pending_tx(){
    assert(!pending_tx_.empty());
    auto message_size = pending_tx_.get();
    assert(message_size<=tx_unacked_bytes_);
    tx_unacked_bytes_  -= message_size;
    LOG(INFO) << "bytes in flight : " << tx_unacked_bytes_;
    //todo maybe tx?
  }

  void mqtt_handler::init() {
    start_ = true;
  }

  void mqtt_handler::close(){
    exit_ = true;
  }

  void mqtt_handler::thread(){
    while (!start_ && !exit_ ){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    while (!exit_ ) {
      LOG(INFO) << "wakeup";
      //do we have stuff todo??

      if (tx_unacked_bytes_ >= MAX_PAYLOAD_IN_FLIGHT) {
        LOG(INFO) << "sleeping on tx_unacked_bytes=" << tx_unacked_bytes_;
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        continue;
      }


      while (!exit_ && tx_unacked_bytes_ < MAX_PAYLOAD_IN_FLIGHT) {
        auto msg = queue_.get_next();
        if (msg) {
          LOG(INFO) << "sending key " << msg->key.c_str() << " value: "" << " << msg->val.c_str();
          std::string topic = "data/" + msg->key.to_string();
          auto pubmsg = mqtt::make_message(topic, msg->val);
          tx_unacked_bytes_ += msg->val.size();
          pending_tx_.put(msg->val.size()); // make sure we add it first if we get the callback before returning
          client_->publish(pubmsg, nullptr, action_listener_);
          continue;
        } else {
          // wait for new message - we should move this loop to a maybe send function that can be called from async callbackk
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      }
      //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }


      /*if (tx_unacked_bytes_ == 0 && streams_.size()) // only send batch data when idle
      {
        auto p = streams_.front();
        if (p->front_size()==0) { // no outstanding messages


          auto chunk = p->get_next_chunk();
          if (chunk.size) {
            LOG(INFO) << "sending chunk topic " << p->topic_name() << " size : "" << " << chunk.size;
            auto pubmsg = mqtt::make_message(p->topic_name()+".p" + std::to_string(chunk.ix), chunk.data, chunk.size);
            tx_unacked_bytes_ += chunk.size;
            pending_tx_.put(p.get()); // make sure we add it first if we get the callback before returning
            client_->publish(pubmsg, nullptr, action_listener_);
          } else {
            streams_[0] = nullptr;
            streams_.pop_front();
            LOG(INFO) << " transfer done, use count =" << p.use_count();
          }
        }
      }
       */

    if (tx_unacked_bytes_==0)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

  std::vector<size_t> mqtt_handler::get_loss(){
    return queue_.get_loss();
  }

  std::vector<size_t> mqtt_handler::get_total(){
    return queue_.get_total();
  }

}
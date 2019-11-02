#include "mqtt_mux.h"
#include <chrono>
#include <glog/logging.h>

using namespace std::chrono_literals;

#define KEEP_ALIVE_INTERVALL 1000
#define MAX_PAYLOAD_IN_FLIGHT 10

action_listener::action_listener(class mqtt_mux* mux)
    :mux_(mux){
}
void action_listener::on_failure(const mqtt::token& tok) {
  LOG(INFO) << "tListener failure for token: "   << tok.get_message_id();
}

void action_listener::on_success(const mqtt::token& tok) {
  LOG(INFO) << "Listener success for token: "   << tok.get_message_id();
  mux_->pop_pending_tx();
}

mqtt_mux::mqtt_mux(mqtt::async_client* client)
    : client_(client)
      , action_listener_(this)
      , thread_([this](){ thread(); }){
}

void mqtt_mux::pop_pending_tx(){
  assert(!pending_tx_.empty());
  auto p = pending_tx_.get();

  auto message_size = p->front_size();
  assert(message_size<=tx_unacked_bytes_);
  tx_unacked_bytes_  -= p->front_size();
  p->pop_front();
  LOG(INFO) << "bytes in flight : " << tx_unacked_bytes_;
}

void mqtt_mux::thread(){
  while (!start_ && !exit_ ){
    std::this_thread::sleep_for(100ms);;
  }

  while (!exit_ ){
    LOG(INFO) << "wakeup";
    //do we have stuff todo??

    if (tx_unacked_bytes_ >= MAX_PAYLOAD_IN_FLIGHT) {
      LOG(INFO) << "sleeping on tx_unacked_bytes=" << tx_unacked_bytes_;
      std::this_thread::sleep_for(10ms);
      continue;
    }

    for(auto& i : queues_){
      if (!i->empty()){
        auto msg = i->front();
        LOG(INFO) << "sending topic " << i->topic_name() << " value: "" << " << i->front();
        auto pubmsg = mqtt::make_message(i->topic_name(), msg);
        tx_unacked_bytes_ += msg.size();
        pending_tx_.put(i.get()); // make sure we add it first if we get the callback before returning
        client_->publish(pubmsg, nullptr, action_listener_);
        if (tx_unacked_bytes_ >= MAX_PAYLOAD_IN_FLIGHT) {
           continue;
        }
      }
    }

    if (tx_unacked_bytes_ == 0 && streams_.size()) // only send batch data when idle
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

      std::this_thread::sleep_for(100ms);
  }
}

std::shared_ptr<mqtt_mux::circular_queue> mqtt_mux::create_topic(std::string topic_name, size_t max_queue_size){
  auto p = std::make_shared<mqtt_mux::circular_queue>(topic_name, max_queue_size);
  queues_.push_back(p);
  return p;
}

std::shared_ptr<mqtt_mux::chunked_transfer> mqtt_mux::create_stream(std::string topic_name,  mqtt::binary_ref payload){
  auto p = std::make_shared<mqtt_mux::chunked_transfer>(topic_name, payload);
  streams_.push_back(p);
  return p;
}

void mqtt_mux::init() {
  start_ = true;
}
#include "mqtt_handler.h"
#include <glog/logging.h>
#include <mqtt_utils/mqtt_utils.h>

#define KEEP_ALIVE_INTERVALL 1000
#define MAX_PAYLOAD_IN_FLIGHT 100

namespace mqtt{
  void mqtt_handler::publish_listener::on_success(const mqtt::token& tok){
    LOG(INFO) << "publish_listener::on_success";
    handler_->on_publish_success();
  }

  void mqtt_handler::publish_listener::on_failure(const mqtt::token& tok){
    LOG(INFO) << "on_failure - do nothing...";
  }


  mqtt_handler::callback::callback(class mqtt_handler* handler)
      :handler_(handler){
  }

  void mqtt_handler::callback::connected(const std::string& cause){
    LOG(INFO) << "connected: "   << cause;
    handler_->on_connected();
  }

  void mqtt_handler::callback::connection_lost(const std::string& cause) {
    LOG(INFO) << "connection_lost: "   << cause;
    handler_->on_connection_lost();
  }

  // Callback for when a message arrives.
  void mqtt_handler::callback::message_arrived(mqtt::const_message_ptr msg) {
    LOG(INFO) << "\ttopic: '" << msg->get_topic() << "'" << "\tpayload: '" << msg->to_string() << ", rx_ts: " << mqtt::milliseconds_since_epoch();
}

// seems to never be called
void mqtt_handler::callback::delivery_complete(mqtt::delivery_token_ptr tok) {
  LOG(INFO) << "delivery_complete token: "   << tok->get_message_id();
  }

  mqtt_handler::mqtt_handler(mqtt::async_client* client, size_t priority_levels, size_t capacity)
  : queue_(priority_levels, capacity)
  , client_(client)
  , action_listener_(this)
  , publish_listener_(this)
  , tx_unacked_bytes_(0)
    , thread_([this](){ thread(); }){
    client_->set_callback(action_listener_);
  }

  mqtt_handler::~mqtt_handler(){
    exit_=true;
    thread_.join();
    LOG(INFO) << "exiting handler";
  }

  void mqtt_handler::pop_pending_tx(){
    assert(!pending_tx_.empty());
    auto message_size = pending_tx_.get();
    assert(message_size<=tx_unacked_bytes_);
    tx_unacked_bytes_  -= message_size;
    //LOG(INFO) << "bytes in flight : " << tx_unacked_bytes_;
    //todo maybe tx?
  }

  void mqtt_handler::init() {
    start_ = true;
  }

  void mqtt_handler::close(){
    exit_ = true;
  }

  void mqtt_handler::reconnect(){
    //client_->reconnect();
  }

  void mqtt_handler::on_connected(){
    LOG(INFO) << "connected";
    LOG(INFO) << "subscribing";
    //client_->subscribe("alarm", 1, nullptr, action_listener_); // can be another instance to separate subscriptions
    client_->subscribe("alarm", 1); // todo make tis an action listener (separate that knows if the subscripotion failes)
  }

  void mqtt_handler::on_connection_lost(){
    size_t count =pending_tx_.size();
    while(!pending_tx_.empty()) {
      auto message_size = pending_tx_.get();
      tx_unacked_bytes_  -= message_size;
    }
    LOG(INFO) << "connection lost - dropped " << count << " messages";
    //reconnect();
  }

  void  mqtt_handler::on_publish_success(){
    pop_pending_tx();
  }

  void mqtt_handler::thread(){
    while (!start_ && !exit_ ){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    while (!exit_ ) {
      if (!client_->is_connected()){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        size_t count =pending_tx_.size();
        while(!pending_tx_.empty()) {
          auto message_size = pending_tx_.get();
          tx_unacked_bytes_  -= message_size;
        }
        LOG(INFO) << "connection lost - dropped " << count << " messages";


        continue;
        /*LOG(INFO) << "connecting...";
        try {
          client_->reconnect()->wait();
          LOG(INFO) << "connection OK";
        } catch (std::exception& e){
          LOG(INFO) << "connect failed: " << e.what();
        }
        continue;
         */
      }

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
          auto pubmsg = mqtt::make_message(msg->key.to_string(), msg->val);
          tx_unacked_bytes_ += msg->val.size();
          pending_tx_.put(msg->val.size()); // make sure we add it first if we get the callback before returning
          //client_->publish(pubmsg, nullptr, action_listener_);
          client_->publish(pubmsg, nullptr, publish_listener_);
          //client_->publish(pubmsg);
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
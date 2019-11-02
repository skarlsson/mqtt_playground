#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <mqtt/async_client.h>
#include <mqtt_utils/mqtt_utils.h>
#include <glog/logging.h>

using namespace std;
using namespace std::chrono;

//const string TOPIC { "data/rand" };
//const int	 QOS = 1;
const auto PERIOD = seconds(5);
const int MAX_BUFFERED_MSGS = 120;	// 120 * 5sec => 10min off-line buffering
const string PERSIST_DIR { "data-persist" };
#define N_RETRY_ATTEMPTS 17

/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////

// Callbacks for the success or failures of requested actions.
// This could be used to initiate further action, but here we just log the
// results to the console.

class action_listener : public virtual mqtt::iaction_listener
{
  std::string name_;

  void on_failure(const mqtt::token& tok) override {
    std::cout << name_ << " failure";
    if (tok.get_message_id() != 0)
      std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
    std::cout << std::endl;
  }

  void on_success(const mqtt::token& tok) override {
    std::cout << name_ << " success";
    if (tok.get_message_id() != 0)
      std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
    auto top = tok.get_topics();
    if (top && !top->empty())
      std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
    std::cout << std::endl;
  }

public:
  action_listener(const std::string& name) : name_(name) {}
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
class callback : public virtual mqtt::callback,
    public virtual mqtt::iaction_listener

{
  // Counter for the number of connection retries
  int nretry_;
  // The MQTT client
  mqtt::async_client& cli_;
  // Options to use if we need to reconnect
  mqtt::connect_options& connOpts_;
  // An action listener to display the result of actions.
  action_listener subListener_;

  // This deomonstrates manually reconnecting to the broker by calling
  // connect() again. This is a possibility for an application that keeps
  // a copy of it's original connect_options, or if the app wants to
  // reconnect with different options.
  // Another way this can be done manually, if using the same options, is
  // to just call the async_client::reconnect() method.
  void reconnect() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    try {
      cli_.connect(connOpts_, nullptr, *this);
    }
    catch (const mqtt::exception& exc) {
      std::cerr << "Error: " << exc.what() << std::endl;
      exit(1);
    }
  }

  // Re-connection failure
  void on_failure(const mqtt::token& tok) override {
    std::cout << "Connection attempt failed" << std::endl;
    if (++nretry_ > N_RETRY_ATTEMPTS)
      exit(1);
    reconnect();
  }

  // (Re)connection success
  // Either this or connected() can be used for callbacks.
  void on_success(const mqtt::token& tok) override {}

  // (Re)connection success
  void connected(const std::string& cause) override {
    std::cout << "\nConnection success" << std::endl;
    /*std::cout << "\nSubscribing to topic '" << TOPIC << "'\n"
              << "\tfor client " << CLIENT_ID
              << " using QoS" << QOS << "\n"
              << "\nPress Q<Enter> to quit\n" << std::endl;
    */

    cli_.subscribe("#", 1, nullptr, subListener_);
  }

  // Callback for when the connection is lost.
  // This will initiate the attempt to manually reconnect.
  void connection_lost(const std::string& cause) override {
    std::cout << "\nConnection lost" << std::endl;
    if (!cause.empty())
      std::cout << "\tcause: " << cause << std::endl;

    std::cout << "Reconnecting..." << std::endl;
    nretry_ = 0;
    reconnect();
  }

  // Callback for when a message arrives.
  void message_arrived(mqtt::const_message_ptr msg) override {
    LOG(INFO) << "\ttopic: '" << msg->get_topic() << "'" << "\tpayload: '" << msg->to_string() << ", rx_ts: " << mqtt::milliseconds_since_epoch();
  }

  void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
  callback(mqtt::async_client& cli, mqtt::connect_options& connOpts)
      : nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {}
};






int main(int argc, char* argv[])
{
  char* mqtt_endpoint = getenv ("BB_MQTT_ENDPOINT");
  if (mqtt_endpoint==nullptr){
    std::cerr << "BB_MQTT_ENDPOINT noot defined - exiting" << std::endl;
    exit(-1);
  }

  char* mqtt_username = getenv ("BB_MQTT_USERNAME");
  if (mqtt_username==nullptr){
    std::cerr << "BB_MQTT_USERNAME noot defined - exiting" << std::endl;
    exit(-1);
  }

  char* mqtt_password = getenv ("BB_MQTT_PASSWORD");
  if (mqtt_password==nullptr){
    std::cerr << "BB_MQTT_PASSWORD noot defined - exiting" << std::endl;
    exit(-1);
  }

  LOG(INFO) << "BB_MQTT_ENDPOINT" << mqtt_endpoint;
  LOG(INFO) << "BB_MQTT_USERNAME" << mqtt_username;
  //LOG(INFO) << "BB_MQTT_ENDPOINT" << mqtt_password;

  mqtt::async_client cli(mqtt_endpoint, "", MAX_BUFFERED_MSGS, PERSIST_DIR);
  mqtt::connect_options connOpts(mqtt_username, mqtt_password);
  connOpts.set_mqtt_version(MQTTVERSION_3_1_1);
  connOpts.set_keep_alive_interval(MAX_BUFFERED_MSGS * PERIOD);
  connOpts.set_clean_session(true);
  connOpts.set_automatic_reconnect(true);

  mqtt::ssl_options sslopts;
  sslopts.set_trust_store("/etc/ssl/certs/ca-certificates.crt");
  connOpts.set_ssl(sslopts);

  callback cb(cli, connOpts);
  cli.set_callback(cb);

	try {
    LOG(INFO) << "Connecting to server '" << mqtt_endpoint << "'...";
    cli.connect(connOpts)->wait();
    LOG(INFO) << "OK";

	  while(true){
	    std::this_thread::sleep_for(100ms);
	  }
	}
	catch (const mqtt::exception& exc) {
		cerr << exc.what() << endl;
		return 1;
	}

 	return 0;
}

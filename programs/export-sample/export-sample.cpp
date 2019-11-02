#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <mqtt_utils/mqtt_utils.h>
#include <mqtt_utils/mqtt_handler.h>
#include <glog/logging.h>

using namespace std;
using namespace std::chrono;

//const string TOPIC { "data/rand" };
//const int	 QOS = 1;
const auto PERIOD = seconds(5);
const int MAX_BUFFERED_MSGS = 120;	// 120 * 5sec => 10min off-line buffering
const string PERSIST_DIR { "data-persist" };

/////////////////////////////////////////////////////////////////////////////

struct simulated_data {
  std::string topic;
  std::string name;
  int64_t next_tx=0;
  int64_t interval=1000;
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

	// Random number generator [0 - 100]
	random_device rnd;
    mt19937 gen(rnd());
    uniform_int_distribution<> dis(0, 100);

	try {
    LOG(INFO) << "Connecting to server '" << mqtt_endpoint << "'...";
    cli.connect(connOpts)->wait();
    LOG(INFO) << "OK";

    mqtt::mqtt_handler handler(&cli, 8, 200);
    handler.init();

    std::vector<simulated_data> next_tx;
    next_tx.push_back({ "alarm", "s0", 0, 1000});
    next_tx.push_back({ "dd", "s1", 0, 10});
    next_tx.push_back({ "dd", "msg2", 0, 10});
    next_tx.push_back({ "dd", "msg3", 0, 10});

    while(true){
      auto now = mqtt::milliseconds_since_epoch();
      int prio=0;
      for (std::vector<simulated_data>::iterator i=next_tx.begin();i!=next_tx.end(); ++i, ++prio){
        if (i->next_tx<=now) {
          auto payload = std::make_shared<const std::string>("{ \"value\": \"whatever\", ts: " + std::to_string(now) + "}");
          auto msg = std::make_unique<mqtt::message2>(i->topic + "/" + i->name, payload);
          handler.push_back(prio, std::move(msg));
          i->next_tx = now + i->interval;
        }
      }
      std::this_thread::sleep_for(10ms);
    }

		// Disconnect
		cout << "\nDisconnecting..." << flush;
		cli.disconnect()->wait();
		cout << "OK" << endl;
	}
	catch (const mqtt::exception& exc) {
		cerr << exc.what() << endl;
		return 1;
	}

 	return 0;
}

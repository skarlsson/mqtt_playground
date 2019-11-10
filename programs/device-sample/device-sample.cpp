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
const int MAX_BUFFERED_MSGS = 120;  // 120 * 5sec => 10min off-line buffering
const string PERSIST_DIR{"data-persist"};

/////////////////////////////////////////////////////////////////////////////

struct simulated_data {
  std::string topic;
  std::string name;
  std::string value;
  int64_t next_tx = 0;
  int64_t interval = 1000;
};

int main(int argc, char *argv[]) {
  char *mqtt_endpoint = getenv("BB_MQTT_ENDPOINT");
  if (mqtt_endpoint == nullptr) {
    std::cerr << "BB_MQTT_ENDPOINT noot defined - exiting" << std::endl;
    exit(-1);
  }

  char *mqtt_username = getenv("BB_MQTT_USERNAME");
  if (mqtt_username == nullptr) {
    std::cerr << "BB_MQTT_USERNAME noot defined - exiting" << std::endl;
    exit(-1);
  }

  char *mqtt_password = getenv("BB_MQTT_PASSWORD");
  if (mqtt_password == nullptr) {
    std::cerr << "BB_MQTT_PASSWORD noot defined - exiting" << std::endl;
    exit(-1);
  }

  LOG(INFO) << "BB_MQTT_ENDPOINT" << mqtt_endpoint;
  LOG(INFO) << "BB_MQTT_USERNAME" << mqtt_username;
  //LOG(INFO) << "BB_MQTT_ENDPOINT" << mqtt_password;

  mqtt::connect_options connOpts(mqtt_username, mqtt_password);
  connOpts.set_mqtt_version(MQTTVERSION_3_1_1);
  connOpts.set_keep_alive_interval(MAX_BUFFERED_MSGS * PERIOD);
  connOpts.set_clean_session(true);
  connOpts.set_automatic_reconnect(true);

  mqtt::ssl_options sslopts;
  sslopts.set_trust_store("/etc/ssl/certs/ca-certificates.crt");
  connOpts.set_ssl(sslopts);

  try {
    mqtt::mqtt_handler handler(mqtt_endpoint, connOpts, 8, 200);
    handler.subscribe("alarm", [](auto topic, auto payload){
      LOG(INFO) << "got alarm " << payload;
    });

    handler.init();

    std::vector<simulated_data> next_tx;
    next_tx.push_back({"alarm", "mob", "{\"lat\": 59.334591, \"lng\": 18.063240}", 0, 10000});
    next_tx.push_back({ "m", "EngineSpeed", "1843",0, 1000});
    next_tx.push_back({ "m", "pos", "{\"lat\": 59.334591, \"lng\": 18.063240}",0, 1000});
    next_tx.push_back({ "m", "RudderPosition", "2.0",0, 1000});


    while (true) {
      auto now = mqtt::milliseconds_since_epoch();
      int prio = 0;
      for (std::vector<simulated_data>::iterator i = next_tx.begin(); i != next_tx.end(); ++i, ++prio) {
        if (i->next_tx <= now) {
          auto payload = std::make_shared<const std::string>(
              "{ \"" + i->name + "\": " + i->value + ", \"ts\" : " + std::to_string(now) + "}");
          auto msg = std::make_unique<mqtt::message2>(i->topic, payload);
          handler.push_back(prio, std::move(msg));
          i->next_tx = now + i->interval;
        }
      }
      std::this_thread::sleep_for(10ms);
    }

    // Disconnect
    cout << "\nDisconnecting..." << flush;
    handler.close();
    //cli.disconnect()->wait();
    cout << "OK" << endl;
  }
  catch (const mqtt::exception &exc) {
    cerr << exc.what() << endl;
    return 1;
  }

  return 0;
}

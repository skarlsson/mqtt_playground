#include <mqtt/message.h>
namespace mqtt {
  class message2 {
  public:
    message2(mqtt::binary_ref k, mqtt::binary_ref v)
    : key(k)
    , val(v) {
    }

    mqtt::binary_ref key;
    mqtt::binary_ref val;
  };
}

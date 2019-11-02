set -ef 

export CPP_STANDARD="17"
export PROTOBUF_VER="3.7.0"
export PAHO_MQTT_C_VER="1.3.1"
export PAHO_MQTT_CPP_VER="1.0.1"


rm -rf tmp
mkdir tmp && cd tmp

wget -O protobuf.tar.gz "https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOBUF_VER/protobuf-cpp-$PROTOBUF_VER.tar.gz" && \
mkdir -p protobuf && \
tar \
  --extract \
  --file protobuf.tar.gz \
  --directory protobuf \
  --strip-components 1 && \
cd protobuf && \
./configure && \
make -j "$(getconf _NPROCESSORS_ONLN)" && \
sudo make install && \
cd .. && \
rm protobuf.tar.gz && \
rm -rf protobuf

wget -O paho.mqtt.c.tar.gz "https://github.com/eclipse/paho.mqtt.c/archive/v$PAHO_MQTT_C_VER.tar.gz" && \
mkdir -p paho.mqtt.c
tar \
  --extract \
  --file paho.mqtt.c.tar.gz \
  --directory paho.mqtt.c \
  --strip-components 1
cd paho.mqtt.c
mkdir build && cd build 
cmake -DPAHO_WITH_SSL=ON -DPAHO_ENABLE_TESTING=OFF ..
make -j "$(getconf _NPROCESSORS_ONLN)"
sudo make install
cd ../.. 
rm paho.mqtt.c.tar.gz
rm -rf paho.mqtt.c

wget -O paho.mqtt.cpp.tar.gz "https://github.com/eclipse/paho.mqtt.cpp/archive/v$PAHO_MQTT_CPP_VER.tar.gz" && \
mkdir -p paho.mqtt.cpp
tar \
  --extract \
  --file paho.mqtt.cpp.tar.gz \
  --directory paho.mqtt.cpp \
  --strip-components 1
cd paho.mqtt.cpp
mkdir build && cd build 
cmake -DPAHO_WITH_SSL=ON ..
make -j "$(getconf _NPROCESSORS_ONLN)"
sudo make install
cd ../.. 
rm paho.mqtt.cpp.tar.gz
rm -rf paho.mqtt.cpp

#out of tmp
cd ..
rm -rf tmp



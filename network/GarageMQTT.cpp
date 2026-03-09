#include "GarageMQTT.h"
#include <cstring>
#include <iostream>
#include <utility>

using namespace std;

GarageMQTT* GarageMQTT::instance_ = nullptr;

GarageMQTT::GarageMQTT(const string& ssid,
                       const string& password,
                       string brokerIp,
                       int port,
                       string clientId)
    : ssid_(ssid),
      password_(password),
      brokerIp_(std::move(brokerIp)),
      port_(port),
      clientId_(std::move(clientId)),
      ipstack_(ssid.c_str(), password.c_str()),
      client_(ipstack_),
      commandAvailable_(false),
      remoteDisabled_(false)
{
    instance_ = this;
    connectData_ = MQTTPacket_connectData_initializer;
    connectData_.MQTTVersion = 3;
    connectData_.clientID.cstring = const_cast<char*>(clientId_.c_str());
}

bool GarageMQTT::connectTcp()
{
    cout << "Connecting TCP to " << brokerIp_ << ":" << port_ << endl;
    int rc = ipstack_.connect(brokerIp_.c_str(), port_);
    if (rc != 0)
    {
        cout << "TCP connection failed, rc=" << rc << endl;
        return false;
    }
    cout << "TCP connected" << endl;
    return true;
}

bool GarageMQTT::connectMqtt()
{
    cout << "Connecting MQTT..." << endl;
    int rc = client_.connect(connectData_);
    if (rc != 0)
    {
        cout << "MQTT connection failed, rc=" << rc << endl;
        return false;
    }
    cout << "MQTT connected" << endl;
    return true;
}

bool GarageMQTT::subscribeCommandTopic()
{
    int rc = client_.subscribe(COMMAND_TOPIC, MQTT::QOS0, messageArrived);
    if (rc != 0)
    {
        cout << "Subscribe failed, rc=" << rc << endl;
        return false;
    }
    cout << "Subscribed to " << COMMAND_TOPIC << endl;
    return true;
}

bool GarageMQTT::connect()
{
    int retry = 0;

    while (retry < MAX_RECONNECT_ATTEMPTS)
    {
        cout << "MQTT connect attempt " << retry + 1 << "/" << MAX_RECONNECT_ATTEMPTS << endl;

        if (!connectTcp())
        {
            retry++;
            tight_loop_contents();
            continue;
        }

        if (!connectMqtt())
        {
            retry++;
            tight_loop_contents();
            continue;
        }

        if (!subscribeCommandTopic())
        {
            retry++;
            tight_loop_contents();
            continue;
        }

        cout << "Remote control ready." << endl;
        cout << "Listening for commands on topic: garage/door/command" << endl;
        cout << "Available commands: OPEN, CLOSE, STOP, CALIBRATE, STATUS, TOGGLE" << endl;
        remoteDisabled_ = false;
        return true;
    }

    cout << "MQTT connection failed after " << MAX_RECONNECT_ATTEMPTS << " attempts" << endl;
    cout << "Device will continue in local-only mode." << endl;
    remoteDisabled_ = true;
    return false;
}

bool GarageMQTT::publishMessage(const char* topic, const char* payload)
{
    if (remoteDisabled_ || payload == nullptr)
    {
        return false;
    }

    MQTT::Message message{};
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)payload;
    message.payloadlen = strlen(payload);

    int rc = client_.publish(topic, message);
    if (rc != 0)
    {
        cout << "Publish failed rc = " << rc << endl;
        return false;
    }

    cout << "Published to " << topic << ": " << payload << endl;
    return true;
}

bool GarageMQTT::publishStatus(const char* payload)
{
    return publishMessage(STATUS_TOPIC, payload);
}

bool GarageMQTT::publishResponse(const char* payload)
{
    return publishMessage(RESPONSE_TOPIC, payload);
}

void GarageMQTT::loop()
{
    if (remoteDisabled_)
    {
        tight_loop_contents();
        return;
    }

    client_.yield(1);
    tight_loop_contents();
}

bool GarageMQTT::hasPendingCommand() const
{
    if (remoteDisabled_)
    {
        return false;
    }
    return commandAvailable_;
}

bool GarageMQTT::popPendingCommand(char* buffer, size_t size)
{
    if (!commandAvailable_ || buffer == nullptr || size == 0)
    {
        return false;
    }

    strncpy(buffer, commandBuffer_, size - 1);
    buffer[size - 1] = '\0';

    commandAvailable_ = false;
    commandBuffer_[0] = '\0';
    return true;
}

void GarageMQTT::messageArrived(MQTT::MessageData& md)
{
    if (instance_ == nullptr)
    {
        return;
    }

    MQTT::Message& message = md.message;

    if (message.retained)
    {
        cout << "Ignoring retained MQTT command." << endl;
        return;
    }

    size_t len = message.payloadlen;
    if (len >= sizeof(instance_->commandBuffer_))
    {
        len = sizeof(instance_->commandBuffer_) - 1;
    }

    memcpy(instance_->commandBuffer_, message.payload, len);
    instance_->commandBuffer_[len] = '\0';
    instance_->commandAvailable_ = true;

    cout << "MQTT command received: [" << instance_->commandBuffer_ << "], retained="
         << message.retained << endl;
}
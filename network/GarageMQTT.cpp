#include "GarageMQTT.h"

#include <cstring>
#include <iostream>
#include <utility>

GarageMQTT* GarageMQTT::instance_ = nullptr;

GarageMQTT::GarageMQTT(const std::string& ssid,
                       const std::string& password,
                       std::string brokerIp,
                       int port,
                       std::string clientId)
    : ssid_(ssid),
      password_(password),
      brokerIp_(std::move(brokerIp)),
      port_(port),
      clientId_(std::move(clientId)),
      ipstack_(ssid.c_str(), password.c_str()),
      client_(ipstack_),
      commandAvailable_(false),
      //reconnectAttempts_(0),
      remoteDisabled_(false),
      remoteFailureReported_(false)
{
    instance_ = this; //ptr save address mqtt
    commandBuffer_[0] = '\0';

    connectData_ = MQTTPacket_connectData_initializer;
    connectData_.MQTTVersion = 3;
    connectData_.clientID.cstring = const_cast<char*>(clientId_.c_str());
}

bool GarageMQTT::isRemoteAvailable()
{
    return !remoteDisabled_ && client_.isConnected();
}

bool GarageMQTT::isRemoteDisabled() const
{
    return remoteDisabled_;
}

bool GarageMQTT::connectTcp()
{
    std::cout << "Connecting TCP to " << brokerIp_ << ":" << port_ << std::endl;

    if (const int rc = ipstack_.connect(brokerIp_.c_str(), port_); rc != 0)
    {
        std::cout << "TCP connection failed, rc=" << rc << std::endl;
        return false;
    }

    std::cout << "TCP connected" << std::endl;
    return true;
}

bool GarageMQTT::connectMqtt()
{
    std::cout << "Connecting MQTT..." << std::endl;

    const int rc = client_.connect(connectData_);
    if (rc != 0)
    {
        std::cout << "MQTT connection failed, rc=" << rc << std::endl;
        return false;
    }

    std::cout << "MQTT connected" << std::endl;
    return true;
}

bool GarageMQTT::subscribeCommandTopic()
{
    const int rc = client_.subscribe(COMMAND_TOPIC, MQTT::QOS0, messageArrived);
    if (rc != 0)
    {
        std::cout << "Subscribe failed, rc=" << rc << std::endl;
        return false;
    }

    std::cout << "Subscribed to " << COMMAND_TOPIC << std::endl;
    return true;
}

bool GarageMQTT::connect()
{
    if (remoteDisabled_)
    {
        return false;
    }

    int reconnectAttempts_ = 0;

    while (reconnectAttempts_ < MAX_RECONNECT_ATTEMPTS)
    {
        std::cout << "MQTT connect attempt "
                  << (reconnectAttempts_ + 1)
                  << "/"
                  << MAX_RECONNECT_ATTEMPTS
                  << std::endl;

        if (!connectTcp())
        {
            reconnectAttempts_++;
            tight_loop_contents();
            continue;
        }

        if (!connectMqtt())
        {
            reconnectAttempts_++;
            tight_loop_contents();
            continue;
        }

        if (!subscribeCommandTopic())
        {
            reconnectAttempts_++;
            tight_loop_contents();
            continue;
        }

        //reconnectAttempts_ = 0;
        remoteFailureReported_ = false;
        remoteDisabled_ = false;

        std::cout << "Remote control ready." << std::endl;
        std::cout << "Listening for commands on topic: garage/door/command" << std::endl;
        std::cout << "Available commands: OPEN, CLOSE, STOP, CALIBRATE, STATUS" << std::endl;
        std::cout << "Command 'CALIBRATE' or use SW0 + SW2 to calibrate." << std::endl;

        return true;
    }

    remoteDisabled_ = true;

    if (!remoteFailureReported_)
    {
        std::cout << "Remote control unavailable after "
                  << MAX_RECONNECT_ATTEMPTS
                  << " failed attempts." << std::endl;
        std::cout << "Device will continue in local-only mode." << std::endl;
        std::cout << "Use SW0 + SW2 to calibrate." << std::endl;
        remoteFailureReported_ = true;
    }

    return false;
}

bool GarageMQTT::publishMessage(const char* topic, const char* payload)
{
    if (payload == nullptr)
    {
        return false;
    }

    if (remoteDisabled_)
    {
        return false;
    }

    if (!client_.isConnected())
    {
        return false;
    }

    MQTT::Message message{};
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = const_cast<char*>(payload);
    message.payloadlen = std::strlen(payload);

    if (const int rc = client_.publish(topic, message); rc != 0)
    {
        std::cout << "Publish failed to " << topic << ", rc=" << rc << std::endl;
        return false;
    }

    std::cout << "Published to " << topic << ": " << payload << std::endl;
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

    if (client_.isConnected())
    {
        client_.yield(1);
    }

    tight_loop_contents();
}

bool GarageMQTT::hasPendingCommand() const
{
    return commandAvailable_;
}

bool GarageMQTT::popPendingCommand(char* buffer, std::size_t size)
{
    if (!commandAvailable_ || buffer == nullptr || size == 0)
    {
        return false;
    }

    std::strncpy(buffer, commandBuffer_, size - 1);
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

    std::size_t len = message.payloadlen;
    if (len >= sizeof(instance_->commandBuffer_))
    {
        len = sizeof(instance_->commandBuffer_) - 1;
    }

    std::memcpy(instance_->commandBuffer_, message.payload, len);
    instance_->commandBuffer_[len] = '\0';
    instance_->commandAvailable_ = true;

    std::cout << "MQTT command received: [" << instance_->commandBuffer_ << "]" << std::endl;
}
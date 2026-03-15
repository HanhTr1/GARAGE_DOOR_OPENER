#pragma once

#include <string>
#include <cstddef>

#include "Countdown.h"
#include "IPStack.h"
#include "MQTTClient.h"

class GarageMQTT {
public:
    GarageMQTT(const std::string& ssid,
               const std::string& password,
               std::string brokerIp,
               int port,
               std::string clientId);

    bool connect();
    void loop();

    bool publishStatus(const char* payload);
    bool publishResponse(const char* payload);

    bool isRemoteAvailable();
    [[nodiscard]] bool isRemoteDisabled() const;

    [[nodiscard]] bool hasPendingCommand() const;
    bool popPendingCommand(char* buffer, std::size_t size);

private:
    bool connectTcp();
    bool connectMqtt();
    bool subscribeCommandTopic();
    bool publishMessage(const char* topic, const char* payload);

    static void messageArrived(MQTT::MessageData& md);

    static GarageMQTT* instance_;

    static constexpr const char* COMMAND_TOPIC  = "garage/door/command";
    static constexpr const char* STATUS_TOPIC   = "garage/door/status";
    static constexpr const char* RESPONSE_TOPIC = "garage/door/response";

    static constexpr int MAX_RECONNECT_ATTEMPTS = 3;

    std::string ssid_;
    std::string password_;
    std::string brokerIp_;
    int port_;
    std::string clientId_;

    IPStack ipstack_;
    MQTT::Client<IPStack, Countdown> client_;
    MQTTPacket_connectData connectData_{};

    char commandBuffer_[64]{};
    bool commandAvailable_;

    bool remoteDisabled_;
    bool remoteFailureReported_;
};
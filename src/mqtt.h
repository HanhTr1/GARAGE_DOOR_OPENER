//
// Created by Tran Lan Anh on 8.3.2026.
//

#ifndef GARAGE_DOOR_OPENER_MQTT_H
#define GARAGE_DOOR_OPENER_MQTT_H
#include <string>
#include "Countdown.h"
#include "IPStack.h"
#include "MQTTPacket.h"
#include "MQTTClient.h"
#include <iostream>
using namespace std;

class MQTTConnect
{
private:
    string ssid;
    string password;
    string broker_ip;
    int port;
    string client_id;
    IPStack ipstack;
    MQTT::Client<IPStack, Countdown> client;

public:
    MQTTConnect(const string& ssid,
         const string& password,
         const string& broker_ip,
         int port,
         const string& client_id);

    bool connectTCP();
    bool connectMQTT();
    bool connect();
    bool subscribeTopic(const char* topic);
    bool publishMessage(const char* topic, const char* payload);
    void loop();
    static void messageArrived(MQTT::MessageData& md);
};

#endif //GARAGE_DOOR_OPENER_MQTT_H
//
// Created by Tran Lan Anh on 8.3.2026.
//
#include "mqtt.h"
MQTTConnect::MQTTConnect(const string& ssid,
                         const string& password,
                         const string& broker_ip,
                         int port,
                         const string& client_id)
    :   ssid(ssid),
        password(password),
        broker_ip(broker_ip),
        port(port),
        client_id(client_id),
        ipstack(ssid.c_str(), password.c_str()),
        client(ipstack){}

bool MQTTConnect::connectTCP()
{
    cout << "Connecting TCP to " << broker_ip << ":" << port << endl;
    int rc = ipstack.connect(broker_ip.c_str(), port);
    if (rc != 0)
    {
        cout << "TCP connection failed, rc:  " << rc << endl;
        return false;
    }
    cout << "TCP connected" << endl;
    return true;
}

bool MQTTConnect::connectMQTT()
{
    cout << "Connecting MQTT ...." << endl;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = const_cast<char*>(client_id.c_str());
    int rc = client.connect(data);
    if (rc != 0)
    {
        cout << "MQTT connection failed, rc:  " << rc << endl;
        return false;
    }
    cout << "MQTT connected" << endl;
    return true;
}

bool MQTTConnect::connect()
{
    int retry = 0;
    const int max_retry = 3;
    while (retry < max_retry)
    {
        cout << "MQTT connect attempt " << retry + 1 << "/" << max_retry << endl;
        if (!connectTCP())
        {
            retry++;
            tight_loop_contents();
            continue;
        }
        if (!connectMQTT())
        {
            retry++;
            tight_loop_contents();
            continue;
        }
        cout << "MQTT connected successfully" << endl;
        return true;
    }
    cout << "MQTT connection failed after " << max_retry << " attempts" << endl;
    return false;
}

bool MQTTConnect::subscribeTopic(const char* topic)
{
    int rc = client.subscribe(topic, MQTT::QOS0, messageArrived);
    if (rc != 0)
    {
        cout << "Subscribe failed, rc:  " << rc << endl;
        return false;
    }
    cout << "Subscribed to topic " << topic << endl;
    return true;

}

bool MQTTConnect::publishMessage(const char* topic, const char* payload)
{
    MQTT::Message message;

    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)payload;
    message.payloadlen = strlen(payload);

    int rc = client.publish(topic, message);

    if (rc != 0) {
        cout << "Publish failed rc = " << rc << endl;
        return false;
    }

    cout << "Published to " << topic << " : " << payload << endl;
    return true;
}

void MQTTConnect::loop()
{
    client.yield(1000);

    // CPU idle
    tight_loop_contents();
}

void MQTTConnect::messageArrived(MQTT::MessageData& md)
{
    MQTT::Message& message = md.message;

    cout << "Message arrived on topic: ";

    for (int i = 0; i < md.topicName.lenstring.len; i++) {
        cout << md.topicName.lenstring.data[i];
    }

    cout << endl;

    cout << "Payload: ";

    for (size_t i = 0; i < message.payloadlen; i++) {
        cout << ((char*)message.payload)[i];
    }

    cout << endl;
}
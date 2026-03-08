#include "src/calibration.h"
#include "src/mqtt.h"

int main() {
    stdio_init_all();
    cout << "Program started" << endl;

    MQTTConnect mqtt(
        "Chill guy in chill house-low",
        "You@re@theft644",
        "192.168.0.3",     // IP broker
        1883,
        "PicoW-sample"
    );

    if (!mqtt.connect()) {
        cout << "MQTT connect failed" << endl;
        while (true) {
            tight_loop_contents();
        }
    }

    mqtt.subscribeTopic("garage/door/cmd");
    mqtt.publishMessage("garage/door/status", "online");

    while (true) {
        mqtt.loop();
        sleep_ms(100);
    }

}
#include "src/calibration.h"

int main() {
    stdio_init_all();
    Calibration cal;
    cal.init();

    while (true) {

        cal.calibration_process();
    }

}
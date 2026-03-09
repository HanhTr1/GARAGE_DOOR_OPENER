#include "Button.h"

static constexpr uint32_t COMBO_DEBOUNCE_MS = 200;

Button::Button(uint sw0, uint sw1, uint sw2)
    : sw0_(sw0),
      sw1_(sw1),
      sw2_(sw2),
      lastSw0_(false),
      lastSw1_(false),
      lastSw2_(false),
      lastComboPressMs_(0)
{
}

void Button::init()
{
    gpio_init(sw0_);
    gpio_set_dir(sw0_, GPIO_IN);
    gpio_pull_up(sw0_);

    gpio_init(sw1_);
    gpio_set_dir(sw1_, GPIO_IN);
    gpio_pull_up(sw1_);

    gpio_init(sw2_);
    gpio_set_dir(sw2_, GPIO_IN);
    gpio_pull_up(sw2_);

    lastSw0_ = false;
    lastSw1_ = false;
    lastSw2_ = false;
    lastComboPressMs_ = 0;
}

bool Button::detectPressEdge(uint pin, bool& lastState)
{
    const bool current = (gpio_get(pin) == 0);
    const bool pressed = current && !lastState;
    lastState = current;
    return pressed;
}

bool Button::sw1Pressed()
{
    return detectPressEdge(sw1_, lastSw1_);
}

bool Button::sw0AndSw2Pressed()
{
    const bool sw0 = (gpio_get(sw0_) == 0);
    const bool sw2 = (gpio_get(sw2_) == 0);
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    bool pressed = false;

    if (sw0 && sw2)
    {
        if (!(lastSw0_ && lastSw2_))
        {
            if (now - lastComboPressMs_ >= COMBO_DEBOUNCE_MS)
            {
                pressed = true;
                lastComboPressMs_ = now;
            }
        }
    }

    lastSw0_ = sw0;
    lastSw2_ = sw2;
    return pressed;
}
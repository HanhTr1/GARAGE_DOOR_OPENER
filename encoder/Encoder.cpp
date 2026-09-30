#include "Encoder.h"

Encoder* Encoder::instance_ = nullptr;
queue_t Encoder::eventQueue_;

Encoder::Encoder(uint pinA, uint pinB): pinA_(pinA),pinB_(pinB){}

void Encoder::init(){
    gpio_init(pinA_);
    gpio_set_dir(pinA_, GPIO_IN);
    gpio_pull_up(pinA_);

    gpio_init(pinB_);
    gpio_set_dir(pinB_, GPIO_IN);
    gpio_pull_up(pinB_);

    queue_init(&eventQueue_, sizeof(int), 64);

    instance_ = this;
    gpio_set_irq_enabled_with_callback(pinA_, GPIO_IRQ_EDGE_RISE, true, &Encoder::isr);
}

void Encoder::isr(uint gpio, uint32_t events){
    (void)events;

    if (instance_ == nullptr)
    {
        return;
    }

    if (gpio != instance_->pinA_)
    {
        return;
    }

    int delta = 0;
    if (gpio_get(instance_->pinB_) == 0)
    {
        delta = -1;
    }
    else
    {
        delta = 1;
    }

    queue_try_add(&eventQueue_, &delta);
}

int Encoder::encoderSteps(){
    int sum = 0;
    int value = 0;

    while (queue_try_remove(&eventQueue_, &value))
    {
        sum += value;
    }
    return sum;
}
#pragma once

#include "LedInterface.h"
#include "pico/stdlib.h"

class LedGpio : public LedInterface
{
public:
    LedGpio(const unsigned int pin, const bool lowActive = false):
        pin_(pin),
        onLevel_(!lowActive)
    {}

    virtual void initialize() override
    {
        gpio_init(pin_);
        gpio_set_dir(pin_, GPIO_OUT);
        off();
    }

    virtual void on() override 
    {
        gpio_put(pin_, onLevel_);
    }
    
    virtual void off() override
    {
        gpio_put(pin_, !onLevel_);
    }
private:
    const unsigned int pin_;
    const bool onLevel_;
};
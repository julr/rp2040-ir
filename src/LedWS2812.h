#pragma once
#include <cstdint>
#include "LedInterface.h"

class LedWS2812 final : public LedInterface
{
public:
    LedWS2812(const unsigned int pin);
    virtual void initialize() override;
    virtual void on() override;
    virtual void off() override;
private:
    const unsigned int pin_;
    void set(std::uint8_t r, std::uint8_t g, std::uint8_t b);
};
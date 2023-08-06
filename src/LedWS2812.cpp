#include "LedWS2812.h"
#include "ws2812.pio.h"

#include "hardware/pio.h"

LedWS2812::LedWS2812(const unsigned int pin) :
    pin_{pin}
{
}

void LedWS2812::initialize()
{
    const PIO pio{pio0};
    const int sm{0};
    const uint offset{pio_add_program(pio, &ws2812_program)};
    ws2812_program_init(pio, sm, offset, pin_, 800000, false);
}

void LedWS2812::on()
{
    set(0, 127, 0);
}

void LedWS2812::off()
{
    set(0, 0, 0);
}

void LedWS2812::set(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    const std::uint32_t color{(static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) | (static_cast<std::uint32_t>(b) << 8)};
    pio_sm_put_blocking(pio0, 0, color);
}
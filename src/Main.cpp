#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "etl/array.h"

#include "tusb.h"
#include "tusb_config.h"

#include "IrDecoder.h"
#include "LedGpio.h"
#include "LedWS2812.h"

#define RP2040ONE

void core1_loop()
{
    while (true)
    {
        tud_task();
    }
}

int main()
{
    stdio_init_all();

    // setup the hardware (board depended)
    #ifdef RP2040ONE //RP2040-One board (receiver on pin 12, WS1812B LED on pin 16)
    constexpr unsigned int irDecoderPin{12};
    LedWS2812 led{16};
    #else //Pi Pico Board (receiver on pin 22, normal LED on pin 25)
    constexpr unsigned int irDecoderPin{22};
    LedGpio led{25};
    #endif
    
    IrDecoder decoder{irDecoderPin, true, &led};
    led.initialize();
    decoder.initialize();
    IrDecoder::Data irData;

    // start USB and execute on core1, just because I can
    tusb_init();
    multicore_launch_core1(core1_loop);
    etl::array<std::uint8_t, 64> usbDataBuffer{};
    usbDataBuffer.fill(0x00);

    while (true)
    {
        if (decoder.getData(irData))
        {
            usbDataBuffer[0] = 0x00;
            usbDataBuffer[1] = irData.address;
            usbDataBuffer[2] = irData.command;
            usbDataBuffer[3] = (irData.repeated) ? 1 : 0;
            tud_vendor_write(usbDataBuffer.data(), usbDataBuffer.size());
        }
    }
}

extern "C" void tud_vendor_rx_cb(std::uint8_t ift)
{
    // Echo only for test of USB rx
    std::uint8_t rxBuffer[CFG_TUD_VENDOR_RX_BUFSIZE];
    tud_vendor_read(rxBuffer, sizeof(rxBuffer));
    tud_vendor_n_write(ift, rxBuffer, sizeof(rxBuffer));
}
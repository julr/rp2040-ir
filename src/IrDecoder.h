#pragma once
#include <cstdint>
#include "etl/delegate.h"
#include "pico/stdlib.h"
#include "LedInterface.h"

class IrDecoder
{
public:
    struct Data
    {
        std::uint8_t address;
        std::uint8_t command;
        bool repeated;
    };

    IrDecoder(const unsigned int pin, const bool idleHigh = false, LedInterface* const led = nullptr) :
    irPin_{pin},
    irInvert_{idleHigh},
    led_{led}
    {}

    void initialize(const bool withPull = true);

    bool getData(Data& data);

private:
    enum class DecoderState
    {
        IDLE,
        WAIT_FOR_START,
        ADDRESS_OR_REPEAT,
        RECEIVE_FRAME,
        WAIT_END
    };

    using CallbackDelegateType = etl::delegate<void(unsigned int, std::uint32_t)>;
    const unsigned int irPin_;
    const bool irInvert_;
    LedInterface* const led_;
    DecoderState state_{DecoderState::IDLE};
    std::uint8_t bitCounter_{0};
    std::uint64_t lastTimeStamp_{0};
    std::uint32_t frameData_{0};
    Data data_{0};
    bool dataIsNew_{false};
    alarm_id_t timeoutAlarmId_{-1};
    static CallbackDelegateType callbackDelegate_;
    



    
    void gpioCallbackFunction(unsigned int gpio, std::uint32_t events);
    static void gpioCallbackTrampoline(unsigned int gpio, std::uint32_t events);
    bool isPulseInRange(std::uint64_t now, std::uint32_t timeUs, std::uint32_t tolerance = 500u);
    void setState(DecoderState newState);

    static std::int64_t timeoutAlarmCallback(alarm_id_t id, void *user_data);

};
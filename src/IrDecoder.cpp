#include "IrDecoder.h"

#define ONE_TIME 1600
#define ZERO_TIME 500
#define BIT_TOLERANCE 250

IrDecoder::CallbackDelegateType IrDecoder::callbackDelegate_{};

void IrDecoder::gpioCallbackTrampoline(unsigned int pin, std::uint32_t events)
{
    if(callbackDelegate_.is_valid())
    {
        callbackDelegate_(pin, events);
    }
}

void IrDecoder::initialize(const bool withPull)
{
    callbackDelegate_ = CallbackDelegateType::create<IrDecoder, &IrDecoder::gpioCallbackFunction>(*this);

    gpio_init(irPin_);
    gpio_set_dir(irPin_, GPIO_IN);
    if(withPull)
    {
        gpio_set_pulls(irPin_, irInvert_, !irInvert_);
    }
    else
    {
        gpio_disable_pulls(irPin_);
    }

    gpio_set_irq_enabled_with_callback(irPin_, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &IrDecoder::gpioCallbackTrampoline);
}

bool IrDecoder::getData(IrDecoder::Data& data)
{
    const bool ret{dataIsNew_};
    dataIsNew_ = false;
    data = data_;
    return ret;
}

void IrDecoder::gpioCallbackFunction(unsigned int, std::uint32_t events)
{
    const std::uint64_t currentTimeStamp = time_us_64();
    const bool level{static_cast<bool>(irInvert_ ? events & GPIO_IRQ_EDGE_FALL : events & GPIO_IRQ_EDGE_RISE)};
    
    /*
    const std::uint32_t timeDiff{static_cast<std::uint32_t>(currentTimeStamp - lastTimeStamp_)};
    printf("Level: %u time: %u\n", level, timeDiff);
    */
    
    switch (state_)
    {
    case DecoderState::IDLE:
        if(level)
        {
            setState(DecoderState::WAIT_FOR_START);
        }
        break;

    case DecoderState::WAIT_FOR_START:
        if(!level)
        {
            if(isPulseInRange(currentTimeStamp, 9000))
            {
                setState(DecoderState::ADDRESS_OR_REPEAT);
            }
            else
            {
                setState(DecoderState::IDLE);
                const std::uint32_t timeDiff{static_cast<std::uint32_t>(currentTimeStamp - lastTimeStamp_)};
                printf("Invalid start time: %lu\n", timeDiff);
            }
            
        }
        else
        {
            setState(DecoderState::IDLE);
            printf("Invalid level after expected start\n");
        }
        break;

    case DecoderState::ADDRESS_OR_REPEAT:
        if(level)
        {
            if(isPulseInRange(currentTimeStamp, 4500))
            {
                setState(DecoderState::RECEIVE_FRAME);
                frameData_ = 0;
                bitCounter_ = 0;
            }
            else if(isPulseInRange(currentTimeStamp, 2250))
            {
                data_.repeated = true;
                dataIsNew_ = true;
                setState(DecoderState::WAIT_END);
            }
        }
        else
        {
            setState(DecoderState::IDLE);
            printf("Invalid level after wait address or repeat\n");
        }
        break;

    case DecoderState::RECEIVE_FRAME:
        if(level)
        {
            if(isPulseInRange(currentTimeStamp, ZERO_TIME, BIT_TOLERANCE)) //0
            {
                frameData_>>=1;
                bitCounter_++;
            }
            else if(isPulseInRange(currentTimeStamp, ONE_TIME, BIT_TOLERANCE)) //1
            {
                frameData_>>=1;
                frameData_ |= 0x80000000;
                bitCounter_++;
            }
            else
            {
                const std::uint32_t timeDiff{static_cast<std::uint32_t>(currentTimeStamp - lastTimeStamp_)};
                printf("Invalid bit length: %lu\n", timeDiff);
                setState(DecoderState::IDLE);
            }

            if(bitCounter_ >= 32)
            {
                const std::uint8_t command_inverse = static_cast<std::uint8_t>(frameData_ >> 24);
                const std::uint8_t command = static_cast<std::uint8_t>(frameData_>> 16);
                const std::uint8_t address_inverse = static_cast<std::uint8_t>(frameData_ >> 8);
                const std::uint8_t address = static_cast<std::uint8_t>(frameData_);
                
                if((command + command_inverse == 0xFF) && (address + address_inverse == 0xFF))
                {
                    data_.address = address;
                    data_.command = command;
                    data_.repeated = false;
                    dataIsNew_ = true;
                    printf("Got frame: Command: %x, Address: %x\n", command, address);
                }
                else
                {
                    printf("Got invalid frame: 0x%lx c: %x, ci: %x, a: %x, ai: %x\n", frameData_, command, command_inverse, address, address_inverse);
                }
                setState(DecoderState::WAIT_END);
            }
        }
        break;
    
    case DecoderState::WAIT_END:
        if(!level)
        {
            setState(DecoderState::IDLE);
        }
        break;

    default:
        setState(DecoderState::IDLE);
        break;
    }
    
    lastTimeStamp_ = currentTimeStamp;
}

void IrDecoder::setState(IrDecoder::DecoderState newState)
{
    switch(newState)
    {
    case DecoderState::WAIT_FOR_START:
        timeoutAlarmId_ = add_alarm_in_ms(100, &IrDecoder::timeoutAlarmCallback, this, false);
        if(led_ != nullptr) led_->on();
        break;
    case DecoderState::IDLE:
        cancel_alarm(timeoutAlarmId_);
        if(led_ != nullptr) led_->off();
        break;
    default:
        break;
    }

    state_ = newState;
}

bool IrDecoder::isPulseInRange(std::uint64_t now, std::uint32_t timeUs, std::uint32_t tolerance)
{
    const std::uint32_t timeDiff{static_cast<std::uint32_t>(now - lastTimeStamp_)};
    const std::uint32_t targetDiff{(timeUs > timeDiff) ? timeUs - timeDiff : timeDiff - timeUs};
    return targetDiff <= tolerance;
}

std::int64_t IrDecoder::timeoutAlarmCallback(alarm_id_t id, void *user_data)
{
    reinterpret_cast<IrDecoder*>(user_data)->setState(DecoderState::IDLE);
    return 0;
}
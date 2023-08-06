#pragma once

class LedInterface
{
public:
    virtual void initialize() = 0;
    virtual void on() = 0;
    virtual void off() = 0;
};
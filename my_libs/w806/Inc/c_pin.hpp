#ifndef C_PIN_HPP
#define C_PIN_HPP

#include "mylibs_config.hpp"
#include "myints.h"
#include "i_pin.hpp"

class C_Pin{
private:
    u32 port;
    u32 pin_no;
public:
    C_Pin(u32 port , u32 pin_no);
    C_Pin(GPIO_TypeDef port, u32 pin2n);
    void write_pin(PinState x);
    void toggle_pin();
};

#endif
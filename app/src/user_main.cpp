#include "user_main.h"
#include "mylibs_config.hpp"
#include "c_pin.hpp"

void user_main()
{
    C_Pin led1 = C_Pin(1, 0);
    C_Pin led2 = C_Pin(1, 1);
    C_Pin led3 = C_Pin(1, 2);
    while(true){
        led1.write_pin(Pin_Set);
        XDelayMs(100);
        led2.write_pin(Pin_Set);
        XDelayMs(100);
        led1.write_pin(Pin_Reset);
        XDelayMs(100);
        led2.write_pin(Pin_Reset);
        XDelayMs(100);
    }
}
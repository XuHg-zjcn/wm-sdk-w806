#include "c_pin.hpp"

C_Pin::C_Pin(u32 port , u32 pin_no)
{
    this->port = port;
    this->pin_no = pin_no;
}

void C_Pin::write_pin(PinState x)
{
    if(port==0){
        HAL_GPIO_WritePin(GPIOA, 1<<pin_no, (GPIO_PinState)x);
    }else{
        HAL_GPIO_WritePin(GPIOB, 1<<pin_no, (GPIO_PinState)x);
    }
}

void C_Pin::toggle_pin()
{
    if(port==0){
        HAL_GPIO_TogglePin(GPIOA, 1<<pin_no);
    }else{
        HAL_GPIO_TogglePin(GPIOB, 1<<pin_no);
    }
}
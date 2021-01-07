#include "mbed.h"
#include  "string"

#pragma once

class LCD
{
public:
    LCD(PinName d4,PinName d5, PinName d6, PinName d7, PinName rs, PinName en);
    LCD() = delete;

    void init(); // funtion to initialise the LCD

    void wtrChar(char); // function to display characters on the LCD
    void wtrString(string format);
    void wtrNumber(float num);
    void setCursor(uint8_t row, uint8_t column);
    void clr();

private:
    DigitalOut ctrl;
    DigitalOut en;
    BusOut data;
    void ftoa(float n, char *res,int afterpoint);
    int intToStr(int x, char str[], int d);
    void reverse(char *str,int len);
    void tglEn(); // function to toggle the enable bit
};

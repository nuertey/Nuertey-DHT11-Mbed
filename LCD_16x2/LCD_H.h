#include "mbed.h"
#include  "string"

#ifndef LCD_H
#define LCD_H

class LCD
{
public:
    LCD(PinName d4,PinName d5, PinName d6, PinName d7, PinName rs, PinName en);
    LCD() = delete;

    void init(); // funtion to initialise the LCD

    void wtrChar(char); //funtion to display characters on the LCD
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
    void tglEn(); //funtion to toggle the enable bit
};

#endif

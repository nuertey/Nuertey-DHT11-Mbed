#include "mbed.h"
#include "LCD_H.h"

/*
LCD::LCD():ctrl(p9),en(p10),data(p5,p6,p7,p8)
{};
*/

LCD::LCD(PinName d4,PinName d5, PinName d6, PinName d7, PinName rs, PinName ena)
    : ctrl(rs)
    , en(ena)
    , data(d4, d5, d6, d7) 
{};

void LCD::tglEn()
{
    en=1;
    ThisThread::sleep_for(1);
    en=0;
}

void LCD::init()
{
    ThisThread::sleep_for(100);

    ctrl = 0; //Recibir instrucciones
    en=0;

    // Function Set = Interface bits DB7=0, DB6=o, DB5=1,DB4 (DL=1->8bits, =0->4bits)=1
    data = 0b0011;
    tglEn();
    wait_us(50);

    //Function SET
    // Interface bits DB7=0, DB6=o, DB5=1,DB4 (DL=1->8bits, =0->4bits)=0 // 4 BITS
    data=0b0010; // 2
    tglEn();
    // DB7(N=0 -> 1 line, =1 -> 2 line )=1, DB6(F=0->5x8 dot, =1 5x11 dot)=0, DB5(x)=0,DB4 (x)=0 // 2 line & 5x8 dot
    data=0b1000; //
    tglEn();
    wait_us(50);

    //Funtion SET x2
    data=0b0010; // 2
    tglEn();
    data=0b1000; // 8
    tglEn();
    wait_us(50);

    ////////////////////////////////////////////////////////////////////////

    //DISPLAY ON/OFF
    data=0b0000; // 0
    tglEn();
    //DB7=1, DB6(D=0 -> Display turned off | =1 -> display turned on)=1, DB5(C=0 -> cursor turned off | =1 -> cursor turned on)=1,DB4 (B=0 -> cursor blink turned off | =1 -> cursor blink turned on)=1
    // Display turned on, cursor turned on and blink turned on
    data=0b1111; // 15
    tglEn();
    wait_us(50);

    //DISPLAY clear
    data=0b0000; // 0
    tglEn();
    data=0b0001; // 1
    tglEn();
    ThisThread::sleep_for(2);

    // Entry mode set
    data=0b0000; // 0
    tglEn();
    //DB7=0, DB6=1, DB5(I/D=0 -> moves left | =1 -> moves right)=0,DB4 (S=0 -> shift of entire display is not performed| =1 -> shift of entire display is performed according to I/D value)=1
    //Shift display to the right. Cursor follows the display shift
    data=0b0001; // 1
    tglEn();
    ThisThread::sleep_for(2);
}

void LCD::wtrChar(char a)
{
    ctrl=1;
    data = a>>4;
    tglEn();
    data = a;
    tglEn();
    wait_us(50);
}

void LCD::clr()
{
    ctrl=0;
    en=0;

    //CLEAR
    data=0b0000; // 0
    tglEn();
    data=0b0001; // 1
    tglEn();
    ThisThread::sleep_for(2);

    // Entry mode set
    data=0b0000; // 0
    tglEn();
    //DB7=0, DB6=1, DB5(I/D=0 -> moves left | =1 -> moves right)=0,DB4 (S=0 -> shift of entire display is not performed| =1 -> shift of entire display is performed according to I/D value)=1
    //Shift display to the right. Cursor follows the display shift
    data=0b0001; // 1
    tglEn();
    ThisThread::sleep_for(2);
}

void LCD::wtrString(string format)
{
    for (unsigned int i=0; i<format.size(); i++)
    {
        wtrChar(format[i]);
    }
}

void LCD::reverse(char *str,int len)
{
    int i=0, j= len-1, temp;
    while (i<j)
    {
        temp=str[i];
        str[i]=str[j];
        str[j]=temp;
        i++;
        j--;
    }
}

int LCD::intToStr(int x, char str[],int d)
{
    int i=0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x=x/10;
    }
    // If number of digits required is more, then
    while(i<d)
    {
        str[i++] = '0';
    }
    reverse(str,i);
    str[i]='\0';
    return i;
}

void LCD::ftoa(float n, char *res,int afterpoint)
{
    int ipart=(int)n;
    float fpart=n-(float)ipart;
    int i=intToStr(ipart,res,0);

    if (afterpoint!=0)
    {
        res[i]='.';
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart= fpart*(pow((double)10,(double)afterpoint));
        intToStr((int)fpart,res+i+1,afterpoint);
    }
}

void LCD:: wtrNumber(float num)
{
    char res[5];
    int afterpoint=2;
    ftoa(num,res,afterpoint);
    wtrString(res);
}

//row, fila 0 o fila 1
//columna de la 0 a la 15
void LCD::setCursor(uint8_t row, uint8_t column)
{
    ctrl=0;
    en=0;
    // Ubicar Cursor, posiciones en hexadecimal

    switch (row)
    {
        case 0:
        {
            data=0b1000;
            tglEn();
            ThisThread::sleep_for(200);
            break;
        }
        case 1:
        {
            data=0b1100;
            tglEn();
            ThisThread::sleep_for(200);
            break;
        }
        default:
        {
            data=0b1000;
            tglEn();
            ThisThread::sleep_for(200);
            break;
        }
    }
    data=column;
    tglEn();
    ThisThread::sleep_for(200);
}

/***********************************************************************
* @file      main.cpp
*
*    An ARM Mbed application that illustrates how a NUCLEO-F767ZI can be 
*    connected to a DHT11 Temperature and Humidity sensor and values 
*    output to an LCD 16x2 display, all mocked-up via a breadboard. This
*    allows us to periodically obtain temperature and humidity readings.
*
* @brief   Input: DHT11 temperature/humidity readings. Output: LCD 16x2.
* 
* @note    Enumerated Peripheral Components Details Are As Follows:
* 
*   1) The DHT11 sensor measures and provides humidity and temperature 
*      values serially over a single wire. Its characteristics are as 
*      follows:
*      
*      - It can measure relative humidity in percentages (20 to 90% RH) 
*        and temperature in degree Celsius in the range of 0 to 50°C.
* 
*   2) LCDs (Liquid Crystal Displays) are used in embedded system 
*      applications for displaying various parameters and statuses of 
*      the system. The LCD 16x2 has the following characteristics:
*  
*      - It is a 16-pin device that has 2 rows that can accommodate 16 
*        characters each. The pins and their corresponding functions are:
* 
*         [a] VSS  - Ground
*         [b] VCC  - +5V
*         [c] VEE  - Contrast Control
*         [d] RS   - Register Select
*         [e] RW   - Read/Write
*         [f] E    - Enable
*         [g] D0   - Data Pin 0
*         [h] D1   - Data Pin 1
*         [i] D2   - Data Pin 2
*         [j] D3   - Data Pin 3
*         [k] D4   - Data Pin 4
*         [l] D5   - Data Pin 5
*         [m] D6   - Data Pin 6
*         [n] D7   - Data Pin 7
*         [o] LED+ - LED+ 5V
*         [p] LED- - LED- Ground
* 
*      - It can be used either in a 4-bit mode or an 8-bit mode.
* 
*      - For displaying on it, custom characters can be created.
* 
*      - And lastly, the LCD 16x2 has 8 data lines and 3 control lines 
*        that can be used for control purposes.
*
* @warning   Note that the I/O pins of STM32 NUCLEO-F767ZI are 3.3 V 
*            compatible instead of 5 V for say, the Arduino Uno V3 
*            microcontroller.
* 
*            Furthermore, the STM32 GPIO pins are not numbered 1-64; rather, 
*            they are named after the MCU IO port that they are controlled
*            by. Hence PA_5 is pin 5 on port A. This means that physical
*            pin location may not be related to the pin name. Consult 
*            the "Extension connectors" sub-chapter of the "Hardware 
*            layout and configuration" chapter of the STM32 Nucleo-144 
*            boards UM1974 User manual (en.DM00244518.pdf) to know where
*            each pin is located. Note that all those pins shown can be 
*            used as GPIOs, however most of them also have alternative 
*            functions which are indicated on those diagrams.
*
* @author    Nuertey Odzeyem
* 
* @date      January 5th, 2021
*
* @copyright Copyright (c) 2021 Nuertey Odzeyem. All Rights Reserved.
***********************************************************************/
#include "Utilities.h"

// Do not return from main() as in  Embedded Systems, there is nothing
// (conceptually) to return to. A crash will occur otherwise!
int main()
{
    printf("\r\n\r\nNuertey-DHT11-Mbed - Beginning... \r\n\r\n");

    //DisplayLCDCapabilities();

    if (Utility::InitializeGlobalResources())
    {
        Utility::g_pMasterEventQueue->dispatch_forever();

        Utility::ReleaseGlobalResources();
    }
    else
    {
        printf("\r\n\r\nError! Initialization of Global Resources Failed!\n");
    }

    printf("\r\n\r\nNuertey-DHT11-Mbed Application - Exiting.\r\n\r\n");
}

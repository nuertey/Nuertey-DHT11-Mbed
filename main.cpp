/***********************************************************************
* @file      main.cpp
*
*    An ARM Mbed application that illustrates how a NUCLEO-F767ZI can be 
*    connected to a DHT11 Temperature and Humidity sensor and an LCD 16x2
*    display, all via a breadboard, to periodically obtain temperature 
*    and humidity readings.
*
* @brief   Input: DHT11 temperature/humidity readings. Output: LCD 16x2. 
* 
* @note    
*
* @warning
*
* @author    Nuertey Odzeyem
* 
* @date      January 5th, 2021
*
* @copyright Copyright (c) 2021 Nuertey Odzeyem. All Rights Reserved.
***********************************************************************/
#include "mbed.h"
#include "DHT11.h"
#include "Utilities.h"

#define LED_ON  1
#define LED_OFF 0

// TBD Nuertey Odzeyem; ensure to protect all prints with the Utility::g_STDIOMutex
// TBD Nuertey Odzeyem; move all the DHT11 processing into its own function or lambda
// TBD Nuertey Odzeyem; consider moving the LCD 16x2 outputting too into its own function or lambda

DHT11       g_DHT11(PB_8);     // DHT11 --> PB_8

DigitalOut g_LEDGreen(LED1);
DigitalOut g_LEDBlue(LED2);
DigitalOut g_LEDRed(LED3);

Ticker      newAction;

volatile uint32_t myState;     // Indicates when to obtain a new sample.

void changeDATA(void);

void DHT11SensorAcquisition()
{
    char                  myChecksum[] = "Ok";
    DHT11::DHT11_status_t aux;
    DHT11::DHT11_data_t   g_DHT11_Data;

    // Indicate with the green LED that DHT11 processing is about to begin.
    g_LEDGreen = LED_ON;
    ThisThread::sleep_for(3);
    g_LEDGreen = LED_OFF;

    // DHT11 starts: Release the bus
    aux  =   g_DHT11.DHT11_Init ();


    myState  =   0UL; // Reset the variable
    newAction.attach( &changeDATA, 1U ); // the address of the function to be attached ( changeDATA ) and the interval ( 1s )


    // Let the callbacks take care of everything
    while(1)
    {
        sleep();

        if ( myState == 1UL )
        {
            myled = 1U;

            // Get a new data
            aux  =   g_DHT11.DHT11_GetData ( &g_DHT11_Data );

            // Check checksum to validate data
            if ( g_DHT11_Data.checksumStatus == DHT11::DHT11_CHECKSUM_OK )
            {
                myChecksum[0]  =   'O';
                myChecksum[1]  =   'k';
            }
            else
            {
                myChecksum[0]  =   'E';
                myChecksum[1]  =   'r';
            }

            // Send data through the UART
            printf("T: %d C | RH: %d %% | Checksum: %s\r\n", g_DHT11_Data.temperature, g_DHT11_Data.humidity, myChecksum);


            // Reset the variables
            myState  =   0UL;
            myled    =   0U;
        }
    }
}

void changeDATA(void)
{
    myState = 1UL;
}

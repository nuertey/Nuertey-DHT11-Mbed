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

// DHT11 uses a simplified single-wire bidirectional communication protocol.
// It follows a Master/Slave paradigm with the MCU observing these states:
// WAITING, READING.
static const uint32_t DHT11_DEVICE_WAITING(0UL);
static const uint32_t DHT11_DEVICE_READING(1UL);

// TBD Nuertey Odzeyem; ensure to protect all prints with the Utility::g_STDIOMutex
// TBD Nuertey Odzeyem; move all the DHT11 processing into its own function or lambda
// TBD Nuertey Odzeyem; consider moving the LCD 16x2 outputting too into its own function or lambda

DHT11             g_DHT11(PB_8);     // DHT11 --> PB_8
                  
DigitalOut        g_LEDGreen(LED1);
DigitalOut        g_LEDBlue(LED2);
DigitalOut        g_LEDRed(LED3);

// Use the LowPowerTicker interface to set up a recurring interrupt. 
// It calls a function repeatedly and at a specified rate.
LowPowerTicker    g_Ticker;

volatile uint32_t g_DeviceState;     // Indicates when to read in a new sample.

void ChangeDeviceState();

void DHT11SensorAcquisition()
{
    DHT11::DHT11_status_t aux;
    DHT11::DHT11_data_t g_DHT11_Data;

    // Indicate with the green LED that DHT11 processing is about to begin.
    g_LEDGreen = LED_ON;
    ThisThread::sleep_for(3);
    g_LEDGreen = LED_OFF;

    // DHT11 processing begins. Therefore first release the single-wire 
    // bidirectional bus:
    aux = g_DHT11.DHT11_Init();

    g_DeviceState = DHT11_DEVICE_WAITING;
    g_Ticker.attach(&ChangeDeviceState, 1U); // the address of the function to be attached ( changeDATA ) and the interval ( 1s )

    // Let the callbacks take care of everything
    while(1)
    {
        sleep();

        if (g_DeviceState == DHT11_DEVICE_READING)
        {
            // Indicate that we are reading from DHT11 with blue LED.
            g_LEDBlue = LED_ON;

            // Get a new data
            aux = g_DHT11.DHT11_GetData(&g_DHT11_Data);

            // Depend on checksum in trusting data read from DHT11 device.
            std::string((g_DHT11_Data.checksumStatus == DHT11::DHT11_CHECKSUM_OK) ? "PASSED" : "FAILED") checksum;

            Utility::g_STDIOMutex.lock();
            printf("Temperature: %d\tHumidity : %d%% RH\tChecksum: %s\r\n", g_DHT11_Data.temperature, g_DHT11_Data.humidity, checksum.c_str());
            Utility::g_STDIOMutex.unlock();

            g_LEDBlue = LED_OFF;

            g_DeviceState = DHT11_DEVICE_WAITING;
        }
    }
}

void ChangeDeviceState()
{
    g_DeviceState = DHT11_DEVICE_READING;
}

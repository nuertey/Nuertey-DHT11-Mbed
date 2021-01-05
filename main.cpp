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
#include "LCD_H.h"
#include "Utilities.h"

#define LED_ON  1
#define LED_OFF 0

static const uint32_t DHT11_DEVICE_USER_OBSERVABILITY_DELAY(3); // 3 seconds.
static const float    DHT11_DEVICE_STATE_CHANGE_RATE(1.0);      // 1 second.

// DHT11 uses a simplified single-wire bidirectional communication protocol.
// It follows a Master/Slave paradigm with the MCU observing these states:
// WAITING, READING.
static const uint32_t DHT11_DEVICE_WAITING(0UL);
static const uint32_t DHT11_DEVICE_READING(1UL);

// TBD Nuertey Odzeyem; ensure to protect all prints with the Utility::g_STDIOMutex
// TBD Nuertey Odzeyem; move all the DHT11 processing into its own function or lambda
// TBD Nuertey Odzeyem; consider moving the LCD 16x2 outputting too into its own function or lambda
// TBD Nuertey Odzeyem; check all pin definitions to make sure that they actually match your physical mock-up.

// DHT11 Sensor Interfacing with ARM MBED. Data communication is single-line serial.
DHT11             g_DHT11(PB_8);     // DHT11 --> PB_8

// LCD 16x2 Interfacing With ARM MBED. LCD 16x2 controlled via the 4-bit interface.
LCD               g_LCD16x2(p5, p6, p7, p8, p9, p10); // RS, RW, D4, D5, D6, D7
          
// As per my ARM NUCLEO-F767ZI specs:        
DigitalOut        g_LEDGreen(LED1);
DigitalOut        g_LEDBlue(LED2);
DigitalOut        g_LEDRed(LED3);

// Use the Ticker interface to set up a recurring interrupt. It calls a
// function repeatedly and at a specified rate. As we don't need 
// microseconds precision anyway, leverage LowPowerTicker instead. 
// Crucially, that would ensure that we do not block deep sleep mode.
LowPowerTicker    g_PeriodicStateChanger;

volatile uint32_t g_DeviceState;     // Indicates when to read in a new sample.

// ==========================================================
// Free-Floating General Helper Functions To Be Used By All :
// ==========================================================
using DHT11StatusCodesMap_t = std::map<DHT11::DHT11_status_t, std::string>;
using IndexElementDHT11_t   = DHT11StatusCodesMap_t::value_type;

inline static auto make_dht11_error_codes_map()
{
    DHT11StatusCodesMap_t eMap;
    
    eMap.insert(IndexElementDHT11_t(DHT11_SUCCESS, std::string("\"Communication success\"")));
    eMap.insert(IndexElementDHT11_t(DHT11_FAILURE, std::string("\"Communication failure\"")));
    eMap.insert(IndexElementDHT11_t(DHT11_DATA_CORRUPTED, std::string("\"Checksum error\"")));
    eMap.insert(IndexElementDHT11_t(DHT11_BUS_TIMEOUT, std::string("\"Bus response timeout/error\"")));
 
    return eMap;
}

static DHT11StatusCodesMap_t gs_DHT11StatusCodesMap = make_dht11_error_codes_map();

inline std::string ToString(const DHT11::DHT11_status_t & key)
{
    return (gs_DHT11StatusCodesMap.at(key));
}

// ==============================
// Begin Actual Implementations :
// ==============================
void AlertToRead()
{
    g_DeviceState = DHT11_DEVICE_READING;
}

void DHT11SensorAcquisition()
{
    DHT11::DHT11_status_t result;
    DHT11::DHT11_data_t   theDHT11Data;

    // Indicate with the green LED that DHT11 processing is about to begin.
    g_LEDGreen = LED_ON;
    ThisThread::sleep_for(DHT11_DEVICE_USER_OBSERVABILITY_DELAY);
    g_LEDGreen = LED_OFF;

    // DHT11 processing begins. Therefore first release the single-wire 
    // bidirectional bus:
    result = g_DHT11.DHT11_Init();
    if (result != DHT11_SUCCESS)
    {
        Utility::g_STDIOMutex.lock();
        printf("Error! g_DHT11.DHT11_Init() returned: [%d] -> %s\n", 
              result, ToString(result).c_str());
        Utility::g_STDIOMutex.unlock();

        return;
    }

    g_DeviceState = DHT11_DEVICE_WAITING;
    g_LCD16x2.init();

    // Interrupt us every second to wake us up:
    g_PeriodicStateChanger.attach(&AlertToRead, DHT11_DEVICE_STATE_CHANGE_RATE);

    while (1)
    {
        // Power Management (sleep)
        //
        // Sleep mode
        //
        // The system clock to the core stops until a reset or an interrupt 
        // occurs. This eliminates dynamic power that the processor, 
        // memory systems and buses use. This mode maintains the processor,
        // peripheral and memory state, and the peripherals continue to work
        // and can generate interrupts.
        //
        // You can wake up the processor by any internal peripheral interrupt
        // or external pin interrupt.
        sleep();

        if (g_DeviceState == DHT11_DEVICE_READING)
        {
            // Indicate that we are reading from DHT11 with blue LED.
            g_LEDBlue = LED_ON;

            result = g_DHT11.DHT11_GetData(&theDHT11Data);

            if (result != DHT11_SUCCESS)
            {
                Utility::g_STDIOMutex.lock();
                printf("Error! g_DHT11.DHT11_GetData() returned: [%d] -> %s\n", 
                      result, ToString(result).c_str());
                Utility::g_STDIOMutex.unlock();
            }
            else
            {
                g_LCD16x2.clr();

                g_LCD16x2.setCursor(0, 0);
                g_LCD16x2.wtrString("Temp: ");
                g_LCD16x2.wtrNumber(static_cast<float>(theDHT11Data.temperature));
                g_LCD16x2.wtrString(" C");
                g_LCD16x2.setCursor(1, 0);
                g_LCD16x2.wtrString("Humi: ");
                g_LCD16x2.wtrNumber(static_cast<float>(theDHT11Data.humidity));
                g_LCD16x2.wtrString(" % RH");

                // Depend on checksum in trusting data read from DHT11 device.
                std::string((theDHT11Data.checksumStatus == DHT11::DHT11_CHECKSUM_OK) ? "PASSED" : "FAILED") checksum;

                Utility::g_STDIOMutex.lock();
                printf("Temperature: %d\tHumidity : %d%% RH\tChecksum: %s\r\n", theDHT11Data.temperature, theDHT11Data.humidity, checksum.c_str());
                Utility::g_STDIOMutex.unlock();
            }

            g_LEDBlue = LED_OFF;
            g_DeviceState = DHT11_DEVICE_WAITING;
        }
    }
}

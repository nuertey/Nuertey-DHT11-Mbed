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
* @note    Enumerated Peripheral Components Details
* 
*   1) The DHT11 sensor measures and provides humidity and temperature 
*      values serially over a single wire. Its characteristics are as 
*      follows:
*      
*      - It can measure relative humidity in percentages (20 to 90% RH) 
*        and temperature in degree Celsius in the range of 0 to 50°C.
* 
*      - It has 4 pins; one of which is used for data communication in 
*        serial form.
* 
*      - Pulses of different TON and TOFF are decoded as logic 1 or 
*        logic 0 or start pulse or end of frame.
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
*      - It can be used either in 4-bit mode or an 8-bit mode.
* 
*      - For displaying on it, custom characters can be created.
* 
*      - And lastly, the LCD 16x2 has 8 data lines and 3 control lines 
*        that can be used for control purposes.
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
#include "DHT.h"
#include "DHT11.h"
#include "LCD_H.h"
#include "Utilities.h"

#define LED_ON  1
#define LED_OFF 0

static const uint32_t DHT11_DEVICE_USER_OBSERVABILITY_DELAY(3); // 3 milliseconds.
static const float    DHT11_DEVICE_STATE_CHANGE_RATE(2.0);      // 2 milliseconds.

// DHT11 uses a simplified single-wire bidirectional communication protocol.
// It follows a Master/Slave paradigm [NUCLEO-F767ZI=Master, DHT11=Slave] 
// with the MCU observing these states:
//
// WAITING, READING.
//static const uint32_t DHT11_DEVICE_WAITING(0UL);
//static const uint32_t DHT11_DEVICE_READING(1UL);

// DHT11 Sensor Interfacing with ARM MBED. Data communication is single-line
// serial. Note that for STM32 Nucleo-144 boards, the ST Zio connectors 
// are designated by [CN7, CN8, CN9, CN10].
//
// Connector: CN7 
// Pin      : 13 
// Pin Name : D22
// STM32 Pin: PB5
// Signal   : SPI_B_MOSI
//DHT11             g_DHT11(PB_5);
DHT               g_DHT11(PB_5, eType::DHT11);

// LCD 16x2 Interfacing With ARM MBED. LCD 16x2 controlled via the 4-bit
// interface. Note that for STM32 Nucleo-144 boards, the ST Zio connectors 
// are designated by [CN7, CN8, CN9, CN10].
//
// Connector: CN7 
// Pin      : 14 
// Pin Name : D11
// STM32 Pin: PA7
// Signal   : SPI_A_MOSI
//
// Connector: CN7 
// Pin      : 12 
// Pin Name : D12
// STM32 Pin: PA6
// Signal   : SPI_A_MISO
//
// Connector: CN7 
// Pin      : 10 
// Pin Name : D13
// STM32 Pin: PA5
// Signal   : SPI_A_SCK
//
// Connector: CN8 
// Pin      : 14 
// Pin Name : D49
// STM32 Pin: PG2
// Signal   : I/O
//
// Connector: CN8 
// Pin      : 16 
// Pin Name : D50
// STM32 Pin: PG3
// Signal   : I/O
//
// Connector: CN9 
// Pin      : 29 
// Pin Name : D65
// STM32 Pin: PG0
// Signal   : I/O
LCD               g_LCD16x2(PA_7, PA_6, PA_5, PG_2, PG_3, PG_0); // RS, RW, D4, D5, D6, D7
          
// As per my ARM NUCLEO-F767ZI specs:        
DigitalOut        g_LEDGreen(LED1);
DigitalOut        g_LEDBlue(LED2);
DigitalOut        g_LEDRed(LED3);

// Use the Ticker interface to set up a recurring interrupt. It calls a
// function repeatedly and at a specified rate. As we don't need 
// microseconds precision anyway, leverage LowPowerTicker instead. 
// Crucially, that would ensure that we do not block deep sleep mode.
//LowPowerTicker    g_PeriodicStateChanger;

//volatile uint32_t g_DeviceState;     // Indicates when to read in a new sample.

// ==========================================================
// Free-Floating General Helper Functions To Be Used By All :
// ==========================================================
using DHT11StatusCodesMap_t = std::map<eError, std::string>;
using IndexElementDHT11_t   = DHT11StatusCodesMap_t::value_type;

inline static auto make_dht11_error_codes_map()
{
    DHT11StatusCodesMap_t eMap;
    
    eMap.insert(IndexElementDHT11_t(eError::ERROR_NONE, std::string("\"Communication success\"")));
    eMap.insert(IndexElementDHT11_t(eError::BUS_BUSY, std::string("\"Communication failure - bus busy\"")));
    eMap.insert(IndexElementDHT11_t(eError::ERROR_NOT_PRESENT, std::string("\"Communication failure - sensor not present on bus\"")));
    eMap.insert(IndexElementDHT11_t(eError::ERROR_ACK_TOO_LONG, std::string("\"Communication failure - ack too long\"")));
    eMap.insert(IndexElementDHT11_t(eError::ERROR_SYNC_TIMEOUT, std::string("\"Communication failure - sync timeout\"")));
    eMap.insert(IndexElementDHT11_t(eError::ERROR_DATA_TIMEOUT, std::string("\"Communication failure - data timeout\"")));
    eMap.insert(IndexElementDHT11_t(eError::ERROR_CHECKSUM, std::string("\"Checksum error\"")));
    eMap.insert(IndexElementDHT11_t(eError::ERROR_NO_PATIENCE, std::string("\"Communication failure - no patience\"")));
 
    return eMap;
}

static DHT11StatusCodesMap_t gs_DHT11StatusCodesMap = make_dht11_error_codes_map();

inline std::string ToString(const eError & key)
{
    return (gs_DHT11StatusCodesMap.at(key));
}

// ==============================
// Begin Actual Implementations :
// ==============================
//void AlertToRead()
//{
//    g_DeviceState = DHT11_DEVICE_READING;
//}

void DHT11SensorAcquisition()
{
    eError error;
    float h = 0.0f, c = 0.0f, f = 0.0f, k = 0.0f, dp = 0.0f, dpf = 0.0f;
//    DHT11::DHT11_status_t result;
//    DHT11::DHT11_data_t   theDHT11Data;

    // Indicate with the green LED that DHT11 processing is about to begin.
    g_LEDGreen = LED_ON;
    ThisThread::sleep_for(DHT11_DEVICE_USER_OBSERVABILITY_DELAY);
    g_LEDGreen = LED_OFF;

    // DHT11 processing begins. Therefore first release the single-wire 
    // bidirectional bus:
//    result = g_DHT11.DHT11_Init();
//    if (result != DHT11::DHT11_SUCCESS)
//    {
//        Utility::g_STDIOMutex.lock();
//        printf("Error! g_DHT11.DHT11_Init() returned: [%d] -> %s\n", 
//              result, ToString(result).c_str());
//        Utility::g_STDIOMutex.unlock();
//
//        return;
//    }
//
//    g_DeviceState = DHT11_DEVICE_WAITING;
//    g_LCD16x2.init();

    // Interrupt us every second to wake us up:
//    g_PeriodicStateChanger.attach(&AlertToRead, DHT11_DEVICE_STATE_CHANGE_RATE);
    while (1)
    {
        ThisThread::sleep_for(DHT11_DEVICE_STATE_CHANGE_RATE);

        // Indicate that we are reading from DHT11 with blue LED.
        g_LEDBlue = LED_ON;

        error = g_DHT11.readData();
        if (eError::ERROR_NONE == error)
        {
            c   = g_DHT11.ReadTemperature(eScale::CELCIUS);
            f   = g_DHT11.ReadTemperature(eScale::FARENHEIT);
            k   = g_DHT11.ReadTemperature(eScale::KELVIN);
            h   = g_DHT11.ReadHumidity();
            dp  = g_DHT11.CalcdewPoint(c, h);
            dpf = g_DHT11.CalcdewPointFast(c, h);

            g_LCD16x2.clr();

            g_LCD16x2.setCursor(0, 0);
            g_LCD16x2.wtrString("Temp: ");
            g_LCD16x2.wtrNumber(c);
            g_LCD16x2.wtrString(" C");
            g_LCD16x2.setCursor(1, 0);
            g_LCD16x2.wtrString("Humi: ");
            g_LCD16x2.wtrNumber(h);
            g_LCD16x2.wtrString(" % RH");

            Utility::g_STDIOMutex.lock();
            printf("\nTemperature in Kelvin: %4.2f, Celcius: %4.2f, Farenheit %4.2f\n", k, c, f);
            printf("Humidity is %4.2f, Dewpoint: %4.2f, Dewpoint fast: %4.2f\n", h, dp, dpf);
            Utility::g_STDIOMutex.unlock();
        }
        else
        {
            g_LCD16x2.clr();
            g_LCD16x2.setCursor(0, 0);
            g_LCD16x2.wtrString("Error!");

            Utility::g_STDIOMutex.lock();
            printf("Error! g_DHT11.readData() returned: [%d] -> %s\n", 
                      error, ToString(error).c_str());
            Utility::g_STDIOMutex.unlock();
        }

        g_LEDBlue = LED_OFF;
    }
/*
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

            if (result != DHT11::DHT11_SUCCESS)
            {
                g_LCD16x2.clr();
                g_LCD16x2.setCursor(0, 0);
                g_LCD16x2.wtrString("Error!");

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
                std::string checksum = std::string((theDHT11Data.checksumStatus == DHT11::DHT11_CHECKSUM_OK) ? "PASSED" : "FAILED");

                Utility::g_STDIOMutex.lock();
                printf("Temperature: %d\tHumidity : %d%% RH\tChecksum: %s\r\n", theDHT11Data.temperature, theDHT11Data.humidity, checksum.c_str());
                Utility::g_STDIOMutex.unlock();
            }

            g_LEDBlue = LED_OFF;
            g_DeviceState = DHT11_DEVICE_WAITING;
        }
    }
*/
}

// Do not return from main() as in  Embedded Systems, there is nothing
// (conceptually) to return to. A crash will occur otherwise!
int main()
{
    printf("\r\n\r\nNuertey-DHT11-Mbed - Beginning... \r\n\r\n");

    Utility::InitializeGlobalResources();

    printf("\r\n%s\r\n", Utility::g_NetworkInterfaceInfo.c_str());
    printf("\r\n%s\r\n", Utility::g_SystemProfile.c_str());
    printf("\r\n%s\r\n", Utility::g_BaseRegisterValues.c_str());
    printf("\r\n%s\r\n", Utility::g_HeapStatistics.c_str());
    
    Utility::gs_CloudCommunicationsEventIdentifier = Utility::gs_MasterEventQueue.call_in(
                                       CLOUD_COMMUNICATIONS_EVENT_DELAY_MSECS,
                                       DHT11SensorAcquisition);
    if (!Utility::gs_CloudCommunicationsEventIdentifier)
    {
        printf("Error! Not enough memory available to allocate DHT11 Sensor Acquisition event.\r\n");
    }

    // To further save RAM, as we have no other work to do in main() after
    // initialization, we will dispatch the global event queue from here,
    // thus avoiding the need to create a separate dispatch thread (context).

    // The dispatch function provides the context for running
    // of the queue and can take a millisecond timeout to run for a
    // fixed time or to just dispatch any pending events.
    printf("\r\nAbout to dispatch regular and periodic events... \r\n");
    Utility::gs_MasterEventQueue.dispatch_forever();
    
    Utility::g_STDIOMutex.lock();
    printf("\r\nPeriodic events dispatch was EXPECTEDLY broken from somewhere. \r\n");
    Utility::g_STDIOMutex.unlock();

    // As we are done dispatching, reset the event id so that potential
    // callbacks do not attempt to perform actions on the Master Event Queue.
    Utility::gs_CloudCommunicationsEventIdentifier = 0;

    Utility::ReleaseGlobalResources();

    printf("\r\n\r\nNuertey-DHT11-Mbed Application - Done.\r\n\r\n");
}

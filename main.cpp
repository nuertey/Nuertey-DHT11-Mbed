/***********************************************************************
* @file      main.cpp
*
*    An ARM Mbed application that illustrates how a NUCLEO-F767ZI can be 
*    connected to a DHT11 Temperature and Humidity sensor and values 
*    output to an LCD 16x2 display, all mocked-up via a breadboard. This
*    allows us to periodically obtain temperature and humidity readings.
* 
*    Furthermore, the application continuously blinks 3 10mm external LEDs
*    connected to various I/O ports at different frequencies.
* 
*    And lastly, the application employs PWM output pins on the 
*    NUCLEO-F767ZI to dim the 10mm LEDs according to precomputed 
*    triangular and sinusoidal waveform function values. Essentially,  
*    the values of said waveforms are scaled and used as the duty cycle
*    in driving, dimming, and brightening the 10mm LEDs.
*
* @brief   Input: DHT11 temperature/humidity readings. Output: LCD 16x2.
*          Output: various cool LED patterns. 
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
*      - It can be used either in a 4-bit mode or an 8-bit mode.
* 
*      - For displaying on it, custom characters can be created.
* 
*      - And lastly, the LCD 16x2 has 8 data lines and 3 control lines 
*        that can be used for control purposes.
*
* @warning   The I/Os of STM32 NUCLEO-F767ZI are 3.3 V compatible 
*            instead of 5 V for say, the Arduino Uno V3 microcontroller.
*            Hence values for current-limiting resistors placed in series
*            with the external 10mm LEDs must be adjusted accordingly.
*
* @author    Nuertey Odzeyem
* 
* @date      January 5th, 2021
*
* @copyright Copyright (c) 2021 Nuertey Odzeyem. All Rights Reserved.
***********************************************************************/
#include "mbed.h"
#include "DHT.h"
#include "LCD_H.h"
#include "Utilities.h"
#include "waveforms.h"

#define LED_ON  1
#define LED_OFF 0

static const uint32_t DHT11_DEVICE_USER_OBSERVABILITY_DELAY(3); // 3 milliseconds.
static const float    DHT11_DEVICE_STATE_CHANGE_RATE(2.0);      // 2 milliseconds.

// DHT11 uses a simplified single-wire bidirectional communication protocol.
// It follows a Master/Slave paradigm [NUCLEO-F767ZI=Master, DHT11=Slave] 
// with the MCU observing these states:
//
// WAITING, READING.
static const uint32_t DHT11_DEVICE_WAITING(0UL);
static const uint32_t DHT11_DEVICE_READING(1UL);

// DHT11 Sensor Interfacing with ARM MBED. Data communication is single-line
// serial. Note that for STM32 Nucleo-144 boards, the ST Zio connectors 
// are designated by [CN7, CN8, CN9, CN10].
//
// Connector: CN7 
// Pin      : 13 
// Pin Name : D22
// STM32 Pin: PB5
// Signal   : SPI_B_MOSI
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

// =======================================================================
// 10mm LED connections to NUCLEO-F767ZI output pins are documented below:
// =======================================================================

// Connector: CN10
// Pin      : 12 
// Pin Name : D2
// STM32 Pin: PF15 ***
// Signal   : I/O
DigitalOut        g_External10mmLEDGreen(PF_15); // LED Current = 18mA; Voltage Drop = 2.1V; Calculated Resistance = 66.67Ω

// *** CAUTION!!!
// PA7 is used as D11 and connected to CN7 pin 14 by default. If JP6 is ON,
// it is also connected to both Ethernet PHY as RMII_DV and CN9 pin 15. 
// In this case only one function of the Ethernet or D11 must be used.
//
// So choose some other pin here other than CN9, pin 15 (PA7).

// Connector: CN9 
// Pin      : 30 
// Pin Name : D64
// STM32 Pin: PG1
// Signal   : I/O
DigitalOut        g_External10mmLEDYellow(PG_1);// LED Current = 18mA; Voltage Drop = 2.1V; Calculated Resistance = 66.67Ω

// Connector: CN10 
// Pin      : 28 
// Pin Name : D38
// STM32 Pin: PE14
// Signal   : I/O
DigitalOut        g_External10mmLEDRed(PE_14);   // LED Current = 18mA; Voltage Drop = 2.0V; Calculated Resistance = 72.22Ω

// TBD Nuertey Odzeyem; select, connect and document PWM pins.
//PwmOut            g_External10mmLEDYellow(PA_5);
//PwmOut            g_External10mmLEDRed(PA_5);

Thread            g_External10mmLEDThread1;
//Thread            g_External10mmLEDThread2;
//Thread            g_External10mmLEDThread3;
Thread            g_External10mmLEDThread4;
Thread            g_External10mmLEDThread5;

// =========================================================
// Free-Floating General Helper Functions To Be Used By All:
// =========================================================
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

// =============================
// Begin Actual Implementations:
// =============================
struct ExternalLED_t 
{
    ExternalLED_t(DigitalOut * pExternalLEDPin, const uint32_t& timeOn, const uint32_t& timeOff)
        : m_pExternalLEDPin(pExternalLEDPin) // CAUTION! Ensure that the scope of pExternalLEDPin outlasts us.
        , m_TimeOn(timeOn)
        , m_TimeOff(timeOff)
    {
    }
    ExternalLED_t(const ExternalLED_t& otherLED)
    {
        // CAUTION! Ensure that the scope of the external LED pin pointer
        // outlasts the thread which uses this type; and it DOES, since it
        // is really declared as global. Eschew std::move() then and simply
        // re-point our pointer to that global variable address.
        //m_pExternalLEDPin = std::move(otherLED.m_pExternalLEDPin);
        m_pExternalLEDPin = otherLED.m_pExternalLEDPin;
        m_TimeOn = otherLED.m_TimeOn;
        m_TimeOff = otherLED.m_TimeOff;
    }

    DigitalOut *   m_pExternalLEDPin;
    uint32_t       m_TimeOn;
    uint32_t       m_TimeOff;
};

void DHT11SensorAcquisition()
{
    eError result;
    float h = 0.0f, c = 0.0f, f = 0.0f, k = 0.0f, dp = 0.0f, dpf = 0.0f;

    // Indicate with the green LED that DHT11 processing is about to begin.
    g_LEDGreen = LED_ON;

    g_LCD16x2.clr();
    g_LCD16x2.setCursor(0, 0);
    g_LCD16x2.wtrString("NUERTEY ODZEYEM");
    g_LCD16x2.setCursor(1, 0);
    g_LCD16x2.wtrString("DHT11 with NUCLEO-F767ZI");

    ThisThread::sleep_for(DHT11_DEVICE_USER_OBSERVABILITY_DELAY);

    g_LEDGreen = LED_OFF;

    while (1)
    {
        ThisThread::sleep_for(DHT11_DEVICE_STATE_CHANGE_RATE);

        // Indicate that we are reading from DHT11 with blue LED.
        g_LEDBlue = LED_ON;

        result = g_DHT11.readData();
        if (eError::ERROR_NONE == result)
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
                      Utility::ToUnderlyingType(result), ToString(result).c_str());
            Utility::g_STDIOMutex.unlock();
        }

        g_LEDBlue = LED_OFF;
    }
}

void LEDBlinker(ExternalLED_t * pExternalLED)
{
    while (true) 
    {
        *(pExternalLED->m_pExternalLEDPin) = LED_ON;
        ThisThread::sleep_for(pExternalLED->m_TimeOn);
        *(pExternalLED->m_pExternalLEDPin) = LED_OFF;
        ThisThread::sleep_for(pExternalLED->m_TimeOff);
    }
}

void LEDTriangularWave(PwmOut * pExternalLEDPin)
{
    auto result = std::max_element(g_TriangleWaveform, g_TriangleWaveform + NUMBER_OF_TRIANGULAR_SAMPLES);
    for (auto & dutyCycle : g_TriangleWaveform) 
    {
        float scaledDutyCycle = (dutyCycle/(*result));
        *pExternalLEDPin = scaledDutyCycle;
        ThisThread::sleep_for(200);
    }
}

void LEDSinusoidalWave(PwmOut * pExternalLEDPin)
{
    auto result = std::max_element(g_SineWaveform, g_SineWaveform + NUMBER_OF_SINUSOID_SAMPLES);
    for (auto & dutyCycle : g_SineWaveform) 
    {
        float scaledDutyCycle = (dutyCycle/(*result));
        *pExternalLEDPin = scaledDutyCycle;
        ThisThread::sleep_for(200);
    }
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

    // Spawns three threads so as to blink the three large (10mm) externally
    // connected LEDs at various frequencies. Note to connect said LEDs
    // in series with an appropriate resistor (60 - 100 ohms?) as the 
    // I/Os of STM32 NUCLEO-F767ZI are only 3.3 V compatible ... whereas 
    // the LEDs are high-powered (5V)? Forward voltage? Forward operating current?

    // It seems that Mbed Callback class can only take in one argument.
    // Not to worry, we will improvise with an aggregate class type.
    ExternalLED_t external10mmLEDGreen(&g_External10mmLEDGreen, 100, 100);
    ExternalLED_t external10mmLEDYellow(&g_External10mmLEDYellow, 200, 100);
    ExternalLED_t external10mmLEDRed(&g_External10mmLEDRed, 500, 200);

    g_External10mmLEDThread1.start(callback(LEDBlinker, &external10mmLEDGreen));
    //g_External10mmLEDThread2.start(callback(LEDTriangularWave, &g_External10mmLEDYellow));
    //g_External10mmLEDThread3.start(callback(LEDSinusoidalWave, &g_External10mmLEDRed));

    // Forget not proper thread joins:
    //g_External10mmLEDThread2.join();
    //g_External10mmLEDThread3.join();

    g_External10mmLEDThread4.start(callback(LEDBlinker, &external10mmLEDYellow));
    g_External10mmLEDThread5.start(callback(LEDBlinker, &external10mmLEDRed));

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

    // Forget not proper thread joins:
    g_External10mmLEDThread1.join();
    g_External10mmLEDThread4.join();
    g_External10mmLEDThread5.join();

    Utility::ReleaseGlobalResources();

    printf("\r\n\r\nNuertey-DHT11-Mbed Application - Done.\r\n\r\n");
}

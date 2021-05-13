/***********************************************************************
* @file      main.cpp
*
*    An ARM Mbed application that illustrates how a NUCLEO-F767ZI can be 
*    connected to a DHT11 Temperature and Humidity sensor and values 
*    output to an LCD 16x2 display, all mocked-up via a breadboard. This
*    allows us to periodically obtain temperature and humidity readings.
* 
*    Furthermore, the application continuously blinks 3 10mm external 
*    LEDs connected to various I/O ports at different frequencies.
* 
*    And lastly, the application employs PWM output pins on the 
*    NUCLEO-F767ZI to vary the intensity of the 10mm LEDs according to 
*    an on-the-fly calculated sawtooth waveform pattern and, precomputed 
*    triangular and sinusoidal waveform function values. Essentially,  
*    the values of said waveforms are scaled and used as the duty cycle
*    in driving, dimming, and brightening the 10mm LEDs.
*
* @brief   Input: DHT11 temperature/humidity readings. Output: LCD 16x2.
*          Output: Various cool LED patterns of continuously changing
*                  intensity. 
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
* @warning   The I/O pins of STM32 NUCLEO-F767ZI are 3.3 V compatible 
*            instead of 5 V for say, the Arduino Uno V3 microcontroller.
*            Hence values for current-limiting resistors placed in series
*            with the external 10mm LEDs must be adjusted accordingly.
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
#include "mbed.h"
#include "NuerteyDHT11Device.h"
#include "LCD_H.h"
#include "Utilities.h"
#include "waveforms.h"
//#include "NuerteyMQTTClient.h"

#define LED_ON  1
#define LED_OFF 0

// Laptop running Mosquitto MQTT Broker/Server hosted on Ubuntu Linux
// outward-facing IP address. Port is particular to the MQTT protocol.
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("96.68.46.185");

// MQTT Broker IP on local LAN gives better results than outward-facing IP. 
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("10.50.10.25");
//static const uint16_t    NUERTEY_MQTT_BROKER_PORT(1883);

// As we are constrained on embedded, prefer to send many topics with
// smaller payloads than one topic with a giant payload. This will also
// ensure that we do not run into hard limits such as the 512 string
// literal byte limit after concatenation (509 + terminators).
//static const char * NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1 = "/Nuertey/Nucleo/F767ZI/Temperature";
//static const char * NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2 = "/Nuertey/Nucleo/F767ZI/Humidity";

static constexpr uint32_t DHT11_DEVICE_USER_OBSERVABILITY_DELAY(3); // 3 milliseconds.
static constexpr uint32_t DHT11_DEVICE_STABLE_STATUS_DELAY(1000);   // 1 second.
static constexpr uint32_t DHT11_DEVICE_SAMPLING_PERIOD(3000);       // 3 seconds.

// DHT11 Sensor Interfacing with ARM MBED. Data communication is single-line
// serial. Note that for STM32 Nucleo-144 boards, the ST Zio connectors 
// are designated by [CN7, CN8, CN9, CN10]. 
//
// From prior successful testing of DHT11 on Arduino Uno, and matching 
// up the specific pins on Arduino with the "Arduino Support" section of
// the STM32 Zio connectors, I isolated the following pin as the 
// Arduino-equivalent data pin:
//
// Connector: CN10 
// Pin      : 10 
// Pin Name : D3        * Arduino-equivalent pin name
// STM32 Pin: PE13
// Signal   : TIMER_A_PWM3
NuerteyDHT11Device<DHT11_t> g_DHT11(PE_13);

// LCD 16x2 Interfacing With ARM MBED. LCD 16x2 controlled via the 4-bit
// interface. Note that for STM32 Nucleo-144 boards, the ST Zio connectors 
// are designated by [CN7, CN8, CN9, CN10].
//
// From prior successful testing of the LCD on Arduino Uno, and matching 
// up the specific pins on Arduino with the "Arduino Support" section of
// the STM32 Zio connectors, I isolated the following pins as the 
// Arduino-equivalent pins for the LCD_16x2 library:
//
// Connector: CN7 
// Pin      : 18 
// Pin Name : D9        * Arduino-equivalent pin name
// STM32 Pin: PD15
// Signal   : TIMER_B_PWM2
//
// Connector: CN7 
// Pin      : 16 
// Pin Name : D10       * Arduino-equivalent pin name
// STM32 Pin: PD14
// Signal   : SPI_A_CS/TIM_B_PWM3
//
// Connector: CN7 
// Pin      : 14 
// Pin Name : D11       * Arduino-equivalent pin name
// STM32 Pin: PA7
// Signal   : SPI_A_MOSI/TIM_E_PWM1
//
// Connector: CN7 
// Pin      : 12 
// Pin Name : D12       * Arduino-equivalent pin name
// STM32 Pin: PA6
// Signal   : SPI_A_MISO 
//
// Connector: CN10 
// Pin      : 2 
// Pin Name : D7        * Arduino-equivalent pin name
// STM32 Pin: PF13
// Signal   : I/O
//
// Connector: CN7 
// Pin      : 20 
// Pin Name : D8        * Arduino-equivalent pin name
// STM32 Pin: PF12
// Signal   : I/O
LCD g_LCD16x2(PD_15, PD_14, PA_7, PA_6, PF_13, PF_12); // LCD designated pins: D4, D5, D6, D7, RS, E
          
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

// =====================================================================
// NUCLEO-F767ZI PWM pins/connections to 10mm LEDs are documented below:
// =====================================================================

// Connector: CN7 
// Pin      : 18 
// Pin Name : D9
// STM32 Pin: PD15
// Signal   : TIMER_B_PWM2
PwmOut            g_ExternalPWMLEDGreen(PD_15);

// Connector: CN10 
// Pin      : 29 
// Pin Name : D32
// STM32 Pin: PA0
// Signal   : TIMER_C_PWM1
PwmOut            g_ExternalPWMLEDYellow(PA_0);

// Connector: CN10 
// Pin      : 31 
// Pin Name : D33
// STM32 Pin: PB0
// Signal   : TIMER_D_PWM1
PwmOut            g_ExternalPWMLEDRed(PB_0);

Thread            g_External10mmLEDThread1;
Thread            g_External10mmLEDThread2;
Thread            g_External10mmLEDThread3;
Thread            g_External10mmLEDThread4;
Thread            g_External10mmLEDThread5;

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
//    NuerteyMQTTClient theMQTTClient(&Utility::g_EthernetInterface, 
//                                    NUERTEY_MQTT_BROKER_ADDRESS,
//                                    NUERTEY_MQTT_BROKER_PORT);

    // Indicate with the green LED that DHT11 processing is about to begin.
    g_LEDGreen = LED_ON;
    g_LCD16x2.clr();
    g_LCD16x2.setCursor(0, 0);
    g_LCD16x2.wtrString("NUERTEY ODZEYEM");
    g_LCD16x2.setCursor(1, 0);
    g_LCD16x2.wtrString("NUCLEO-F767ZI");
    ThisThread::sleep_for(DHT11_DEVICE_USER_OBSERVABILITY_DELAY);
    g_LEDGreen = LED_OFF;

    // Indicate with the blue LED that MQTT network initialization is ongoing.
    g_LEDBlue = LED_ON;
//    bool retVal = theMQTTClient.Connect();
    bool retVal = true;

    if (retVal)
    {
        // This echo back from the server is NOT just for our peace of mind, 
        // NOT just to ensure that publishing did in fact get to the server/broker. 
        // It is also to ensure that the internal design of NuerteyMQTTClient
        // with the invocation of ::Yield() after every publish, is happy.
        // That design pattern is mandated by the embedded MQTT library to
        // facilitate context switching. Hence subscribe to every topic you
        // aim to publish.
        //theMQTTClient.Subscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1);
        //theMQTTClient.Subscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2);
        g_LEDBlue = LED_OFF;

        ThisThread::sleep_for(DHT11_DEVICE_STABLE_STATUS_DELAY);

        while (1)
        {
            // Indicate that we are reading from DHT11 with green LED.
            g_LEDGreen = LED_ON;

            auto result = g_DHT11.ReadData();
            if (!result)
            {
                // Clear red LED indicating previous error.
                g_LEDRed = LED_OFF;

                auto h = 0.0f, c = 0.0f, f = 0.0f, k = 0.0f, dp = 0.0f, dpf = 0.0f;

                c   = g_DHT11.GetTemperature(TemperatureScale_t::CELCIUS);
                f   = g_DHT11.GetTemperature(TemperatureScale_t::FARENHEIT);
                k   = g_DHT11.GetTemperature(TemperatureScale_t::KELVIN);
                h   = g_DHT11.GetHumidity();
                dp  = g_DHT11.CalculateDewPoint(c, h);
                dpf = g_DHT11.CalculateDewPointFast(c, h);

                g_LCD16x2.clr();
                g_LCD16x2.setCursor(0, 0);
                g_LCD16x2.wtrString("Temp: ");
                g_LCD16x2.wtrNumber(f);
                g_LCD16x2.wtrString(" F");
                g_LCD16x2.setCursor(1, 0);
                g_LCD16x2.wtrString("Humi: ");
                g_LCD16x2.wtrNumber(h);
                g_LCD16x2.wtrString(" % RH");

                Utility::g_STDIOMutex.lock();
                printf("\nTemperature in Kelvin: %4.2fK, Celcius: %4.2f°C, Farenheit %4.2f°F\n", k, c, f);
                printf("Humidity is %4.2f, Dewpoint: %4.2f, Dewpoint fast: %4.2f\n", h, dp, dpf);
                Utility::g_STDIOMutex.unlock();

                // Indicate that publishing is about to commence with the blue LED.
                g_LEDBlue = LED_ON;
                
//                std::string sensorTemperature = std::to_string(c);
//                theMQTTClient.Publish(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1, 
//                                       (void *)sensorTemperature.c_str(), 
//                                       sensorTemperature.size() + 1);  
//
//                std::string sensorHumidity = std::to_string(h);
//                theMQTTClient.Publish(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2, 
//                                       (void *)sensorHumidity.c_str(), 
//                                       sensorHumidity.size() + 1);  
//                
                // Indicate that publishing was successful and a message was 
                // received in response by turning off the blue LED.
                g_LEDBlue = LED_OFF; 
            }
            else
            {
                // Indicate with the red LED that an error occurred.
                g_LEDRed = LED_ON;

                g_LCD16x2.clr();
                g_LCD16x2.setCursor(0, 0);
                g_LCD16x2.wtrString("Sensor Error!");

                Utility::g_STDIOMutex.lock();
                printf("Error! g_DHT11.ReadData() returned: [%d] -> %s\n", 
                      result.value(), result.message().c_str());
                Utility::g_STDIOMutex.unlock();
            }

            g_LEDGreen = LED_OFF;
            // Per datasheet/device specifications:
            //
            // "Sampling period：Secondary Greater than 2 seconds"
            ThisThread::sleep_for(DHT11_DEVICE_SAMPLING_PERIOD);
        }

        // Indicate with the blue LED that MQTT network de-initialization is ongoing.
        g_LEDBlue = LED_ON;

//        theMQTTClient.UnSubscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1);
//        theMQTTClient.UnSubscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2);

        // Bring down the MQTT session.
//        theMQTTClient.Disconnect();
        
        g_LEDBlue = LED_OFF;
    }
    else
    {
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

void LEDSawToothWave(PwmOut * pExternalLEDPin)
{
    while (1)
    {
        // Gradually change the intensity of the LED according to the
        // saw-tooth waveform pattern.
        *pExternalLEDPin = *pExternalLEDPin + 0.01;
        ThisThread::sleep_for(200);

        // Set the output duty-cycle, specified as a percentage (float)
        //
        // Parameters
        //    value A floating-point value representing the output duty-cycle, 
        //    specified as a percentage. The value should lie between 0.0f 
        //    (representing on 0%) and 1.0f (representing on 100%). Values 
        //    outside this range will be saturated to 0.0f or 1.0f.
        if (*pExternalLEDPin == 1.0)
        {
            *pExternalLEDPin = 0;
        }
    }
}

void LEDTriangularWave(PwmOut * pExternalLEDPin)
{
    auto result = std::max_element(g_TriangleWaveform, g_TriangleWaveform + NUMBER_OF_TRIANGULAR_SAMPLES);
    for (auto & dutyCycle : g_TriangleWaveform) 
    {
        // Set the output duty-cycle, specified as a percentage (float)
        //
        // Parameters
        //    value A floating-point value representing the output duty-cycle, 
        //    specified as a percentage. The value should lie between 0.0f 
        //    (representing on 0%) and 1.0f (representing on 100%). Values 
        //    outside this range will be saturated to 0.0f or 1.0f.
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
        // Set the output duty-cycle, specified as a percentage (float)
        //
        // Parameters
        //    value A floating-point value representing the output duty-cycle, 
        //    specified as a percentage. The value should lie between 0.0f 
        //    (representing on 0%) and 1.0f (representing on 100%). Values 
        //    outside this range will be saturated to 0.0f or 1.0f.
        float scaledDutyCycle = (dutyCycle/(*result));
        *pExternalLEDPin = scaledDutyCycle;
        ThisThread::sleep_for(40);
    }
}

// Do not return from main() as in  Embedded Systems, there is nothing
// (conceptually) to return to. A crash will occur otherwise!
int main()
{
    printf("\r\n\r\nNuertey-DHT11-Mbed - Beginning... \r\n\r\n");

//    Utility::InitializeGlobalResources();
//
//    printf("\r\n%s\r\n", Utility::g_NetworkInterfaceInfo.c_str());
//    printf("\r\n%s\r\n", Utility::g_SystemProfile.c_str());
//    printf("\r\n%s\r\n", Utility::g_BaseRegisterValues.c_str());
//    printf("\r\n%s\r\n", Utility::g_HeapStatistics.c_str());

    // Spawns three threads so as to blink the three large (10mm) externally
    // connected LEDs at various frequencies. Note to connect said LEDs
    // in series with an appropriate resistor (60 - 100 ohms?) as the 
    // I/Os of STM32 NUCLEO-F767ZI are only 3.3 V compatible ... whereas 
    // the LEDs are high-powered (5V)? Forward voltage? Forward operating current?

    // It seems that Mbed Callback class can only take in one argument.
    // Not to worry, we will improvise with an aggregate class type.
    //ExternalLED_t external10mmLEDGreen(&g_External10mmLEDGreen, 100, 100);
    //ExternalLED_t external10mmLEDYellow(&g_External10mmLEDYellow, 200, 100);
    //ExternalLED_t external10mmLEDRed(&g_External10mmLEDRed, 500, 200);

    //g_External10mmLEDThread1.start(callback(LEDBlinker, &external10mmLEDGreen));
    //g_External10mmLEDThread1.start(callback(LEDSawToothWave, &g_ExternalPWMLEDGreen));
    //g_External10mmLEDThread2.start(callback(LEDTriangularWave, &g_ExternalPWMLEDYellow));
    //g_External10mmLEDThread3.start(callback(LEDSinusoidalWave, &g_ExternalPWMLEDRed));

    // Forget not proper thread joins:
    //g_External10mmLEDThread2.join();
    //g_External10mmLEDThread3.join();

    //g_External10mmLEDThread4.start(callback(LEDBlinker, &external10mmLEDYellow));
    //g_External10mmLEDThread5.start(callback(LEDBlinker, &external10mmLEDRed));

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
    //g_External10mmLEDThread1.join();
    //g_External10mmLEDThread4.join();
    //g_External10mmLEDThread5.join();

//    Utility::ReleaseGlobalResources();

    printf("\r\n\r\nNuertey-DHT11-Mbed Application - Done.\r\n\r\n");
}

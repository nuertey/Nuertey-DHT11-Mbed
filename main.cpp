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
#include "mbed.h"
#include "NuerteyDHT11Device.h"
#include "LCD.h"
#include "Utilities.h"
#include "NuerteyMQTTClient.h"

#define LED_ON  1
#define LED_OFF 0

// Special chars meant to show the capabilities of the LCD 16x2 display.
uint8_t upArrow[8] =
{
    0b00100,
    0b01010,
    0b10001,
    0b00100,
    0b00100,
    0b00100,
    0b00000,
};

uint8_t downArrow[8] =
{
    0b00000,
    0b00100,
    0b00100,
    0b00100,
    0b10001,
    0b01010,
    0b00100,
};

uint8_t rightArrow[8] =
{
    0b00000,
    0b00100,
    0b00010,
    0b11001,
    0b00010,
    0b00100,
    0b00000,
};

uint8_t leftArrow[8] =
{
    0b00000,
    0b00100,
    0b01000,
    0b10011,
    0b01000,
    0b00100,
    0b00000,
};

// Laptop running Mosquitto MQTT Broker/Server hosted on Ubuntu Linux
// outward-facing IP address. Port is particular to the MQTT protocol.
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("96.68.46.185"); // 96-68-46-185-static.hfc.comcastbusiness.net
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("fe80::7e98:14d8:418a:dca8"); // IPv6 Local address

// test.mosquitto.org
static const std::string NUERTEY_MQTT_BROKER_ADDRESS("test.mosquitto.org");

// MQTT Broker IP on local LAN gives better results than outward-facing IP. 
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("10.50.10.25");
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("10.42.0.1");
static const uint16_t    NUERTEY_MQTT_BROKER_PORT(1883);

// As we are constrained on embedded, prefer to send many topics with
// smaller payloads than one topic with a giant payload. This will also
// ensure that we do not run into hard limits such as the 512 string
// literal byte limit after concatenation (509 + terminators).
static const char * NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1 = "/Nuertey/Nucleo/F767ZI/Temperature";
static const char * NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2 = "/Nuertey/Nucleo/F767ZI/Humidity";

static constexpr uint32_t DHT11_DEVICE_USER_OBSERVABILITY_DELAY(2000); // 2 seconds.
static constexpr uint32_t DHT11_DEVICE_STABLE_STATUS_DELAY(1000);      // 1 second.
static constexpr uint32_t DHT11_DEVICE_SAMPLING_PERIOD(3000);          // 3 seconds.

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
// Connector: CN7 
// Pin      : 10 
// Pin Name : D13        * Arduino-equivalent pin name
// STM32 Pin: PA5
// Signal   : SPI_A_SCK
//
// Connector: CN7 
// Pin      : 4 
// Pin Name : D14        * Arduino-equivalent pin name
// STM32 Pin: PB9
// Signal   : I2C_A_SDA
//
// Connector: CN7 
// Pin      : 2 
// Pin Name : D15        * Arduino-equivalent pin name
// STM32 Pin: PB8
// Signal   : I2C_A_SCL
//
//LCD g_LCD16x2(D10, D11, D12, D13, D14, D15, LCD16x2); // LCD designated pins: RS, E, D4, D5, D6, D7, LCD type
        
// As per my ARM NUCLEO-F767ZI specs:        
DigitalOut        g_LEDGreen(LED1);
DigitalOut        g_LEDBlue(LED2);
DigitalOut        g_LEDRed(LED3);

void PrepareRow1(LCD * pLCD16x2)
{
    pLCD16x2->create(0, downArrow);
    pLCD16x2->create(1, upArrow);
    pLCD16x2->create(2, rightArrow);
    pLCD16x2->create(3, leftArrow);
            
    pLCD16x2->cls();
    pLCD16x2->locate(0, 0);
}

void PrepareRow2(LCD * pLCD16x2)
{
    pLCD16x2->character(0, 1, 0);
    pLCD16x2->character(3, 1, 1);
    pLCD16x2->character(5, 1, 2);
    pLCD16x2->character(7, 1, 3);
            
    pLCD16x2->locate(0, 1);
}

/*
void DisplayLCDCapabilities()
{
    g_LCD16x2.create(0, downArrow);
    g_LCD16x2.create(1, upArrow);
    g_LCD16x2.create(2, rightArrow);
    g_LCD16x2.create(3, leftArrow);

    g_LCD16x2.cls();
    g_LCD16x2.locate(0, 0);
    g_LCD16x2.printf("NUCLEO-F767ZI\n");
    g_LCD16x2.character(0, 1, 0);
    g_LCD16x2.character(3, 1, 1);
    g_LCD16x2.character(5, 1, 2);
    g_LCD16x2.character(7, 1, 3);

    //ThisThread::sleep_for(2000);
    //g_LCD16x2.cls();
    g_LCD16x2.locate(0, 1);
    g_LCD16x2.printf("NUERTEY ODZEYEM\n");

    //ThisThread::sleep_for(2000);
    //g_LCD16x2.display(DISPLAY_OFF);
    //ThisThread::sleep_for(2000);
    //g_LCD16x2.display(DISPLAY_ON);
    //ThisThread::sleep_for(2000);
    //g_LCD16x2.display(CURSOR_ON);
    //ThisThread::sleep_for(2000);
    //g_LCD16x2.display(BLINK_ON);
    //ThisThread::sleep_for(2000);
    //g_LCD16x2.display(BLINK_OFF);
    //ThisThread::sleep_for(2000);
    //g_LCD16x2.display(CURSOR_OFF);
    //
    //for (uint8_t nuert = 0; nuert < 1; nuert++)
    //{
    //    for (uint8_t pos = 0; pos < 13; pos++)
    //    {
    //        // scroll one position to left
    //        g_LCD16x2.display(SCROLL_LEFT);
    //        // step time
    //        ThisThread::sleep_for(500);
    //    }
    //
    //    // scroll 29 positions (string length + display length) to the right
    //    // to move it offscreen right
    //    for (uint8_t pos = 0; pos < 29; pos++)
    //    {
    //        // scroll one position to right
    //        g_LCD16x2.display(SCROLL_RIGHT);
    //        // step time
    //        ThisThread::sleep_for(500);
    //    }
    //
    //    // scroll 16 positions (display length + string length) to the left
    //    // to move it back to center
    //    for (uint8_t pos = 0; pos < 16; pos++)
    //    {
    //        // scroll one position to left
    //        g_LCD16x2.display(SCROLL_LEFT);
    //        // step time
    //        ThisThread::sleep_for(500);
    //    }
    //
    //    ThisThread::sleep_for(1000);
    //}
}*/

void DHT11SensorAcquisition()
{
    static bool initial_run = true;

    NuerteyMQTTClient g_TheMQTTClient(Utility::g_pNetworkInterface, 
                                    NUERTEY_MQTT_BROKER_ADDRESS,
                                    NUERTEY_MQTT_BROKER_PORT);
                              
    //ThisThread::sleep_for(DHT11_DEVICE_STABLE_STATUS_DELAY);

    while (1)
    {
        // Let us see if a local instantiation of the LCD driver each 
        // iteration might give more better and consistent results. 
        LCD theLCD16x2(D10, D11, D12, D13, D14, D15, LCD16x2); // LCD designated pins: RS, E, D4, D5, D6, D7, LCD type
        PrepareRow1(&theLCD16x2);
        
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
            dp  = g_DHT11.CalculateDewPoint(f, h);
            dpf = g_DHT11.CalculateDewPointFast(f, h);

            //g_LCD16x2.cls();
            //g_LCD16x2.locate(0, 0); // column, row
            //g_LCD16x2.printf("Temp: %4.2f F", f); // TBD perhaps convert all to text with sprintf and then write.
            std::string tempString = Utility::TemperatureToString(f);
            //g_LCD16x2.printf(tempString.c_str());
            //g_LCD16x2.locate(0, 1); // column, row
            //g_LCD16x2.printf("Humi: %4.2f %% RH", h);// TBD perhaps convert all to text with sprintf and then write.
            std::string humiString = Utility::HumidityToString(h);
            //g_LCD16x2.printf(humiString.c_str());
            
            theLCD16x2.printf(tempString.c_str());
            PrepareRow2(&theLCD16x2);
            theLCD16x2.printf(humiString.c_str());

            Utility::g_STDIOMutex.lock();
            printf("\nAdapted Temperature String:\n%s", tempString.c_str());
            printf("\nAdapted Humidity String:\n%s\n", humiString.c_str());
            printf("\nTemperature in Kelvin: %4.2fK, Celcius: %4.2f°C, Farenheit %4.2f°F\n", k, c, f);
            printf("Humidity is %4.2f, Dewpoint: %4.2f, Dewpoint fast: %4.2f\n", h, dp, dpf);
            Utility::g_STDIOMutex.unlock();

            std::string sensorTemperature = Utility::TruncateAndToString<float>(f, 2);
            std::string sensorHumidity = Utility::TruncateAndToString<float>(h, 2);
            
            // Indicate that publishing is about to commence with the blue LED.
            g_LEDBlue = LED_ON;

            if (initial_run)
            {
                bool retVal = g_TheMQTTClient.Connect();

                if (retVal)
                {
                    initial_run = false;
                    
                    // This echo back from the server is NOT just for our peace of mind, 
                    // NOT just to ensure that publishing did in fact get to the server/broker. 
                    // It is also to ensure that the internal design of NuerteyMQTTClient
                    // with the invocation of ::Yield() after every publish, is happy.
                    // That design pattern is mandated by the embedded MQTT library to
                    // facilitate context switching. Hence subscribe to every topic you
                    // aim to publish.
                    g_TheMQTTClient.Subscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1);
                    g_TheMQTTClient.Subscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2);
                    
                    Utility::g_STDIOMutex.lock();
                    printf("\nSuccessfully connected to MQTT Broker/Server on initiial_run and subscribed to topis:->\n\t%s\n\t%s", 
                           NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1, NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2);
                    Utility::g_STDIOMutex.unlock();
                }
                else
                {
                    Utility::g_STDIOMutex.lock();
                    printf("\nFailed to connect to MQTT Broker/Server on initiial_run :-> %s:%d", 
                           NUERTEY_MQTT_BROKER_ADDRESS.c_str(), NUERTEY_MQTT_BROKER_PORT);
                    Utility::g_STDIOMutex.unlock();
                }
            }
            
            // CAUTION: Per the Paho MQTT library's behavior, the 3rd size parameter to Publish()
            // must match the 2nd c_str parameter exactly!, not more nor less, for the peer receiving
            // side to be able to decode the MQTT payload successfully. If, for example, one attempts
            // to over-compensate by, say, increasing size by 1 in order to account for some aberrant 
            // null-termination, the received MQTT payload would have an extra "\x00" at the tail-end,
            // which would cause the payload decoding by the peer to fail (at least on Python 3.7). 
            g_TheMQTTClient.Publish(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1, 
                                   (void *)sensorTemperature.c_str(), 
                                   sensorTemperature.size());  

            g_TheMQTTClient.Publish(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2, 
                                   (void *)sensorHumidity.c_str(), 
                                   sensorHumidity.size());  
            
            // Indicate that publishing was successful and a message was 
            // received in response by turning off the blue LED.
            g_LEDBlue = LED_OFF; 
        }
        else
        {
            // Indicate with the red LED that an error occurred.
            g_LEDRed = LED_ON;

            //g_LCD16x2.cls(); // Also implicitly locates to (0, 0).
            //g_LCD16x2.printf("Error Sensor!"); // TBD, does it wraparound?
            theLCD16x2.printf("Error Sensor!");

            Utility::g_STDIOMutex.lock();
            printf("Error! g_DHT11.ReadData() returned: [%d] -> %s\n", 
                  result.value(), result.message().c_str());
            Utility::g_STDIOMutex.unlock();
        }

        g_LEDGreen = LED_OFF;
        // Per device datasheet specifications:
        //
        // "Sampling period：Secondary Greater than 2 seconds"
        ThisThread::sleep_for(DHT11_DEVICE_SAMPLING_PERIOD);
    }

    // Indicate with the blue LED that MQTT network de-initialization is ongoing.
    g_LEDBlue = LED_ON;

    g_TheMQTTClient.UnSubscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1);
    g_TheMQTTClient.UnSubscribe(NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2);

    // Bring down the MQTT session.
    g_TheMQTTClient.Disconnect();
    
    g_LEDBlue = LED_OFF;
}

// Do not return from main() as in  Embedded Systems, there is nothing
// (conceptually) to return to. A crash will occur otherwise!
int main()
{
    printf("\r\n\r\nNuertey-DHT11-Mbed - Beginning... \r\n\r\n");

    //DisplayLCDCapabilities();

    if (Utility::InitializeGlobalResources())
    {
        printf("\r\n%s\r\n", Utility::g_NetworkInterfaceInfo.c_str());
        printf("\r\n%s\r\n", Utility::g_SystemProfile.c_str());
        printf("\r\n%s\r\n", Utility::g_BaseRegisterValues.c_str());
        printf("\r\n%s\r\n", Utility::g_HeapStatistics.c_str());

        DHT11SensorAcquisition();

        Utility::ReleaseGlobalResources();
    }
    else
    {
        printf("\r\n\r\nError! Initialization of Global Resources Failed!\n");
    }

    printf("\r\n\r\nNuertey-DHT11-Mbed Application - Exiting.\r\n\r\n");
}

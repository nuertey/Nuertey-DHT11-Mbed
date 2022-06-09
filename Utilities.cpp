/****************************************************************************
* @file
*
* Assays... i.e. keeping busy and trying out new ideas.
*
* @note
*
* @warning
*
*  Created: October 16, 2018
*   Author: Nuertey Odzeyem
******************************************************************************/
#include "Utilities.h"
#include "NuerteyDHT11Device.h"
#include "LCD.h"
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
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("test.mosquitto.org");
// Also "broker.hivemq.com" can be targetted for testing. 

// MQTT Broker IP on local LAN gives better results than outward-facing IP. 
//static const std::string NUERTEY_MQTT_BROKER_ADDRESS("10.50.10.25");
static const std::string NUERTEY_MQTT_BROKER_ADDRESS("10.42.0.1");
static const uint16_t    NUERTEY_MQTT_BROKER_PORT(1883);

// As we are constrained on embedded, prefer to send many topics with
// smaller payloads than one topic with a giant payload. This will also
// ensure that we do not run into hard limits such as the 512 string
// literal byte limit after concatenation (509 + terminators).
static const char * NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC1 = "/Nuertey/Nucleo/F767ZI/Temperature";
static const char * NUCLEO_F767ZI_DHT11_IOT_MQTT_TOPIC2 = "/Nuertey/Nucleo/F767ZI/Humidity";

static constexpr MilliSecs_t DHT11_DEVICE_USER_OBSERVABILITY_DELAY = 2000ms; // 2 seconds.
static constexpr MilliSecs_t DHT11_DEVICE_STABLE_STATUS_DELAY      = 1000ms; // 1 second.
static constexpr MilliSecs_t DHT11_DEVICE_SAMPLING_PERIOD          = 3000ms; // 3 seconds.

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

namespace Utility
{
    // System identification and composed statistics variables.
    std::string                      g_NetworkInterfaceInfo;
    std::string                      g_SystemProfile;
    std::string                      g_BaseRegisterValues;
    std::string                      g_HeapStatistics;

    EventQueue *                     g_pMasterEventQueue = mbed_event_queue();

    size_t                           g_MessageLength = 0;
    std::unique_ptr<MQTT::Message>   g_pMessage; // MQTT messages' lifeline must last until yield occurs for actual transmission.

    // Protect the platform STDIO object so it is shared politely between 
    // threads, periodic events and periodic callbacks (not in IRQ context
    // but when safely translated into the EventQueue context). Essentially,
    // ensure our output does not come out garbled on the serial terminal.
    PlatformMutex                    g_STDIOMutex; 
    EthernetInterface                g_EthernetInterface;
    NetworkInterface *               g_pNetworkInterface;
    
    TCPSocket                        m_TheSocket{}; // This must definitely precede the MQTT client.
    SocketAddress                    m_TheSocketAddress{};
    
    NuerteyNTPClient                 g_NTPClient(&g_EthernetInterface);

    void NetworkStatusCallback(nsapi_event_t status, intptr_t param)
    {
        assert(status == NSAPI_EVENT_CONNECTION_STATUS_CHANGE);

        g_STDIOMutex.lock();
        printf("Network Connection status changed!\r\n");

        switch (param)
        {
            case NSAPI_STATUS_LOCAL_UP:
            {
                printf("Local IP address set!\r\n");
                g_STDIOMutex.unlock();
                break;
            }
            case NSAPI_STATUS_GLOBAL_UP:
            {
                printf("Global IP address set!\r\n");
                g_STDIOMutex.unlock();
                
                g_pMasterEventQueue->call_in(20ms, RetrieveNTPTime);            
                break;
            }
            case NSAPI_STATUS_DISCONNECTED:
            {
                printf("Socket disconnected from network!\r\n");
                g_STDIOMutex.unlock();

                // TBD, Nuertey Odzeyem; does the embedded application's
                // requirements necessarily warrant me performing such an action?
                // It is 'embedded' after all and I may just want to 'run forever'.
                g_pMasterEventQueue->break_dispatch();
                break;
            }
            case NSAPI_STATUS_CONNECTING:
            {
                printf("Connecting to network!\r\n");
                g_STDIOMutex.unlock();
                break;
            }
            default:
            {
                printf("Not supported\r\n");
                g_STDIOMutex.unlock();
                break;
            }
        }
    }

    // To prevent order of initialization defects.
    bool InitializeGlobalResources()
    {
        // Defensive programming; start from a clean slate.
        g_pMessage.reset();

        randLIB_seed_random();

        g_pNetworkInterface = NetworkInterface::get_default_instance();

        if (!g_pNetworkInterface)
        {
            g_STDIOMutex.lock();
            printf("FATAL! No network interface found.\n");
            g_STDIOMutex.unlock();
            return false;
        }

        // Asynchronously monitor for Network Status events.
        //g_EthernetInterface.attach(&NetworkStatusCallback);
        g_pNetworkInterface->attach(&NetworkStatusCallback);

        g_pNetworkInterface->set_blocking(false);
        [[maybe_unused]] auto asynchronous_connect_return_perhaps_can_be_safely_ignored \
                                               = g_pNetworkInterface->connect();
        
        return true;
    }

    // For symmetry and to encourage correct and explicit cleanups.
    void ReleaseGlobalResources()
    {
        // Bring down the Ethernet interface.
        g_EthernetInterface.disconnect();
    }
    
    void DisplayStatistics()
    {
        // By means of DHCP, extract our IP address and other related 
        // configuration information such as the subnet mask and default gateway.
        std::tie(g_NetworkInterfaceInfo, g_SystemProfile, g_BaseRegisterValues, g_HeapStatistics) = ComposeSystemStatistics();
            
        g_STDIOMutex.lock();
        printf("\r\n%s\r\n", g_NetworkInterfaceInfo.c_str());
        printf("\r\n%s\r\n", g_SystemProfile.c_str());
        printf("\r\n%s\r\n", g_BaseRegisterValues.c_str());
        printf("\r\n%s\r\n", g_HeapStatistics.c_str());
        g_STDIOMutex.unlock();
    }
    
    void RetrieveNTPTime()
    {
        g_STDIOMutex.lock();
        printf("Retrieving NTP time from \"2.pool.ntp.org\" server...\n");
        g_STDIOMutex.unlock();
        
        g_NTPClient.SynchronizeRTCTimestamp();
        
        // Just to allow things to stabilize:
        ThisThread::sleep_for(20ms);
        
        DisplayStatistics();
                
        DHT11SensorAcquisition();
    }
} // namespace


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

//void DisplayLCDCapabilities()
//{
//    g_LCD16x2.create(0, downArrow);
//    g_LCD16x2.create(1, upArrow);
//    g_LCD16x2.create(2, rightArrow);
//    g_LCD16x2.create(3, leftArrow);
//
//    g_LCD16x2.cls();
//    g_LCD16x2.locate(0, 0);
//    g_LCD16x2.printf("NUCLEO-F767ZI\n");
//    g_LCD16x2.character(0, 1, 0);
//    g_LCD16x2.character(3, 1, 1);
//    g_LCD16x2.character(5, 1, 2);
//    g_LCD16x2.character(7, 1, 3);
//
//    //ThisThread::sleep_for(2000ms);
//    //g_LCD16x2.cls();
//    g_LCD16x2.locate(0, 1);
//    g_LCD16x2.printf("NUERTEY ODZEYEM\n");
//
//    //ThisThread::sleep_for(2000ms);
//    //g_LCD16x2.display(DISPLAY_OFF);
//    //ThisThread::sleep_for(2000ms);
//    //g_LCD16x2.display(DISPLAY_ON);
//    //ThisThread::sleep_for(2000ms);
//    //g_LCD16x2.display(CURSOR_ON);
//    //ThisThread::sleep_for(2000ms);
//    //g_LCD16x2.display(BLINK_ON);
//    //ThisThread::sleep_for(2000ms);
//    //g_LCD16x2.display(BLINK_OFF);
//    //ThisThread::sleep_for(2000ms);
//    //g_LCD16x2.display(CURSOR_OFF);
//    //
//    //for (uint8_t nuert = 0; nuert < 1; nuert++)
//    //{
//    //    for (uint8_t pos = 0; pos < 13; pos++)
//    //    {
//    //        // scroll one position to left
//    //        g_LCD16x2.display(SCROLL_LEFT);
//    //        // step time
//    //        ThisThread::sleep_for(500ms);
//    //    }
//    //
//    //    // scroll 29 positions (string length + display length) to the right
//    //    // to move it offscreen right
//    //    for (uint8_t pos = 0; pos < 29; pos++)
//    //    {
//    //        // scroll one position to right
//    //        g_LCD16x2.display(SCROLL_RIGHT);
//    //        // step time
//    //        ThisThread::sleep_for(500ms);
//    //    }
//    //
//    //    // scroll 16 positions (display length + string length) to the left
//    //    // to move it back to center
//    //    for (uint8_t pos = 0; pos < 16; pos++)
//    //    {
//    //        // scroll one position to left
//    //        g_LCD16x2.display(SCROLL_LEFT);
//    //        // step time
//    //        ThisThread::sleep_for(500ms);
//    //    }
//    //
//    //    ThisThread::sleep_for(1000ms);
//    //}
//}

bool InitializeSocket(const std::string & server, const uint16_t & port)
{
    bool result = false;
    
    std::string                m_MQTTBrokerDomainName{server};    // Domain name will always exist.
    std::optional<std::string> m_MQTTBrokerAddress{std::nullopt}; // However IP Address might not always exist...
    uint16_t                   m_MQTTBrokerPort{port};
    
    // The socket has to be opened and connected in order for the client
    // to be able to interact with the broker.
    nsapi_error_t rc = Utility::m_TheSocket.open(Utility::g_pNetworkInterface);
    if (rc != NSAPI_ERROR_OK)
    {
        printf("Error! TCPSocket.open() returned: \
            [%d] -> %s\r\n", rc, ToString(rc).c_str());

        // Abandon attempting to connect to the socket.                
        return result;
    }
    
    // Set timeout on blocking socket operations.
    //
    // Initially all sockets have unbounded timeouts. NSAPI_ERROR_WOULD_BLOCK
    // is returned if a blocking operation takes longer than the specified timeout.
    Utility::m_TheSocket.set_blocking(true);
    Utility::m_TheSocket.set_timeout(BLOCKING_SOCKET_TIMEOUT_MILLISECONDS);
    
    auto ipAddress = Utility::ResolveAddressIfDomainName(m_MQTTBrokerDomainName
                                                         , Utility::g_pNetworkInterface
                                                         , &Utility::m_TheSocketAddress);
    
    if (ipAddress)
    {
        std::swap(m_MQTTBrokerAddress, ipAddress);
    }
    else
    {
        printf("Error! Utility::ResolveAddressIfDomainName() failed.\r\n");

        // Abandon attempting to connect to the socket.                
        return result; 
    }
    
    Utility::m_TheSocketAddress.set_port(m_MQTTBrokerPort);
    
    printf("Connecting to \"%s\" as resolved to: \"%s:%d\" ...\n",
        m_MQTTBrokerDomainName.c_str(), 
        m_MQTTBrokerAddress.value().c_str(), 
        m_MQTTBrokerPort);
        
    // The new MbedOS-MQTT API expects to receive a pointer to a configured and 
    // connected socket. This socket will be used for further communication.
    rc = Utility::m_TheSocket.connect(Utility::m_TheSocketAddress);

    if (rc != NSAPI_ERROR_OK)
    {
        printf("Error! TCPSocket.connect() to Broker returned:\
            [%d] -> %s\n", rc, ToString(rc).c_str());
            
        // Abandon attempting to connect to the socket.               
        return result;
    }
    else
    {   
        printf("Success! Connected to Socket at \"%s\" as resolved to: \"%s:%d\"\n", 
            m_MQTTBrokerDomainName.c_str(), m_MQTTBrokerAddress.value().c_str(), m_MQTTBrokerPort);
            
        result = true;
    }   
    
    return result;
}

void DHT11SensorAcquisition()
{
    printf("Running DHT11SensorAcquisition() ... \r\n");
    
    if (InitializeSocket(NUERTEY_MQTT_BROKER_ADDRESS, NUERTEY_MQTT_BROKER_PORT))
    {
        static bool initial_run = true;

        NuerteyMQTTClient g_TheMQTTClient(NUERTEY_MQTT_BROKER_ADDRESS,
                                          NUERTEY_MQTT_BROKER_PORT);

        printf("After NuerteyMQTTClient g_TheMQTTClient ... \r\n");                              
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
                        
                        break;
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
    
    printf("Exiting DHT11SensorAcquisition() ... \r\n");
}



/***********************************************************************
* @file
*
* Encapsulation of an Embedded MQTT Client Library targeted for 
* environments such as ARM Mbed, Arduino and FreeRTOS. MQTT is a 
* machine-to-machine (M2M) "Internet of Things" connectivity protocol.
*       
* @note     The underlying Eclipse Paho MQTTClient has the following
*           characteristics :
* 
*           [1] Avoids dynamic memory allocations and the use of the STL.
* 
*           [2] Dependency on the lowest level MQTTPacket library.
* 
*           [3] Supports Last Will and Testament (LWT) and SSL/TLS.
* 
*           [4] Supports Standard TCP but not WebSockets.
* 
*           [5] Does not support Message Persistence or Automatic Reconnects.
* 
*           [6] Has no support for Offline Buffering or High Availability.
* 
*           Reference : http://www.eclipse.org/paho/clients/c/embedded/
* 
* @warning  [1] Note that the underlying Eclipse Paho MQTT client API is  
*           blocking and non-threaded. This implies that the API blocks 
*           on all method calls, until they are complete. Hence only one 
*           MQTT request can be in process at any given time. 
* 
*           [2] Ensure to not transmit more subscription requests than 
*           the 'Maximum Number of Packets that can be Inflight' parameter
*           below.
* 
*           [3] Ensure that all published payloads do not approach or exceed
*           the maximum packet size configuration parameter specified below.
*           Furthermore, do not forget that an extra generated MQTT header 
*           will be appended onto the formed packet.  
* 
*           [4] Ensure that MQTT messages' lifelines last until yield() 
*           occurs for actual message transmission so as not to segfault.
* 
*  Created: October 25, 2018
*   Author: Nuertey Odzeyem        
************************************************************************/
#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "NetworkInterface.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"
//#include "nanopb/source/protobuf/pb_common.h"
//#include "nanopb/source/protobuf/pb_encode.h"
//#include "nanopb/source/protobuf/pb_decode.h"

#define MQTTCLIENT_QOS1 1

class NuerteyMQTTClient 
{
public:
    static const std::string DEFAULT_MQTT_BROKER_ADDRESS;    // I am targeting "broker.hivemq.com" for testing 
    static const std::string DEFAULT_MQTT_CLIENT_IDENTIFIER; // For now I will use "nuertey-nucleo_f767zi" but ideally should be generated from UUID if multiple devices would be deployed. 
    static const std::string DEFAULT_MQTT_USERNAME;          // Let's not forget authentication as security is important. 
    static const std::string DEFAULT_MQTT_PASSWORD;          // Let's not forget authentication as security is important.
    static const uint16_t    DEFAULT_MQTT_BROKER_PORT = 1883;
    static const uint32_t    DEFAULT_TIME_TO_WAIT_FOR_RECEIVED_MESSAGE_MSECS = 500;
    static const char * NUERTEY_ADDRESS_BOOK_MQTT_TOPIC;     // The Paho MQTT embedded client does not seem to like ...
    static const char * NUCLEO_F767ZI_IOT_MQTT_TOPIC1;       // playing nice with the null appended to ...
    static const char * NUCLEO_F767ZI_IOT_MQTT_TOPIC2;       // std::string::c_str() so rather use char *.
    static const char * NUCLEO_F767ZI_IOT_MQTT_TOPIC3;
    static const char * NUCLEO_F767ZI_IOT_MQTT_TOPIC4;
    static const char * RELATIVE_TIME_MQTT_TOPIC;
    static const char * ABSOLUTE_TIME_MQTT_TOPIC;
    static const char * NUCLEO_F767ZI_CONVERSATION_MQTT_TOPIC;
    
    NuerteyMQTTClient(NetworkInterface * pNetworkInterface, 
                      const std::string & server = DEFAULT_MQTT_BROKER_ADDRESS, 
              const uint16_t & port = DEFAULT_MQTT_BROKER_PORT);
    
    // TBD, Nuertey Odzeyem : Perhaps add code to detect spurious client 
    // disconnects and logic to reconnect if so, and on the fly.
    // Maybe leverage the LWT feature and a special topic and payload. See :
    //
    // https://stackoverflow.com/questions/47115790/paho-mqtt-c-client-fails-to-connect-to-mosquitto
    // https://www.hivemq.com/blog/how-to-get-started-with-mqtt
    
    NuerteyMQTTClient(const NuerteyMQTTClient&) = delete;
    NuerteyMQTTClient& operator=(const NuerteyMQTTClient&) = delete;
    NuerteyMQTTClient(NuerteyMQTTClient&&) = default;
    NuerteyMQTTClient& operator=(NuerteyMQTTClient&&) = default;
    
    virtual ~NuerteyMQTTClient();

    bool Connect();
    void Disconnect();

    void Subscribe(const char * topic);
    void UnSubscribe(const char * topic);
    
    void Publish(const char * topic);
    void Publish(const char * topic, MQTT::Message & data);
    // TBD, Nuertey Odzeyem : Maybe consider replacing pointer and size with mbed::Span. 
    void Publish(const char * topic, const void * data, const size_t & size); 

    std::optional<std::string>    GetHostDomainName() const {return m_MQTTBrokerDomainName;}
    std::string                   GetHostIPAddress() const {return m_MQTTBrokerAddress;}
    uint16_t                      GetPortNumber() const {return m_MQTTBrokerPort;}     
    bool                          IsConnected() const {return m_IsMQTTSessionEstablished;} 

    // The intent of ::Yield() is to hand-over our execution context to 
    // the Paho MQTT Client library. While yield is executed that library 
    // processes incoming messages and sends out MQTT keepalives when required.
    // Note that if there are no messages waiting for the client, then 
    // the underlying yield() will return -1 even if the connection is still valid.
    //
    // Reference : https://github.com/monstrenyatko/ArduinoMqtt/issues/12
    int Yield(const uint32_t & timeInterval = DEFAULT_TIME_TO_WAIT_FOR_RECEIVED_MESSAGE_MSECS);
 
    static void MessageArrived(MQTT::MessageData & data);
        
    static uint64_t                               m_ArrivedMessagesCount;
    static uint64_t                               m_OldMessagesCount;
private:
    NetworkInterface *                            m_pNetworkInterface;
    MQTTNetwork                                   m_MQTTNetwork;
    MQTT::Client<MQTTNetwork, Countdown, 1024, 5> m_PahoMQTTclient;       // Increase the maximum packet size to 1Kb.
                                                                          // Increase the maximum number of subscriptions to 5.
    std::optional<std::string>                    m_MQTTBrokerDomainName; // Domain name would not always necessarily exist...
    std::string                                   m_MQTTBrokerAddress;    // However IP Address always would.
    uint16_t                                      m_MQTTBrokerPort;
    bool                                          m_IsMQTTSessionEstablished;
};

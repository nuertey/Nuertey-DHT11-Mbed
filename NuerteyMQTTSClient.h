/***********************************************************************
* @file
*
* Encapsulation of an Embedded MQTTS Client Library targeted for 
* environments such as ARM Mbed, Arduino and FreeRTOS. MQTTS is a secure
* machine-to-machine (M2M) "Internet of Things" connectivity protocol.
* 
* It is intended that this encapsulation will be used to connect to 
* Cloud IOT Services such as Amazon AWS IOT, Google Cloud IOT, IBM Watson
* IOT, or some such service.
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
*           [4] Supports Standard TLS but not WebSockets.
* 
*           [5] Does not support Message Persistence or Automatic Reconnects.
* 
*           [6] Has no support for Offline Buffering or High Availability.
* 
*           Reference : http://www.eclipse.org/paho/clients/c/embedded/
* 
* @warning  [0] To err on the side of caution and security, it is expected 
*           that the user will assume the responsibility of generating
*           and providing their own relevant certificates, registry and 
*           device IDs, indeed all the other errata pertaining to their 
*           particular service. These should then be supplied during 
*           construction of this class abstraction/encapsulation,
*           alongst with the service type, all specified as well-typed
*           tuples to be outlined below.    
* 
*           [1] Note that the underlying Eclipse Paho MQTT client API is  
*           blocking and non-threaded. This implies that the API blocks 
*           on all method calls, until they are complete. Hence only one 
*           MQTTS request can be in process at any given time. 
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
*  Created: January 13, 2019
*   Author: Nuertey Odzeyem        
************************************************************************/
#pragma once

#include "NetworkInterface.h"
#include "MQTTmbed.h"
#include "MQTTNetwork.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed_events.h"
#include "mbedtls/error.h"
#include "jwt-mbed.h"
#include "Utilities.h"
#include "lwip/arch.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "NTPClient.h"

#define MQTTCLIENT_QOS1 1

using SSLHost_t = const char *;
using SSLPort_t = uint16_t;
using SSLURL_t = std::pair<SSLHost_t, SSLPort_t>;

using SSLRootCertificate_t = const char *;
using SSLClientCertificate_t = const char *;
using SSLClientPrivateKey_t = const char *;
using SSLPublicKeyInfrastructure_t = std::tuple<SSLRootCertificate_t, 
                                                SSLClientCertificate_t, 
                                                SSLClientPrivateKey_t>;

struct AmazonCloudCredentials_t
{
    const char *    m_pPolicy; 
    const char *    m_pPolicyARN; 
    const char *    m_pDeviceGatewayEndpoint;
};

struct GoogleCloudCredentials_t
{
    const char *    m_pProjectID; 
    const char *    m_pRegion; 
    const char *    m_pRegistryID;
    const char *    m_pDeviceID;
};

struct IBMCloudCredentials_t
{
    const char *    m_pOrganizationID; 
    const char *    m_pDeviceType; 
    const char *    m_pDeviceID;
    const char *    m_pAuthenticationMethod;
    const char *    m_pAuthenticationToken;
};

template <typename CloudCredentials_t>
class NuerteyMQTTSClient 
{
    static_assert(Utility::TrueTypesEquivalent<CloudCredentials_t, AmazonCloudCredentials_t>::value
               || Utility::TrueTypesEquivalent<CloudCredentials_t, GoogleCloudCredentials_t>::value
               || Utility::TrueTypesEquivalent<CloudCredentials_t, IBMCloudCredentials_t>::value,
    "Hey! NuerteyMQTTSClient in its current form is only designed with Amazon, Google or IBM Cloud in mind!!");
    
public:
    static constexpr int       JSON_WEB_TOKENS_DURATION_SECONDS =  3600;
    static constexpr int       JSON_WEB_TOKENS_BUFFER_SIZE      =  2048;
    static const unsigned long DELAY_FOR_RECEIVED_MESSAGE_MSECS =   100;
    static constexpr int       MAXIMUM_MQTTS_CONNECTIONS        =     5;// concurrent subscriptions.
    static constexpr int       MAXIMUM_MQTTS_PACKET_SIZE        =  1024;// 1Kb.
    static constexpr int       MAXIMUM_TLS_ERROR_CODE           = -4096;
    static constexpr int       TLS_ERROR_STRING_BUFFER_SIZE     =   256;

    NuerteyMQTTSClient(NetworkInterface * pNetworkInterface,
                       const SSLURL_t & url, 
                       const SSLPublicKeyInfrastructure_t & pki, 
                       const CloudCredentials_t & credentials);
    
    NuerteyMQTTSClient(const NuerteyMQTTSClient&) = delete;
    NuerteyMQTTSClient& operator=(const NuerteyMQTTSClient&) = delete;
    NuerteyMQTTSClient(NuerteyMQTTSClient&&) = default;
    NuerteyMQTTSClient& operator=(NuerteyMQTTSClient&&) = default;
    
    virtual ~NuerteyMQTTSClient();

    bool Connect();
    void Disconnect();
    void NetworkDisconnect();

    void Subscribe(const char * topic);
    void UnSubscribe(const char * topic);
    
    void Publish(const char * topic, bool shouldYield = true);
    void Publish(const char * topic, MQTT::Message & data, bool shouldYield = true); 
    void Publish(const char * topic, const void * data, const size_t & size, bool shouldYield = true); 
   
    bool IsNetworkConnected() const {return m_IsNetworkConnected;}
    bool IsConnected() const {return m_IsMQTTSSessionEstablished;} 

    int Yield(const unsigned long & timeInterval = DELAY_FOR_RECEIVED_MESSAGE_MSECS);
 
    static void MessageArrived(MQTT::MessageData & data);
        
    static uint64_t                             m_ArrivedMessagesCount;
    static uint64_t                             m_OldMessagesCount;
    
protected:
    // Completely efface this method at compile-time if GoogleCloud is not the cloud service.
    template <typename T = CloudCredentials_t>
    std::enable_if_t<std::is_same_v<T, GoogleCloudCredentials_t>, bool> ComposeGooglePassword()
    {   
        bool result = false; 
        std::error_code expect;
        std::error_code actual;
        
        NTPClient googleNTPClient(&Utility::g_EthernetInterface);
        time_t synchronizedTime = googleNTPClient.get_timestamp();
        set_time(synchronizedTime);

        std::string audience(m_theCredentials.m_pProjectID);

        // std::chrono::system_clock::now() does not seem to work with Google Cloud IOT.
        jwt::date now = std::chrono::system_clock::from_time_t(time(NULL)); 
        jwt::date expiry = now + std::chrono::seconds(JSON_WEB_TOKENS_DURATION_SECONDS);

        auto token = jwt::create()
            .set_algorithm("RS256")
            .set_type("JWT")
            .set_audience(audience)
            .set_issued_at(now)
            .set_expires_at(expiry)
            .sign(jwt::algorithm::rs256(std::string(std::get<1>(m_thePKI)),
                                        std::string(std::get<2>(m_thePKI)),
                                        "", ""), actual); 

        if (expect.message() == actual.message())
        {
            // No errors whilst signing. Go ahead and use token
            tr_debug("Generated JWT token (length %d) := \n\t%s\n", token.size(), token.c_str());
            
            m_GoogleMQTTSPassword = new char[token.size() + 1];
            strncpy(m_GoogleMQTTSPassword, token.c_str(), token.size() + 1);
            m_GoogleMQTTSPassword[token.size()] = '\0';
            result = true;
        }
        else
        {
            tr_error("ERROR: Failed to generate JSON_WEB_TOKENS password.\r\n");
        }
        return result;
    }
    
    template <typename T = CloudCredentials_t>
    std::enable_if_t<std::is_same_v<T, AmazonCloudCredentials_t>, void> ComposeAmazonMQTTConnectData()
    { 
        m_MQTTConnectData.MQTTVersion = 3; // 3 = 3.1 4 = 3.1.1
        m_MQTTConnectData.clientID.cstring = (char *)m_theCredentials.m_pPolicy;
        m_MQTTConnectData.username.cstring = (char *)"testuser";
        m_MQTTConnectData.password.cstring = (char *)"testpassword";
    }

    template <typename T = CloudCredentials_t>
    std::enable_if_t<std::is_same_v<T, GoogleCloudCredentials_t>, void> ComposeGoogleMQTTConnectData()
    { 
        std::string mqttClientID = std::string("projects/") 
                                  + m_theCredentials.m_pProjectID
                                  + "/locations/" 
                                  + m_theCredentials.m_pRegion 
                                  + "/registries/" 
                                  + m_theCredentials.m_pRegistryID 
                                  + "/devices/" 
                                  + m_theCredentials.m_pDeviceID;
        m_MQTTConnectData.MQTTVersion = 4;
        m_MQTTConnectData.clientID.cstring = (char *)mqttClientID.c_str();
        m_MQTTConnectData.username.cstring = (char *)"ignored";
        m_MQTTConnectData.password.cstring = (char *)m_GoogleMQTTSPassword;
        //tr_debug("Nuertey [ComposeGoogleMQTTConnectData] here...\r\n");
    }

    template <typename T = CloudCredentials_t>
    std::enable_if_t<std::is_same_v<T, IBMCloudCredentials_t>, void> ComposeIBMMQTTConnectData()
    { 
        std::string mqttClientID = std::string("d:") 
                                  + m_theCredentials.m_pOrganizationID
                                  + ":" 
                                  + m_theCredentials.m_pDeviceType 
                                  + ":" 
                                  + m_theCredentials.m_pDeviceID;
        m_MQTTConnectData.MQTTVersion = 4;
        m_MQTTConnectData.clientID.cstring = (char *)mqttClientID.c_str();
        m_MQTTConnectData.username.cstring = (char *)m_theCredentials.m_pAuthenticationMethod;
        m_MQTTConnectData.password.cstring = (char *)m_theCredentials.m_pAuthenticationToken;
    }
    
private:
    NetworkInterface *                          m_pNetworkInterface;
    SSLURL_t                                    m_theURL;
    SSLPublicKeyInfrastructure_t                m_thePKI;
    CloudCredentials_t                          m_theCredentials;
    MQTTNetwork *                               m_pMQTTNetwork;
    MQTT::Client<MQTTNetwork, Countdown, 
                 MAXIMUM_MQTTS_PACKET_SIZE, 
                 MAXIMUM_MQTTS_CONNECTIONS> *   m_pPahoMQTTclient; 
    bool                                        m_IsMQTTSSessionEstablished;
    bool                                        m_IsNetworkConnected;
    char *                                      m_GoogleMQTTSPassword;
    MQTTPacket_connectData                      m_MQTTConnectData;
};

template <typename CloudCredentials_t>
uint64_t NuerteyMQTTSClient<CloudCredentials_t>::m_ArrivedMessagesCount = 0;

template <typename CloudCredentials_t>
uint64_t NuerteyMQTTSClient<CloudCredentials_t>::m_OldMessagesCount = 0;

template <typename CloudCredentials_t>
NuerteyMQTTSClient<CloudCredentials_t>::NuerteyMQTTSClient(
                       NetworkInterface * pNetworkInterface,
                       const SSLURL_t & url, 
                       const SSLPublicKeyInfrastructure_t & pki, 
                       const CloudCredentials_t & credentials
               )
    : m_pNetworkInterface(pNetworkInterface)
    , m_theURL(url)
    , m_thePKI(pki)
    , m_theCredentials(credentials)
    , m_pMQTTNetwork(new MQTTNetwork(pNetworkInterface))
    , m_pPahoMQTTclient(new MQTT::Client<MQTTNetwork, 
                                         Countdown, 
                                         MAXIMUM_MQTTS_PACKET_SIZE, 
                                         MAXIMUM_MQTTS_CONNECTIONS>
                                         (*m_pMQTTNetwork)) 
    , m_IsMQTTSSessionEstablished(false)
    , m_IsNetworkConnected(false)
    , m_GoogleMQTTSPassword(nullptr)
{
    mbed_trace_init();
    m_MQTTConnectData = MQTTPacket_connectData_initializer;
}

template <typename CloudCredentials_t>
NuerteyMQTTSClient<CloudCredentials_t>::~NuerteyMQTTSClient()
{
    if (IsConnected())
    {
        Disconnect();
    }
    else if (IsNetworkConnected())
    {
        NetworkDisconnect();
    }
    
    delete m_pPahoMQTTclient;
    delete m_pMQTTNetwork;
    if (nullptr != m_GoogleMQTTSPassword)
    {
        delete[] m_GoogleMQTTSPassword;
    }
}

template <typename CloudCredentials_t>
bool NuerteyMQTTSClient<CloudCredentials_t>::Connect()
{
    tr_debug("Connecting to : \"%s:%d\" ...\r\n", 
           m_theURL.first, m_theURL.second);
    bool result = false;
    
    // No point proceeding if we aim to communicate with Google IOT Cloud 
    // but are unable to generate the Json Web Tokens password.
    if constexpr (std::is_same<CloudCredentials_t, GoogleCloudCredentials_t>::value)
    {
        if (!ComposeGooglePassword<GoogleCloudCredentials_t>())
        {
            return result;
        }
    }
      
    nsapi_error_t rc = m_pMQTTNetwork->connect(m_theURL.first, 
                                             (int)m_theURL.second,
                                             std::get<0>(m_thePKI),
                                             std::get<1>(m_thePKI),
                                             std::get<2>(m_thePKI)
                                           );
    if (rc != NSAPI_ERROR_OK)
    {   
        // Network error
        if ((MAXIMUM_TLS_ERROR_CODE < rc) && (rc < NSAPI_ERROR_OK)) 
        {
            tr_error("Error! TCP.connect() returned: [%d] -> %s\n", 
                  rc, ToString(rc).c_str());
        }
        
        // TLS error - mbedTLS error codes starts from -0x1000 to -0x8000.
        if (rc <= MAXIMUM_TLS_ERROR_CODE) 
        {
            char buffer[TLS_ERROR_STRING_BUFFER_SIZE] = {};
            mbedtls_strerror(rc, buffer, TLS_ERROR_STRING_BUFFER_SIZE);
            tr_error("TLS ERROR [%d] -> %s\r\n", rc, buffer);
        }
    }
    else
    {
        m_IsNetworkConnected = true;
        tr_debug("TLS Network Connection established.\r\n");

        if constexpr (std::is_same<CloudCredentials_t, AmazonCloudCredentials_t>::value)
        {
            ComposeAmazonMQTTConnectData<AmazonCloudCredentials_t>();
        }
        else if constexpr (std::is_same<CloudCredentials_t, GoogleCloudCredentials_t>::value)
        {
            ComposeGoogleMQTTConnectData<GoogleCloudCredentials_t>();
        }
        else if constexpr (std::is_same<CloudCredentials_t, IBMCloudCredentials_t>::value)
        {
            ComposeIBMMQTTConnectData<IBMCloudCredentials_t>();
        }   
        // Broker should wipe the session each time we disconnect.
        // Otherwise our subscriptions are retained and messages 
        // sent with Quality of Service 1 and 2 are buffered by the broker.
        m_MQTTConnectData.cleansession = 1; 
        int ret = m_pPahoMQTTclient->connect(m_MQTTConnectData);
        if (ret != MQTT::SUCCESS)
        {
            tr_error("Error! MQTTS.connect() returned: [%d].\n", ret);
            MQTTConnectionError_t retVal = static_cast<MQTTConnectionError_t>(ret);
            tr_error("Error! MQTTS.connect() returned: [%d] -> %s\n", 
                   ret, ToString(retVal).c_str());
        }
        else
        {
            result = true;
            m_IsMQTTSSessionEstablished = true;
            m_ArrivedMessagesCount = 0;
            tr_debug("MQTTS session established with broker at [%s:%d]\r\n", 
                   m_theURL.first, m_theURL.second);
        }
    }
    
    return result;
}

template <typename CloudCredentials_t>    
void NuerteyMQTTSClient<CloudCredentials_t>::Disconnect()
{
    if (IsConnected())
    {
        tr_debug("Disconnecting from : \"%s:%d\" ...\r\n", 
              m_theURL.first, m_theURL.second);  
        int retVal = m_pPahoMQTTclient->disconnect();
        if (retVal != MQTT::SUCCESS)
        {
            tr_error("Error! MQTTS.disconnect() returned: [%d].\n", retVal);
        }
         
        nsapi_error_t rc = m_pMQTTNetwork->disconnect();
        if (rc != NSAPI_ERROR_OK)
        {
            tr_error("Error! TCP.disconnect() returned: [%d] -> %s\n", 
                  rc, ToString(rc).c_str());
        }
        m_ArrivedMessagesCount = 0;
        m_IsMQTTSSessionEstablished = false;
        m_IsNetworkConnected = false;
    }
}

template <typename CloudCredentials_t>    
void NuerteyMQTTSClient<CloudCredentials_t>::NetworkDisconnect()
{
    if (IsNetworkConnected())
    {
        nsapi_error_t rc = m_pMQTTNetwork->disconnect();
        if (rc != NSAPI_ERROR_OK)
        {
            tr_error("Error! TCP.disconnect() returned: [%d] -> %s\n", 
                  rc, ToString(rc).c_str());
        }
        m_IsNetworkConnected = false;
    }
}

template <typename CloudCredentials_t>
void NuerteyMQTTSClient<CloudCredentials_t>::Subscribe(const char * topic)
{
    if (strcmp(topic, "") != 0)
    {
        int rc = MQTT::FAILURE;
        
        // "Exactly Once". Such messages are guaranteed to be delivered 
        // exactly once. Hence no duplicates or lost messages.
        if ((rc = m_pPahoMQTTclient->subscribe(topic, MQTT::QOS1, 
                    NuerteyMQTTSClient::MessageArrived)) != MQTT::SUCCESS)
        {
            tr_error("Error! MQTT.subscribe() returned: [%d].\n", rc);
        }
    }
}

template <typename CloudCredentials_t>
void NuerteyMQTTSClient<CloudCredentials_t>::UnSubscribe(const char * topic)
{
    if (strcmp(topic, "") != 0)
    {
        int rc = MQTT::FAILURE;
        
        // "Exactly Once". Such messages are guaranteed to be delivered 
        // exactly once. Hence no duplicates or lost messages.
        if ((rc = m_pPahoMQTTclient->unsubscribe(topic)) != MQTT::SUCCESS)
        {
            tr_error("Error! MQTT.unsubscribe() returned: [%d].\n", rc);
        }
    }
}

template <typename CloudCredentials_t>
void NuerteyMQTTSClient<CloudCredentials_t>::Publish(const char * topic, 
                    bool shouldYield)
{
    if (strcmp(topic, "") != 0)
    {
        m_OldMessagesCount = m_ArrivedMessagesCount;
        Utility::g_pMessage.reset(new MQTT::Message());
        
        Utility::g_pMessage->qos = MQTT::QOS1;
        Utility::g_pMessage->retained = false;
        Utility::g_pMessage->dup = false;
        Utility::g_pMessage->payload = nullptr;
        Utility::g_pMessage->payloadlen = 0;
        int rc = m_pPahoMQTTclient->publish(topic, *Utility::g_pMessage.get());
        if (rc != MQTT::SUCCESS)
        {
            tr_error("Error! MQTT.publish() returned: [%d].\n", rc);
        }
        else
        {
            if (shouldYield)
            {
                while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
                {
                    tr_debug("Yielding... ");
                    int retVal = Yield();
                    if (MQTT::FAILURE == retVal)
                    {
                        tr_error("Warning! MQTT.yield() indicates that \
                        for whatever reason, we have been disconnected from the broker.\n");
                        break;
                    }
                }
            }
        }
    }
}

template <typename CloudCredentials_t>
void NuerteyMQTTSClient<CloudCredentials_t>::Publish(const char * topic, 
            MQTT::Message & data, bool shouldYield)
{
    m_OldMessagesCount = m_ArrivedMessagesCount;
    int rc = m_pPahoMQTTclient->publish(topic, data);
    if (rc != MQTT::SUCCESS)
    {
        tr_error("Error! MQTT.publish() returned: [%d].\n", rc);
    }
    else
    {
        if (shouldYield)
        {
            while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
            {
                tr_debug("Yielding... ");
                int retVal = Yield();
                if (MQTT::FAILURE == retVal)
                {
                    tr_error("Warning! MQTT.yield() indicates that \
                    for whatever reason, we have been disconnected from the broker.\n");
                    break;
                }
            }
        }
    }
}

template <typename CloudCredentials_t>
void NuerteyMQTTSClient<CloudCredentials_t>::Publish(const char * topic, 
                const void * data, const size_t & size, bool shouldYield)
{
    m_OldMessagesCount = m_ArrivedMessagesCount;
    Utility::g_pMessage.reset(new MQTT::Message());
    
    Utility::g_pMessage->qos = MQTT::QOS1;
    Utility::g_pMessage->retained = false;
    Utility::g_pMessage->dup = false;
    Utility::g_pMessage->payload = (void*)data;
    
    // Cautionary note here: if data was composed using snprintf and their
    // ilk, ensure to add 1 to this size so as to include the null terminator.
    // Possibility exists that there are servers out there not robust 
    // enough to otherwise handle reading such data. Safety and security always!
    Utility::g_pMessage->payloadlen = size;
    int rc = m_pPahoMQTTclient->publish(topic, *Utility::g_pMessage.get());
    if (rc != MQTT::SUCCESS)
    {
        tr_error("Error! MQTT.publish() returned: [%d].\n", rc);
    }
    else
    {
        if (shouldYield)
        {
            while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
            {
                tr_debug("Yielding... ");
                int retVal = Yield();
                if (MQTT::FAILURE == retVal)
                {
                    tr_error("Warning! MQTT.yield() indicates that \
                    for whatever reason, we have been disconnected from the broker.\n");
                    break;
                }
            }
        }
    }
}

template <typename CloudCredentials_t>
int NuerteyMQTTSClient<CloudCredentials_t>::Yield(const unsigned long & timeInterval)
{
    // The intent of ::Yield() is to hand-over our execution context to 
    // the Paho MQTT Client library. While yield is executed that library 
    // processes incoming messages and sends out MQTT keepalives when required.
    return (m_pPahoMQTTclient->yield(timeInterval));
}

template <typename CloudCredentials_t>
void NuerteyMQTTSClient<CloudCredentials_t>::MessageArrived(MQTT::MessageData & data)
{
    MQTT::Message &message = data.message;
    tr_debug("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", 
          message.qos, message.retained, message.dup, message.id);
    tr_debug("data.topicName.lenstring.data :-> %.*s\r\n", 
         data.topicName.lenstring.len, data.topicName.lenstring.data);
    tr_debug("message.payloadlen :-> %d\r\n", message.payloadlen);
    
    if (message.qos == MQTT::QOS0)
    {
        tr_debug("MQTT::QOS0\r\n");
    }
    else if (message.qos == MQTT::QOS1)
    {
        tr_debug("MQTT::QOS1\r\n");
    }
    else if (message.qos == MQTT::QOS2)
    {
        tr_debug("MQTT::QOS2\r\n");
    }
    else
    {
        tr_warn("MQTT::QOS??? :-> %d\r\n", message.qos);
    }
        
    if (message.payloadlen > 0)
    {
        tr_debug("Binary Payload : \r\n\r\n%.*s\r\n", message.payloadlen, 
              (char*)message.payload);
    }
    
    // Allow Yield to break out... implies some tricky context switching
    // here. Asynchronous callback from the underlying client versus our
    // synchronuous context of the main thread.
    ++m_ArrivedMessagesCount;
}

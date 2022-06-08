#include "NuerteyMQTTClient.h"
#include "Utilities.h"
#include "mbed_trace.h"

#define TRACE_GROUP  "NuerteyMQTTClient"

const std::string NuerteyMQTTClient::DEFAULT_MQTT_CLIENT_IDENTIFIER("nuertey-nucleo_f767zi");
const std::string NuerteyMQTTClient::DEFAULT_MQTT_USERNAME("testuser");
const std::string NuerteyMQTTClient::DEFAULT_MQTT_PASSWORD("testpassword");

const uint32_t    NuerteyMQTTClient::DEFAULT_TIME_TO_WAIT_FOR_RECEIVED_MESSAGE_MSECS;

uint64_t    NuerteyMQTTClient::m_ArrivedMessagesCount(0);
uint64_t    NuerteyMQTTClient::m_OldMessagesCount(0);

NuerteyMQTTClient::NuerteyMQTTClient(NetworkInterface * pNetworkInterface, 
                                     const std::string & server, const uint16_t & port)
    : m_pNetworkInterface(pNetworkInterface)
    , m_PahoMQTTclient(&m_TheSocket)
    , m_MQTTBrokerDomainName(server)
    , m_MQTTBrokerAddress(std::nullopt)
    , m_MQTTBrokerPort(port)
    , m_IsMQTTSessionEstablished(false)
{
    mbed_trace_init();
}

NuerteyMQTTClient::~NuerteyMQTTClient()
{
}

bool NuerteyMQTTClient::Connect()
{
    bool result = false;
    
    // The socket has to be opened and connected in order for the client
    // to be able to interact with the broker.
    nsapi_error_t rc = m_TheSocket.open(m_pNetworkInterface);
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
    m_TheSocket.set_blocking(true);
    //m_TheSocket.set_timeout(BLOCKING_SOCKET_TIMEOUT_MILLISECONDS);
    
    auto ipAddress = Utility::ResolveAddressIfDomainName(m_MQTTBrokerDomainName
                                                         , m_pNetworkInterface
                                                         , &m_TheSocketAddress);
    
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
    
    m_TheSocketAddress.set_port(m_MQTTBrokerPort);
    
    printf("Connecting to \"%s\" as resolved to: \"%s:%d\" ...\n",
        m_MQTTBrokerDomainName.c_str(), 
        m_MQTTBrokerAddress.value().c_str(), 
        m_MQTTBrokerPort);
        
    // The new MbedOS-MQTT API expects to receive a pointer to a configured and 
    // connected socket. This socket will be used for further communication.
    rc = m_TheSocket.connect(m_TheSocketAddress);

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
    }

    // Note: Default values are not defined for members of MQTTClient_connectOptions
    // so it is good practice to specify all settings. If the MQTTClient_connectOptions
    // structure is defined as an automatic variable, all members are set to random 
    // values and thus must be set by the client application. If the MQTTClient_connectOptions
    // structure is defined as a static variable, initialization (in compliant compilers) 
    // sets all values to 0 (NULL for pointers). A keepAliveInterval setting of 0 prevents
    // correct operation of the client and so you must at least set a value for keepAliveInterval.     
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)DEFAULT_MQTT_CLIENT_IDENTIFIER.c_str();
    
    // Ensure to configure the MQTT Broker to "allow_anonymous true" if
    // not setting a username and password:
    //
    // cat /etc/mosquitto/conf.d/default.conf
    // cat /etc/mosquitto/mosquitto.conf
    data.username.cstring = (char *)DEFAULT_MQTT_USERNAME.c_str();
    data.password.cstring = (char *)DEFAULT_MQTT_PASSWORD.c_str();

    // Broker should wipe the session each time we disconnect.
    // Otherwise our subscriptions are retained and messages
    // sent with Quality of Service 1 and 2 are buffered by the broker.
    data.cleansession = 1;
    
    // The "keep alive" interval, measured in seconds, defines the 
    // maximum time that should pass without communication between 
    // the client and the server. The client will ensure that at least
    // one message travels across the network within each keep alive
    // period. In the absence of a data-related message during the time
    // period, the client sends a very small MQTT "ping" message, which
    // the server will acknowledge. The keep alive interval enables the
    // client to detect when the server is no longer available without
    // having to wait for the long TCP/IP timeout. 
    data.keepAliveInterval = 7200;
     
    printf("\r\nm_PahoMQTTclient connecting to MQTT Broker at: \"%s:%d\" ...", 
        m_MQTTBrokerAddress.value().c_str(), m_MQTTBrokerPort);
    
    int retVal = MQTT::FAILURE;
    if ((retVal = m_PahoMQTTclient.connect(data)) != MQTT::SUCCESS)
    {
        tr_error("Error! nm_PahoMQTTclient.connect() returned: [%d] -> %s\n", 
              retVal, ToString(ToEnum<MQTTConnectionError_t, int>(retVal)).c_str());
    }
    else
    {
        m_IsMQTTSessionEstablished = true;
        m_ArrivedMessagesCount = 0;
        printf("\r\n\r\nMQTT session established with broker at [%s:%d]\r\n", 
            m_MQTTBrokerAddress.value().c_str(), m_MQTTBrokerPort);
        result = true;
    }

    return result;
}

void NuerteyMQTTClient::Disconnect()
{
    if (IsConnected())
    {
        printf("\r\nClosing session with broker : \"%s\" ...", m_MQTTBrokerDomainName.c_str());
        int retVal = m_PahoMQTTclient.disconnect();
        if (retVal != MQTT::SUCCESS)
        {
            printf("\r\n\r\nError! MQTT.disconnect() returned: [%d] -> %s\n", 
                retVal, ToString(ToEnum<MQTTConnectionError_t, int>(retVal)).c_str());
        }

        printf("\r\nClosing socket... ");
        nsapi_error_t rc = m_TheSocket.close();
        if (rc != NSAPI_ERROR_OK)
        {
            printf("\r\n\r\nError! TCP.disconnect() returned: [%d] -> %s\n", rc, ToString(rc).c_str());
        }
        m_ArrivedMessagesCount = 0;
        m_IsMQTTSessionEstablished = false;
    }
}

void NuerteyMQTTClient::Subscribe(const char * topic)
{
    if (strcmp(topic, "") != 0)
    {
        int rc = MQTT::FAILURE;

        // "Exactly Once". Such messages are guaranteed to be delivered
        // exactly once. Hence no duplicates or lost messages.
        if ((rc = m_PahoMQTTclient.subscribe(topic, MQTT::QOS1, NuerteyMQTTClient::MessageArrived)) != MQTT::SUCCESS)
        {
            printf("\r\n\r\nError! MQTT.subscribe() returned: [%d].\n", rc);
        }
    }
}

void NuerteyMQTTClient::UnSubscribe(const char * topic)
{
    if (strcmp(topic, "") != 0)
    {
        int rc = MQTT::FAILURE;

        // "Exactly Once". Such messages are guaranteed to be delivered
        // exactly once. Hence no duplicates or lost messages.
        if ((rc = m_PahoMQTTclient.unsubscribe(topic)) != MQTT::SUCCESS)
        {
            printf("\r\n\r\nError! MQTT.unsubscribe() returned: [%d].\n", rc);
        }
    }
}

void NuerteyMQTTClient::Publish(const char * topic)
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
        int rc = m_PahoMQTTclient.publish(topic, *Utility::g_pMessage.get());
        if (rc != MQTT::SUCCESS)
        {
            printf("\r\n\r\nError! MQTT.publish() returned: [%d].\n", rc);
        }
        else
        {
            while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
            {
                //printf("\r\nYielding... ");
                printf(".");
                int retVal = Yield();
                if (MQTT::FAILURE == retVal)
                {
                    printf("\r\n\r\nWarning! MQTT.yield() indicates that for whatever reason, we have been disconnected from the broker.\n");
                    break;
                }
            }
        }
    }
}

void NuerteyMQTTClient::Publish(const char * topic, MQTT::Message & data)
{
    m_OldMessagesCount = m_ArrivedMessagesCount;
    int rc = m_PahoMQTTclient.publish(topic, data);
    if (rc != MQTT::SUCCESS)
    {
        printf("\r\n\r\nError! MQTT.publish() returned: [%d].\n", rc);
    }
    else
    {
        while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
        {
            //printf("\r\nYielding... ");
            printf(".");
            int retVal = Yield();
            if (MQTT::FAILURE == retVal)
            {
                printf("\r\n\r\nWarning! MQTT.yield() indicates that for whatever reason, we have been disconnected from the broker.\n");
                break;
            }
        }
    }
}

void NuerteyMQTTClient::Publish(const char * topic, const void * data, const size_t & size)
{
    m_OldMessagesCount = m_ArrivedMessagesCount;
    Utility::g_pMessage.reset(new MQTT::Message());

    Utility::g_pMessage->qos = MQTT::QOS1;
    Utility::g_pMessage->retained = false;
    Utility::g_pMessage->dup = false;
    Utility::g_pMessage->payload = (void*)data;
    Utility::g_pMessage->payloadlen = size;
    int rc = m_PahoMQTTclient.publish(topic, *Utility::g_pMessage.get());
    if (rc != MQTT::SUCCESS)
    {
        printf("\r\n\r\nError! MQTT.publish() returned: [%d].\n", rc);
    }
    else
    {
        while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
        {
            //printf("\r\nYielding... ");
            printf(".");
            int retVal = Yield();
            if (MQTT::FAILURE == retVal)
            {
                printf("\r\n\r\nWarning! MQTT.yield() indicates that for whatever reason, we have been disconnected from the broker.\n");
                break;
            }
        }
    }
}

int NuerteyMQTTClient::Yield(const uint32_t & timeInterval)
{
    // The intent of ::Yield() is to hand-over our execution context to 
    // the Paho MQTT Client library. While yield is executed that library 
    // processes incoming messages and sends out MQTT keepalives when required.
    return (m_PahoMQTTclient.yield(timeInterval));
}

void NuerteyMQTTClient::MessageArrived(MQTT::MessageData & data)
{
    MQTT::Message &message = data.message;
    printf("\r\nMessage arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("\r\ndata.topicName.lenstring.data :-> %.*s\r\n", data.topicName.lenstring.len, data.topicName.lenstring.data);
    printf("\r\nmessage.payloadlen :-> %d\r\n", message.payloadlen);

    if (message.qos == MQTT::QOS0)
    {
        printf("\r\nMQTT::QOS0\r\n");
    }
    else if (message.qos == MQTT::QOS1)
    {
        printf("\r\nMQTT::QOS1\r\n");
    }
    else if (message.qos == MQTT::QOS2)
    {
        printf("\r\nMQTT::QOS2\r\n");
    }
    else
    {
        printf("\r\nMQTT::QOS??? :-> %d\r\n", message.qos);
    }

    if (message.payloadlen > 0)
    {
        printf("Binary Payload : \r\n\r\n%.*s\r\n", message.payloadlen, (char*)message.payload);
    }

    // Allow Yield to break out... implies some tricky context switching here.
    // Asynchronous callback from the underlying client versus our synchronuous context of the main thread.
    ++m_ArrivedMessagesCount;
}

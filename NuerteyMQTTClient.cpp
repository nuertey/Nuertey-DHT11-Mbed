#include "NuerteyMQTTClient.h"
#include "Utilities.h"
#include "lwip/arch.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "nuertey_mqtt.pb.h"

const std::string NuerteyMQTTClient::DEFAULT_MQTT_BROKER_ADDRESS("broker.hivemq.com");
const std::string NuerteyMQTTClient::DEFAULT_MQTT_CLIENT_IDENTIFIER("nuertey-nucleo_f767zi");
const std::string NuerteyMQTTClient::DEFAULT_MQTT_USERNAME("testuser");
const std::string NuerteyMQTTClient::DEFAULT_MQTT_PASSWORD("testpassword");
const uint16_t    NuerteyMQTTClient::DEFAULT_MQTT_BROKER_PORT;
const uint32_t    NuerteyMQTTClient::DEFAULT_TIME_TO_WAIT_FOR_RECEIVED_MESSAGE_MSECS;
const char * NuerteyMQTTClient::NUERTEY_ADDRESS_BOOK_MQTT_TOPIC = "/Nuertey/Nucleo/F767ZI/AddressBook";

// As we are constrained on embedded, prefer to send many topics with
// smaller payloads than one topic with a giant payload. This will also
// ensure that we do not run into hard limits such as the 512 string
// literal byte limit after concatenation (509 + terminators).
const char * NuerteyMQTTClient::NUCLEO_F767ZI_IOT_MQTT_TOPIC1 = "/Nuertey/Nucleo/F767ZI/NetworkInterface";
const char * NuerteyMQTTClient::NUCLEO_F767ZI_IOT_MQTT_TOPIC2 = "/Nuertey/Nucleo/F767ZI/SystemProfile";
const char * NuerteyMQTTClient::NUCLEO_F767ZI_IOT_MQTT_TOPIC3 = "/Nuertey/Nucleo/F767ZI/BaseRegisterValues";
const char * NuerteyMQTTClient::NUCLEO_F767ZI_IOT_MQTT_TOPIC4 = "/Nuertey/Nucleo/F767ZI/HeapStatistics";

const char * NuerteyMQTTClient::RELATIVE_TIME_MQTT_TOPIC = "/Nuertey/Nucleo/F767ZI/Time/Seconds/2500";
const char * NuerteyMQTTClient::ABSOLUTE_TIME_MQTT_TOPIC = "/Nuertey/Nucleo/F767ZI/Time/ISO8601/2018-10-20T06:30:06";

const char * NuerteyMQTTClient::NUCLEO_F767ZI_CONVERSATION_MQTT_TOPIC = "/Nuertey/Nucleo/F767ZI/Conversation";

uint64_t    NuerteyMQTTClient::m_ArrivedMessagesCount(0);
uint64_t    NuerteyMQTTClient::m_OldMessagesCount(0);

NuerteyMQTTClient::NuerteyMQTTClient(NetworkInterface * pNetworkInterface, const std::string & server, const uint16_t & port)
    : m_pNetworkInterface(pNetworkInterface)
    , m_MQTTNetwork(m_pNetworkInterface)
    , m_PahoMQTTclient(m_MQTTNetwork)
    , m_MQTTBrokerDomainName(std::nullopt)
    , m_MQTTBrokerAddress(server)
    , m_MQTTBrokerPort(port)
    , m_IsMQTTSessionEstablished(false)
{
}

NuerteyMQTTClient::~NuerteyMQTTClient()
{
}

void NuerteyMQTTClient::Connect()
{
    auto [ipAddress, domainName] = Utility::ResolveAddressIfDomainName(m_MQTTBrokerAddress);
    m_MQTTBrokerAddress = ipAddress;
    if (domainName)
    {
        m_MQTTBrokerDomainName = std::move(domainName);
    }

    Utility::g_SerialTerminal.printf("\r\nConnecting to : \"%s:%d\" ...", m_MQTTBrokerAddress.c_str(), m_MQTTBrokerPort);
    nsapi_error_t rc = m_MQTTNetwork.connect(m_MQTTBrokerAddress.c_str(), m_MQTTBrokerPort);
    if (rc != NSAPI_ERROR_OK)
    {
        Utility::g_SerialTerminal.printf("\r\n\r\nError! TCP.connect() returned: [%d] -> %s\n", rc, ToString(rc).c_str());
    }
    else
    {
        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        data.MQTTVersion = 3;
        data.clientID.cstring = (char *)DEFAULT_MQTT_CLIENT_IDENTIFIER.c_str();
        data.username.cstring = (char *)DEFAULT_MQTT_USERNAME.c_str();
        data.password.cstring = (char *)DEFAULT_MQTT_PASSWORD.c_str();
        // Broker should wipe the session each time we disconnect.
        // Otherwise our subscriptions are retained and messages
        // sent with Quality of Service 1 and 2 are buffered by the broker.
        data.cleansession = 1;
        int retVal = MQTT::FAILURE;
        if ((retVal = m_PahoMQTTclient.connect(data)) != MQTT::SUCCESS)
        {
            Utility::g_SerialTerminal.printf("\r\n\r\nError! MQTT.connect() returned: [%d].\n", retVal);
        }
        else
        {
            m_IsMQTTSessionEstablished = true;
            m_ArrivedMessagesCount = 0;
            Utility::g_SerialTerminal.printf("\r\n\r\nMQTT session established with broker at [%s:%d]\r\n", m_MQTTBrokerAddress.c_str(), m_MQTTBrokerPort);
        }
    }
}

void NuerteyMQTTClient::Disconnect()
{
    if (IsConnected())
    {
        Utility::g_SerialTerminal.printf("\r\nClosing session with broker : \"%s\" ...", (m_MQTTBrokerDomainName ? (*m_MQTTBrokerDomainName).c_str() : m_MQTTBrokerAddress.c_str()));
        int retVal = m_PahoMQTTclient.disconnect();
        if (retVal != MQTT::SUCCESS)
        {
            Utility::g_SerialTerminal.printf("\r\n\r\nError! MQTT.disconnect() returned: [%d].\n", retVal);
        }

        Utility::g_SerialTerminal.printf("\r\nDisconnecting from network : \"%s:%d\" ...", m_MQTTBrokerAddress.c_str(), m_MQTTBrokerPort);
        nsapi_error_t rc = m_MQTTNetwork.disconnect();
        if (rc != NSAPI_ERROR_OK)
        {
            Utility::g_SerialTerminal.printf("\r\n\r\nError! TCP.disconnect() returned: [%d] -> %s\n", rc, ToString(rc).c_str());
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
            Utility::g_SerialTerminal.printf("\r\n\r\nError! MQTT.subscribe() returned: [%d].\n", rc);
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
            Utility::g_SerialTerminal.printf("\r\n\r\nError! MQTT.unsubscribe() returned: [%d].\n", rc);
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
            Utility::g_SerialTerminal.printf("\r\n\r\nError! MQTT.publish() returned: [%d].\n", rc);
        }
        else
        {
            while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
            {
                printf("\r\nYielding... ");
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
        Utility::g_SerialTerminal.printf("\r\n\r\nError! MQTT.publish() returned: [%d].\n", rc);
    }
    else
    {
        while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
        {
            printf("\r\nYielding... ");
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
        Utility::g_SerialTerminal.printf("\r\n\r\nError! MQTT.publish() returned: [%d].\n", rc);
    }
    else
    {
        while (m_ArrivedMessagesCount < m_OldMessagesCount + 1)
        {
            printf("\r\nYielding... ");
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
    Utility::g_SerialTerminal.printf("\r\nMessage arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    Utility::g_SerialTerminal.printf("\r\ndata.topicName.lenstring.data :-> %.*s\r\n", data.topicName.lenstring.len, data.topicName.lenstring.data);
    Utility::g_SerialTerminal.printf("\r\nmessage.payloadlen :-> %d\r\n", message.payloadlen);

    if (message.qos == MQTT::QOS0)
    {
        Utility::g_SerialTerminal.printf("\r\nMQTT::QOS0\r\n");
    }
    else if (message.qos == MQTT::QOS1)
    {
        Utility::g_SerialTerminal.printf("\r\nMQTT::QOS1\r\n");
    }
    else if (message.qos == MQTT::QOS2)
    {
        Utility::g_SerialTerminal.printf("\r\nMQTT::QOS2\r\n");
    }
    else
    {
        Utility::g_SerialTerminal.printf("\r\nMQTT::QOS??? :-> %d\r\n", message.qos);
    }

    //if (message.payloadlen > 0)
    //{
    //Utility::g_SerialTerminal.printf("Binary Payload : \r\n\r\n%.*s\r\n", message.payloadlen, (char*)message.payload);
    //}

    // Allow Yield to break out... implies some tricky context switching here.
    // Asynchronous callback from the underlying client versus our synchronuous context of the main thread.
    ++m_ArrivedMessagesCount;

    if (strncmp(data.topicName.lenstring.data, NUERTEY_ADDRESS_BOOK_MQTT_TOPIC, data.topicName.lenstring.len) == 0)
    {
        // This manner of printing the data stream (%.*s) has the added
        // safety and advantage of ensuring that we don't run into buffer
        // overrun issues as there is no guarantee that said stream
        // was null-terminated at the origination (sending) endpoint.
        //Utility::g_SerialTerminal.printf("Binary Payload %.*s\r\n", message.payloadlen, (char*)message.payload);

        // As always, ensure to initialize structures so that the soon-to-be-decoded
        // message data does not start of with garbage values in its RAM location.
        nuertey_mqtt_AddressBook addressBook = nuertey_mqtt_AddressBook_init_zero;

        // Create a nanopb stream that reads from the incoming buffer.
        pb_istream_t stream = pb_istream_from_buffer((pb_byte_t *)message.payload, message.payloadlen);

        // Commence decoding and deserializing the message contents.
        bool status = pb_decode(&stream, nuertey_mqtt_AddressBook_fields, &addressBook);

        if (!status)
        {
            Utility::g_SerialTerminal.printf("Error! Nanopb Protocol Buffers message decoding failed: %s\r\n", PB_GET_ERROR(&stream));
        }
        else
        {
            for (pb_size_t i = 0; i < addressBook.people_count; i++)
            {
                Utility::g_SerialTerminal.printf("\r\nPerson ID :-> %ld", addressBook.people[i].id);
                Utility::g_SerialTerminal.printf("\r\nName :-> %s", addressBook.people[i].name);

                if (strcmp(addressBook.people[i].email, "") != 0)
                {
                    Utility::g_SerialTerminal.printf("\r\nE-mail address :-> %s", addressBook.people[i].email);
                }

                for (pb_size_t j = 0; j < addressBook.people[i].phones_count; j++)
                {
                    switch (addressBook.people[i].phones[j].type)
                    {
                    case nuertey_mqtt_Person_PhoneType_MOBILE:
                        Utility::g_SerialTerminal.printf("\r\nMobile phone #: ");
                        break;
                    case nuertey_mqtt_Person_PhoneType_HOME:
                        Utility::g_SerialTerminal.printf("\r\nHome phone #: ");
                        break;
                    case nuertey_mqtt_Person_PhoneType_WORK:
                        Utility::g_SerialTerminal.printf("\r\nWork phone #: ");
                        break;
                    default:
                        Utility::g_SerialTerminal.printf("\r\nUnknown phone #: ");
                        break;
                    }
                    Utility::g_SerialTerminal.printf("%s", addressBook.people[i].phones[j].number);
                }

                if (addressBook.people[i].which_employment == nuertey_mqtt_Person_unemployed_tag)
                {
                    Utility::g_SerialTerminal.printf("\r\nunemployed :-> %s", addressBook.people[i].employment.unemployed ? "true" : "false");
                }
                else if (addressBook.people[i].which_employment == nuertey_mqtt_Person_employer_tag)
                {
                    Utility::g_SerialTerminal.printf("\r\nemployer :-> %s", addressBook.people[i].employment.employer);
                }
                else if (addressBook.people[i].which_employment == nuertey_mqtt_Person_school_tag)
                {
                    Utility::g_SerialTerminal.printf("\r\nschool :-> %s", addressBook.people[i].employment.school);
                }
                else if (addressBook.people[i].which_employment == nuertey_mqtt_Person_selfEmployed_tag)
                {
                    Utility::g_SerialTerminal.printf("\r\nselfEemployed :-> %s", addressBook.people[i].employment.selfEmployed ? "true" : "false");
                }
                else
                {
                    Utility::g_SerialTerminal.printf("invalid employment oneof");
                }

                Utility::g_SerialTerminal.printf("\r\nUpdated: [%s]\r\n", (Utility::SecondsToString(addressBook.people[i].last_updated.seconds)).c_str());
            }
        }
    }
    else if ((strncmp(data.topicName.lenstring.data, NUCLEO_F767ZI_IOT_MQTT_TOPIC1, data.topicName.lenstring.len) == 0)
             || (strncmp(data.topicName.lenstring.data, NUCLEO_F767ZI_IOT_MQTT_TOPIC2, data.topicName.lenstring.len) == 0)
             || (strncmp(data.topicName.lenstring.data, NUCLEO_F767ZI_IOT_MQTT_TOPIC3, data.topicName.lenstring.len) == 0)
             || (strncmp(data.topicName.lenstring.data, NUCLEO_F767ZI_IOT_MQTT_TOPIC4, data.topicName.lenstring.len) == 0))
    {
        // This manner of printing the data stream (%.*s) has the added
        // safety and advantage of ensuring that we don't run into buffer
        // overrun issues as there is no guarantee that said stream
        // was null-terminated at the origination (sending) endpoint.
        Utility::g_SerialTerminal.printf("JSON Payload %.*s\r\n", message.payloadlen, (char*)message.payload);

    }
    else if ((strncmp(data.topicName.lenstring.data, RELATIVE_TIME_MQTT_TOPIC, data.topicName.lenstring.len) == 0)
             || (strncmp(data.topicName.lenstring.data, ABSOLUTE_TIME_MQTT_TOPIC, data.topicName.lenstring.len) == 0))
    {
        Utility::g_SerialTerminal.printf("No Payload is contained in these MQTT topics.\r\n");
    }
    else if (strncmp(data.topicName.lenstring.data, NUCLEO_F767ZI_CONVERSATION_MQTT_TOPIC, data.topicName.lenstring.len) == 0)
    {
        Utility::g_SerialTerminal.printf("\r\n%.*s\r\n", message.payloadlen, (char*)message.payload);
    }
}

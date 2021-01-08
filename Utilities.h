/***********************************************************************
* @file
*
* A miscellany of utilities for programming ARM Mbed-enabled targets.
*       
* @note     Quiet Thought is the Mother of Innovation. 
* 
* @warning  
* 
*  Created: October 15, 2018
*   Author: Nuertey Odzeyem        
************************************************************************/
#pragma once

#define PICOJSON_NO_EXCEPTIONS
#define PICOJSON_USE_INT64

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <ctime>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <memory>
#include <utility> 
#include <type_traits>
#include <algorithm>
#include <functional>
#include <optional>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <map>
#include <array>
#include <tuple>
#include <string>
#include <chrono>
#include "Date.h"
#include "jwt-mbed.h"
#include "nsapi_types.h"
#include "EthernetInterface.h"
#include "MQTTClient.h"
#include "NuerteyNTPClient.h"
//#include "mbed_mem_trace.h"
#include "randLIB.h"
#include "mbed_events.h"   // thread and irq safe
#include "mbed.h"

#if !defined(MBED_SYS_STATS_ENABLED)
#error "[NOT_SUPPORTED] MBED System Statistics Not Enabled"
#endif

#if !defined(MBED_HEAP_STATS_ENABLED)
#error "[NOT_SUPPORTED] MBED Heap Statistics Not Enabled"
#endif

#if !defined(MBED_CONF_NSAPI_SOCKET_STATS_ENABLED)
#error "[NOT_SUPPORTED] Socket Statistics not supported"
#endif

static const uint8_t     TOTAL_NUMBER_OF_HOURS_IN_A_DAY =   24;
static const uint16_t    MAXIMUM_WRITE_RETRIES          =   20;
static const uint16_t    MAXIMUM_READ_RETRIES           =   20; // Small timeout and many retries is preferred to... 
static const uint32_t    DEFAULT_TCP_SOCKET_TIMEOUT     =   40; // let the full-duplex socket do its thing.
static const uint32_t    DEFAULT_HTTP_SOCKET_TIMEOUT    =  100;
static const uint32_t    DEFAULT_HTTP_RESPONSE_SIZE     =  256;
static const uint32_t    MAXIMUM_HTTP_RESPONSE_SIZE     =  700;
static const uint32_t    MAXIMUM_WEBSOCKET_FRAME_SIZE   =  200;
    
// These clocks should NOT be relied on in embedded systems. Rather, use the RTC. 
using SystemClock_t         = std::chrono::system_clock;
using SteadyClock_t         = std::chrono::steady_clock;

using Seconds_t             = std::chrono::seconds;
using MilliSecs_t           = std::chrono::milliseconds;
using MicroSecs_t           = std::chrono::microseconds;
using NanoSecs_t            = std::chrono::nanoseconds;
using DoubleSecs_t          = std::chrono::duration<double>;
using FloatingMilliSecs_t   = std::chrono::duration<double, std::milli>;
using FloatingMicroSecs_t   = std::chrono::duration<double, std::micro>;

// Ensure that these are declared const so that the compiler can optimize
// and place the array dictionaries into Flash memory instead of RAM. Otherwise
// you are likely to run into "section .bss will not fit in region RAM"
// and "region RAM overflowed with stack" compiler errors.  
extern "C" const char adjective_txt;
extern "C" const unsigned int adjective_txt_len;
extern "C" const char adverb_txt;
extern "C" const unsigned int adverb_txt_len;
extern "C" const char noun_txt;
extern "C" const unsigned int noun_txt_len;
extern "C" const char preposition_txt;
extern "C" const unsigned int preposition_txt_len;
extern "C" const char pronoun_txt;
extern "C" const unsigned int pronoun_txt_len;
extern "C" const char verb_txt;
extern "C" const unsigned int verb_txt_len;

static const size_t ABSOLUTE_MAXIMUM_ADJECTIVE_DICTIONARY_SIZE   =   324; 
static const size_t ABSOLUTE_MAXIMUM_ADVERB_DICTIONARY_SIZE      =   142; 
static const size_t ABSOLUTE_MAXIMUM_NOUN_DICTIONARY_SIZE        =   104; 
static const size_t ABSOLUTE_MAXIMUM_PREPOSITION_DICTIONARY_SIZE =    70; 
static const size_t ABSOLUTE_MAXIMUM_PRONOUN_DICTIONARY_SIZE     =   269; 
static const size_t ABSOLUTE_MAXIMUM_VERB_DICTIONARY_SIZE        =   289; 

template<size_t N>
inline static auto make_dictionary(const char * pTextCollection, const unsigned int & length)
{
    // As we are on embedded, prefer static arrays to dynamic ones 
    // (i.e. std::vector). Never use std::vector on embedded as you are 
    // liable to run into " Operator new out of memory" exceptions.
    using Dictionary_t = std::array<std::string, N>;
    
    Dictionary_t dictionary = {};   
    const char * p = pTextCollection;     // Where the pointer points to can be changed but not what it points to. 
    const char * begin = pTextCollection; // Where the pointer points to can be changed but not what it points to.
    size_t index = 0;
     
    for (; p != pTextCollection + length; p++) 
    {
        if (*p == 0x0a)
        {
            dictionary.at(index) = std::string(begin, p - begin);
            index++;
            begin = p + 1;
        }
    }
    
    return dictionary;
}

static const auto gs_AdjectiveDictionary = make_dictionary<ABSOLUTE_MAXIMUM_ADJECTIVE_DICTIONARY_SIZE>(&adjective_txt, adjective_txt_len);
static const auto gs_AdverbDictionary = make_dictionary<ABSOLUTE_MAXIMUM_ADVERB_DICTIONARY_SIZE>(&adverb_txt, adverb_txt_len);
static const auto gs_NounDictionary = make_dictionary<ABSOLUTE_MAXIMUM_NOUN_DICTIONARY_SIZE>(&noun_txt, noun_txt_len);
static const auto gs_PrepositionDictionary = make_dictionary<ABSOLUTE_MAXIMUM_PREPOSITION_DICTIONARY_SIZE>(&preposition_txt, preposition_txt_len);
static const auto gs_PronounDictionary = make_dictionary<ABSOLUTE_MAXIMUM_PRONOUN_DICTIONARY_SIZE>(&pronoun_txt, pronoun_txt_len);
static const auto gs_VerbDictionary = make_dictionary<ABSOLUTE_MAXIMUM_VERB_DICTIONARY_SIZE>(&verb_txt, verb_txt_len);

static const uint32_t PRIME_TESTING_PERIOD_MSECS             =   2000;
static const uint32_t SENSOR_ACQUISITION_PERIOD_MSECS        =  30000; 
static const uint32_t HTTP_REQUEST_PERIOD_MSECS              =  15000;
static const uint32_t WEBSOCKET_MESSAGING_PERIOD_MSECS       =  20000;
static const uint32_t WEBSOCKET_STREAMING_PERIOD_MSECS       =  40000;
static const uint32_t NETWORK_DISCONNECT_QUERY_PERIOD_MSECS  =   1000;
static const uint32_t CLOUD_COMMUNICATIONS_EVENT_DELAY_MSECS =      3;

enum class MQTTConnectionError_t : int8_t
{
    SUCCESS_NO_ERROR                 = 0,
    UNACCEPTABLE_PROTOCOL_VERSION    = 1,
    IDENTIFIER_REJECTED              = 2,
    SERVER_UNAVAILABLE               = 3,
    BAD_USER_NAME_OR_PASSWORD        = 4,
    NOT_AUTHORIZED                   = 5,
    RESERVED                         = 6,
    MQTTCLIENT_FAILURE               = -1,
    MQTTCLIENT_DISCONNECTED          = -3,
    MQTTCLIENT_MAX_MESSAGES_INFLIGHT = -4,
    MQTTCLIENT_BAD_UTF8_STRING       = -5,
    MQTTCLIENT_NULL_PARAMETER        = -6,
    MQTTCLIENT_TOPICNAME_TRUNCATED   = -7,
    MQTTCLIENT_BAD_STRUCTURE         = -8,
    MQTTCLIENT_BAD_QOS               = -9,
    MQTTCLIENT_SSL_NOT_SUPPORTED     = -10,
    MQTTCLIENT_BAD_MQTT_VERSION      = -11,
    MQTTCLIENT_BAD_PROTOCOL          = -14,
    MQTTCLIENT_BAD_MQTT_OPTION       = -15,
    MQTTCLIENT_WRONG_MQTT_VERSION    = -16
};

using MQTTConnectionErrorMap_t = std::map<MQTTConnectionError_t, std::string>;
using IndexElementMQTT_t       = MQTTConnectionErrorMap_t::value_type;

inline static auto make_mqtt_connection_error_map()
{
    MQTTConnectionErrorMap_t eMap;
    
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::SUCCESS_NO_ERROR, std::string("\"Connection succeeded: no errors\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::UNACCEPTABLE_PROTOCOL_VERSION, std::string("\"Connection refused: Unacceptable protocol version\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::IDENTIFIER_REJECTED, std::string("\"Connection refused: Identifier rejected\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::SERVER_UNAVAILABLE, std::string("\"Connection refused: Server unavailable\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::BAD_USER_NAME_OR_PASSWORD, std::string("\"Connection refused: Bad user name or password\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::NOT_AUTHORIZED, std::string("\"Connection refused: Not authorized\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::RESERVED, std::string("\"Reserved for future use\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_FAILURE, std::string("\"Generic MQTT client operation failure\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_DISCONNECTED, std::string("\"The client is disconnected.\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_MAX_MESSAGES_INFLIGHT, std::string("\"The maximum number of messages allowed to be simultaneously in-flight has been reached.\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_BAD_UTF8_STRING, std::string("\"An invalid UTF-8 string has been detected.\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_NULL_PARAMETER, std::string("\"A NULL parameter has been supplied when this is invalid.\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_TOPICNAME_TRUNCATED, std::string("\"The topic has been truncated (the topic string includes embedded NULL characters). String functions will not access the full topic. Use the topic length value to access the full topic.\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_BAD_STRUCTURE, std::string("\"A structure parameter does not have the correct eyecatcher and version number.\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_BAD_QOS, std::string("\"A QoS value that falls outside of the acceptable range (0,1,2)\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_SSL_NOT_SUPPORTED, std::string("\"Attempting SSL connection using non-SSL version of library\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_BAD_MQTT_VERSION, std::string("\"unrecognized MQTT version\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_BAD_PROTOCOL, std::string("\"protocol prefix in serverURI should be tcp:// or ssl://\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_BAD_MQTT_OPTION, std::string("\"option not applicable to the requested version of MQTT\"")));
    eMap.insert(IndexElementMQTT_t(MQTTConnectionError_t::MQTTCLIENT_WRONG_MQTT_VERSION, std::string("\"call not applicable to the requested version of MQTT\"")));

    return eMap;
}

static MQTTConnectionErrorMap_t gs_MQTTConnectionErrorMap_t = make_mqtt_connection_error_map();

inline std::string ToString(const MQTTConnectionError_t & key)
{
    return (gs_MQTTConnectionErrorMap_t.at(key));
}

using ErrorCodesMap_t = std::map<nsapi_size_or_error_t, std::string>;
using IndexElement_t  = ErrorCodesMap_t::value_type;

inline static auto make_error_codes_map()
{
    ErrorCodesMap_t eMap;
    
    eMap.insert(IndexElement_t(NSAPI_ERROR_OK, std::string("\"no error\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_WOULD_BLOCK, std::string("\"no data is not available but call is non-blocking\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_UNSUPPORTED, std::string("\"unsupported functionality\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_PARAMETER, std::string("\"invalid configuration\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_NO_CONNECTION, std::string("\"not connected to a network\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_NO_SOCKET, std::string("\"socket not available for use\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_NO_ADDRESS, std::string("\"IP address is not known\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_NO_MEMORY, std::string("\"memory resource not available\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_NO_SSID, std::string("\"ssid not found\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_DNS_FAILURE, std::string("\"DNS failed to complete successfully\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_DHCP_FAILURE, std::string("\"DHCP failed to complete successfully\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_AUTH_FAILURE, std::string("\"connection to access point failed\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_DEVICE_ERROR, std::string("\"failure interfacing with the network processor\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_IN_PROGRESS, std::string("\"operation (eg connect) in progress\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_ALREADY, std::string("\"operation (eg connect) already in progress\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_IS_CONNECTED, std::string("\"socket is already connected\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_CONNECTION_LOST, std::string("\"connection lost\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_CONNECTION_TIMEOUT, std::string("\"connection timed out\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_ADDRESS_IN_USE, std::string("\"Address already in use\"")));
    eMap.insert(IndexElement_t(NSAPI_ERROR_TIMEOUT, std::string("\"operation timed out\"")));    
    return eMap;
}

static ErrorCodesMap_t gs_ErrorCodesMap = make_error_codes_map();

inline std::string ToString(const nsapi_size_or_error_t & key)
{
    return (gs_ErrorCodesMap.at(key));
}

enum class TimeTopic_t
{
    RELATIVE_TIME,
    ABSOLUTE_TIME   
};

enum class SocketMode_t
{
    BLOCKING,
    NON_BLOCKING   
};
    
namespace Utility
{
    extern std::string                      g_NetworkInterfaceInfo;
    extern std::string                      g_SystemProfile;
    extern std::string                      g_BaseRegisterValues;
    extern std::string                      g_HeapStatistics;

    extern EventQueue                       gs_MasterEventQueue;
    extern int                              gs_NetworkDisconnectEventIdentifier;
    extern int                              gs_SensorEventIdentifier;
    extern int                              gs_HTTPEventIdentifier;
    extern int                              gs_WebSocketEventIdentifier;
    extern int                              gs_WebSocketStreamEventIdentifier;
    extern int                              gs_PrimeEventIdentifier;
    extern int                              gs_CloudCommunicationsEventIdentifier;

    extern size_t                           g_MessageLength;
    extern std::unique_ptr<MQTT::Message>   g_pMessage;

    extern PlatformMutex                    g_STDIOMutex;
    extern EthernetInterface                g_EthernetInterface;
    extern NetworkInterface *               g_pNetworkInterface;
    //extern NTPClient                        g_NTPClient;
    extern NuerteyNTPClient                 g_NTPClient;

    void NetworkStatusCallback(nsapi_event_t status, intptr_t param);
    void NetworkDisconnectQuery();
    void InitializeGlobalResources();
    void ReleaseGlobalResources();
    //void PrintMemoryInfo();

    template <typename T, typename U>
    struct TrueTypesEquivalent : std::is_same<typename std::decay<T>::type, U>::type
    {};

    template <typename E>
    constexpr auto ToUnderlyingType(E e) -> typename std::underlying_type<E>::type
    {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

    template <typename E, typename V = unsigned long>
    constexpr auto ToEnum(V value) -> E
    {
        return static_cast<E>(value);
    }

    // This custom clock type obtains the time from RTC too whilst noting the Processor speed.
    struct NucleoF767ZIClock_t
    {
        using rep        = std::int64_t;
        using period     = std::ratio<1, 216'000'000>; // Processor speed, 1 tick == 4.62962963ns
        using duration   = std::chrono::duration<rep, period>;
        using time_point = std::chrono::time_point<NucleoF767ZIClock_t>;
        static constexpr bool is_steady = true;

        // This method/approach has been proven to work. Yay!
        static time_point now() noexcept
        {
            return from_time_t(time(NULL));
        }

        // This method/approach has been proven to work. Yay!
        static std::time_t to_time_t(const time_point& __t) noexcept
        {
            return std::time_t(std::chrono::duration_cast<Seconds_t>(__t.time_since_epoch()).count());
        }

        // This method/approach has been proven to work. Yay!
        static time_point from_time_t(std::time_t __t) noexcept
        {
            typedef std::chrono::time_point<NucleoF767ZIClock_t, Seconds_t> __from;
            return std::chrono::time_point_cast<NucleoF767ZIClock_t::duration>(__from(Seconds_t(__t)));
        }
    };

    // Now you can say things like:
    //
    // auto t = Utility::NucleoF767ZIClock_t::now();  // a chrono::time_point
    //
    // and
    //
    // auto d = Utility::NucleoF767ZIClock_t::now() - t;  // a chrono::duration

    // This custom clock type obtains the time from the 'chip-external' Real Time Clock (RTC).
    template <typename Clock_t = SystemClock_t>
    const auto Now = []()
    {
        time_t seconds = time(NULL);
        typename Clock_t::time_point tempTimepoint = Clock_t::from_time_t(seconds);

        return tempTimepoint;
    };

    const auto WhatTimeNow = []()
    {
        char buffer[32];
        time_t seconds = time(NULL);
        //printf("\r\nTime as seconds since January 1, 1970 = %lld", seconds);
        //printf("\r\nTime as a basic string = %s", ctime(&seconds));
        std::size_t len = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&seconds));
        std::string timeStr(buffer, len);

        //printf("\r\nDebug len :-> [%d]", len);
        //printf("\r\nDebug timeStr :-> [%s]", timeStr.c_str());

        return timeStr;
    };

    const auto SecondsToString = [](const time_t& seconds)
    {
        char buffer[32];
        std::size_t len = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&seconds));
        std::string timeStr(buffer, len);

        return timeStr;
    };

    template <typename T>
    typename std::make_unsigned<T>::type abs(T x)
    {
        // Cast before negating x to avoid overflow.
        return x < 0? -static_cast<decltype(abs(x))>(x) : x;
    }

    template <typename E>
    constexpr auto ToIntegral(E e) -> typename std::underlying_type<E>::type
    {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

    template <typename TimeUnits_t = MilliSecs_t>
    struct Measure
    {
        template <typename F, typename ...Args>
        static auto Execution(F&& func, Args&&... args)
        {
            auto start = Now<SteadyClock_t>();
            std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
            return (Now<SteadyClock_t>() - start).count();
        }

        template <typename F, typename ...Args>
        static auto Execution_Trunc(F&& func, Args&&... args)
        {
            auto start = Now<SteadyClock_t>();
            std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
            auto duration = std::chrono::duration_cast<TimeUnits_t>(Now<SteadyClock_t>() - start);
            return duration.count();
        }

        template <typename F, typename ...Args>
        static auto Duration(F&& func, Args&&... args)
        {
            auto start = Now<SteadyClock_t>();
            std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
            return (Now<SteadyClock_t>() - start);
        }

        template <typename F, typename ...Args>
        static auto Duration_Trunc(F&& func, Args&&... args)
        {
            auto start = Now<SteadyClock_t>();
            std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
            return std::chrono::duration_cast<TimeUnits_t>(Now<SteadyClock_t>() - start);
        }
    };

    const auto IsDomainNameAddress = [](const std::string & address)
    {
        bool result = false;

        if (!address.empty())
        {
            if (std::count_if(address.begin(), address.end(), [](unsigned char c){ return std::isalpha(c); } ) > 0)
            {
                result = true;
            }
        }
        return result;
    };

    // Be careful about designating the return here as const. It will
    // trap you in some sneaky behavior as a move from the std::optional
    // will rather invoke the copy constructor without the compiler complaining.
    auto ResolveAddressIfDomainName = [](const std::string & address)
    {
        std::optional<std::string> domainName(std::nullopt);
        std::string ipAddress = address;

        if (!address.empty())
        {
            if (IsDomainNameAddress(address))
            {
                domainName.emplace(address);
                SocketAddress serverSocketAddress;
                nsapi_size_or_error_t retVal;

                // Resolve Domain Name :
                do
                {
                    printf("\r\nPerforming DNS lookup for : \"%s\" ...", address.c_str());
                    retVal = g_EthernetInterface.gethostbyname(address.c_str(), &serverSocketAddress);
                    if (retVal < 0)
                    {
                        printf("\r\nError! On DNS lookup, Network returned: [%d] -> %s", retVal, ToString(retVal).c_str());
                    }
                }  while (retVal < 0);

                //g_EthernetInterface.get_ip_address(&serverSocketAddress);
                ipAddress = std::string(serverSocketAddress.get_ip_address());
            }
        }

        return std::make_pair(ipAddress, domainName);
    };

    template <typename T>
    std::string IntegerToHex(T i)
    {
        std::stringstream stream;
        stream << std::showbase << std::setfill('0') << std::setw(sizeof(T)) << std::hex << std::uppercase << static_cast<T>(i);
        return stream.str();
    }

    template <typename T>
    std::string IntegerToDec(T i)
    {
        std::stringstream stream;
        stream << std::dec << std::to_string(i);
        return stream.str();
    }

    const auto ComposeSystemStatistics = []()
    {
        const char * ip = g_EthernetInterface.get_ip_address();
        const char * netmask = g_EthernetInterface.get_netmask();
        const char * gateway = g_EthernetInterface.get_gateway();
        const char * mac = g_EthernetInterface.get_mac_address();

        mbed_stats_sys_t stats;
        mbed_stats_sys_get(&stats);

        uint8_t implementer = ((stats.cpu_id >> 24) & 0xff);
        uint8_t variant = ((stats.cpu_id >> 20) & 0x0f);
        uint8_t architecture = ((stats.cpu_id >> 16) & 0x0f);
        uint16_t partno = ((stats.cpu_id >> 4) & 0x0fff);
        uint8_t revno = (stats.cpu_id & 0x0f);

        mbed_stats_heap_t heapStats;
        mbed_stats_heap_get(&heapStats);

        // An alternative to the previous manual serialization approach is to
        // simply employ picojson with int64_t support and exceptions disabled.
        picojson::object jsonObject1;
        picojson::object jsonObject2;
        picojson::object jsonObject3;
        picojson::object jsonObject4;
        
        // Populate the object keys with an arbitrarily contrived alphabetic
        // prefix so that the eventual serialized prettified string would
        // be arranged in some manner conducive to use (i.e. preferred display format). 
        jsonObject1["[a] Module"] 
            = picojson::value("Nuertey Odzeyem - Nucleo-F767ZI Device Statistics");
        jsonObject1["[b] RTC Current Time"] 
            = picojson::value(WhatTimeNow());
        
        // Note that ternary operators must work on the same type for the
        // second and third operands, or implicitly convertible types anyway.
        jsonObject1["[c] MAC Address"] = picojson::value(std::string(mac ? mac : "None"));
        jsonObject1["[d] IP Address"] = picojson::value(std::string(ip ? ip : "None"));
        jsonObject1["[e] Netmask"] = picojson::value(std::string(netmask ? netmask : "None"));
        jsonObject1["[f] Gateway"] = picojson::value(std::string(gateway ? gateway : "None"));
        
        #ifdef MBED_MAJOR_VERSION
            jsonObject2["[g] MBED OS Version"] = picojson::value(
                              std::to_string(MBED_MAJOR_VERSION) + "."
                            + std::to_string(MBED_MINOR_VERSION) + "."
                            + std::to_string(MBED_PATCH_VERSION));
        #endif
        jsonObject2["[h] MBED OS Version (populated only for tagged releases)"] 
            = picojson::value(std::to_string(stats.os_version));
        jsonObject2["[i] Compiler ID"] 
            = picojson::value(std::string((stats.compiler_id == ARM) ? "ARM" : "")
            + std::string((stats.compiler_id == GCC_ARM) ? "GCC_ARM" : "")
            + std::string((stats.compiler_id == IAR) ? "IAR" : ""));
        jsonObject2["[j] Compiler Version"] 
            = picojson::value(std::to_string(stats.compiler_version));
        jsonObject2["[k] Device SystemClock"] 
            = picojson::value(std::to_string(SystemCoreClock) 
            + std::string(" Hz"));

        jsonObject3["[l] CPUID Base Register Values (Cortex-M only supported)"] 
            = picojson::value(IntegerToHex(stats.cpu_id));
        jsonObject3["[m] Implementer"] 
            = picojson::value(std::string((implementer == 0x41) ? "ARM" 
                                          : IntegerToHex(implementer)));
        jsonObject3["[n] Variant"] 
            = picojson::value(std::to_string(variant));
        jsonObject3["[o] Architecture"] 
            = picojson::value(std::string((architecture == 0x0c) ? "Baseline" : "")
            + std::string((architecture == 0x0f) ? "Constant i.e. Mainline" : ""));
        jsonObject3["[p] Part Number"] 
            = picojson::value(std::string((partno == 0x0c20) ? "Cortex-M0" : "")
            + std::string((partno == 0x0c60) ? "Cortex-M0+" : "")
            + std::string((partno == 0x0c23) ? "Cortex-M3" : "")
            + std::string((partno == 0x0c24) ? "Cortex-M4" : "")
            + std::string((partno == 0x0c27) ? "Cortex-M7" : "")
            + std::string((partno == 0x0d20) ? "Cortex-M23" : "")
            + std::string((partno == 0x0d21) ? "Cortex-M33" : ""));
        jsonObject3["[q] Revision"] = picojson::value(IntegerToHex(revno));

        jsonObject4["[r] Bytes allocated on heap"] 
            = picojson::value(std::to_string(heapStats.current_size));
        jsonObject4["[s] Maximum bytes allocated on heap at one time since reset"] 
            = picojson::value(std::to_string(heapStats.max_size));
        jsonObject4["[t] Cumulative sum of bytes allocated on heap not freed"] 
            = picojson::value(std::to_string(heapStats.total_size));
        jsonObject4["[u] Number of bytes reserved for heap"] 
            = picojson::value(std::to_string(heapStats.reserved_size));
        jsonObject4["[v] Number of allocations not freed since reset"] 
            = picojson::value(std::to_string(heapStats.alloc_cnt));
        jsonObject4["[w] Number of failed allocations since reset"] 
            = picojson::value(std::to_string(heapStats.alloc_fail_cnt));

        // The boolean is to enable prettified output with newlines.
        // JSON was not invented for humans after all, so otherwise things would look ugly. 
        std::string rawString1 = picojson::value(jsonObject1).serialize(true);
        std::string rawString2 = picojson::value(jsonObject2).serialize(true);
        std::string rawString3 = picojson::value(jsonObject3).serialize(true);
        std::string rawString4 = picojson::value(jsonObject4).serialize(true);
        
        return std::make_tuple(rawString1, rawString2, rawString3, rawString4);
    };

    const auto GenerateRandomSentence = []()
    {
        if ((gs_PronounDictionary.size() - 1) > std::numeric_limits<uint16_t>::max())
        {
            printf("\r\nWarning! gs_PronounDictionary is too large for randLIB to fully explore its range!!\r\n");
        }

        if ((gs_AdverbDictionary.size() - 1) > std::numeric_limits<uint16_t>::max())
        {
            printf("\r\nWarning! gs_AdverbDictionary is too large for randLIB to fully explore its range!!\r\n");
        }

        if ((gs_VerbDictionary.size() - 1) > std::numeric_limits<uint16_t>::max())
        {
            printf("\r\nWarning! gs_VerbDictionary is too large for randLIB to fully explore its range!!\r\n");
        }

        if ((gs_PrepositionDictionary.size() - 1) > std::numeric_limits<uint16_t>::max())
        {
            printf("\r\nWarning! gs_PrepositionDictionary is too large for randLIB to fully explore its range!!\r\n");
        }

        if ((gs_AdjectiveDictionary.size() - 1) > std::numeric_limits<uint16_t>::max())
        {
            printf("\r\nWarning! gs_AdjectiveDictionary is too large for randLIB to fully explore its range!!\r\n");
        }

        if ((gs_NounDictionary.size() - 1) > std::numeric_limits<uint16_t>::max())
        {
            printf("\r\nWarning! gs_NounDictionary is too large for randLIB to fully explore its range!!\r\n");
        }

        std::ostringstream oss;

        oss << gs_PronounDictionary.at(randLIB_get_random_in_range(static_cast<uint16_t>(0), gs_PronounDictionary.size() - 1))
            << " " << gs_AdverbDictionary.at(randLIB_get_random_in_range(static_cast<uint16_t>(0), gs_AdverbDictionary.size() - 1))
            << " " << gs_VerbDictionary.at(randLIB_get_random_in_range(static_cast<uint16_t>(0), gs_VerbDictionary.size() - 1))
            << " " << gs_PrepositionDictionary.at(randLIB_get_random_in_range(static_cast<uint16_t>(0), gs_PrepositionDictionary.size() - 1))
            << " the " << gs_AdjectiveDictionary.at(randLIB_get_random_in_range(static_cast<uint16_t>(0), gs_AdjectiveDictionary.size() - 1))
            << " " << gs_NounDictionary.at(randLIB_get_random_in_range(static_cast<uint16_t>(0), gs_NounDictionary.size() - 1)) << ".\n" ;

        return oss.str();
    };

    const auto SubstituteForDelimiter = [](const std::string & topic, const std::string & delimiter, const std::string & value)
    {
        std::optional<std::string> result(std::nullopt);

        if (!topic.empty())
        {
            std::size_t position = 0;
            if ((position = topic.find(delimiter)) != std::string::npos)
            {
                std::string tempString = topic;
                tempString.replace(position, delimiter.length(), value);
                result.emplace(tempString);
            }
        }
        return result;
    };

    const auto WhichTimeType = [](const std::string & topic)
    {
        std::optional<TimeTopic_t> timeType(std::nullopt);

        if (!topic.empty())
        {
            std::size_t position = 0;
            if ((position = topic.find(std::string("Time/"))) != std::string::npos)
            {
                std::string tempString = topic.substr(position + 5, 7);
                if (tempString == "Seconds")
                {
                    timeType.emplace(TimeTopic_t::RELATIVE_TIME);
                }
                else if (tempString == "ISO8601")
                {
                    timeType.emplace(TimeTopic_t::ABSOLUTE_TIME);
                }
            }
        }
        return timeType;
    };

    const auto Seconds2Timepoint = [](const std::string & topic)
    {
        std::optional<int> timepoint(std::nullopt);

        if (!topic.empty())
        {
            std::size_t position = 0;
            if ((position = topic.rfind(std::string("/"))) != std::string::npos)
            {
                std::string tempString = topic.substr(position + 1);
                timepoint.emplace(std::stoi(tempString));
                printf("\r\nParsed seconds :-> [%d]", *timepoint);
            }
        }
        return timepoint;
    };

    const auto ISO86012Timepoint = [](const std::string & topic)
    {
        using namespace date;
        using namespace std::chrono;

        std::optional<SystemClock_t::time_point> requestedTimepoint(std::nullopt);

        if (!topic.empty())
        {
            std::size_t position = 0;
            if ((position = topic.rfind(std::string("/"))) != std::string::npos)
            {
                int requestedYear = std::stoi(topic.substr(position + 1, 4));
                int requestedMonth = std::stoi(topic.substr(position + 6, 2));
                int requestedDay = std::stoi(topic.substr(position + 9, 2));
                int requestedHour = std::stoi(topic.substr(position + 12, 2));
                int requestedMinute = std::stoi(topic.substr(position + 15, 2));
                int requestedSecond = std::stoi(topic.substr(position + 18, 2));
                printf("\r\nParsed year :-> [%d]", requestedYear);
                printf("\r\nParsed month :-> [%d]", requestedMonth);
                printf("\r\nParsed day :-> [%d]", requestedDay);
                printf("\r\nParsed hour :-> [%d]", requestedHour);
                printf("\r\nParsed minute :-> [%d]", requestedMinute);
                printf("\r\nParsed second :-> [%d]", requestedSecond);

                auto tempDaypoint = date::year{requestedYear}/requestedMonth/requestedDay;
                if (tempDaypoint.ok())
                {
                    printf("\r\nYipee! Successfully constructed daypoint from user-supplied year/month/date.");
                    SystemClock_t::time_point tempTimepoint = sys_days(tempDaypoint) + hours(requestedHour) + minutes(requestedMinute) + seconds(requestedSecond);
                    if (tempTimepoint > Now<SystemClock_t>())
                    {
                        printf("\r\nExcellent! Requested timepoint is in the future. Wise is he whom hankers for the future. Hope shall always garland his brow regardless of his present fortunes.");
                        requestedTimepoint.emplace(tempTimepoint);
                        std::time_t reqTime_c = SystemClock_t::to_time_t(*requestedTimepoint);
                        std::ostringstream oss;
                        oss << std::put_time(std::localtime(&reqTime_c), "%F %T");
                        printf("\r\nRequested Timepoint :-> [%s]", oss.str().c_str());
                    }
                    else
                    {
                        printf("\r\nDear User! Please refrain from requesting a timepoint in the past. Nostalgia, can also kill the soul.");
                    }
                }
                else
                {
                    printf("\r\nUnable to construct daypoint from user-supplied year/month/date!");
                }
            }
        }
        return requestedTimepoint;
    };

    // Reference : https://stackoverflow.com/questions/4424374/determining-if-a-number-is-prime
    struct PrimeTester
    {
        auto PowerOf(size_t a, size_t n, size_t mod)
        {
            uint64_t power = a, result = 1;

            while (n)
            {
                if (n&1)
                    result = (result*power) % mod;
                power = (power*power) % mod;
                n >>= 1;
            }
            return result;
        }

        auto WitnessOf(size_t a, size_t n)
        {
            size_t t = 0, u = 0, i = 0;
            uint64_t prev = 0, curr = 0;

            u = n/2;
            t = 1;
            while (!(u&1))
            {
                u /= 2;
                ++t;
            }

            prev = PowerOf(a, u, n);
            for (i = 1; i <= t; ++i)
            {
                curr = (prev*prev) % n;
                if ((curr == 1) && (prev !=1) && (prev != n-1))
                    return true;
                prev = curr;
            }
            if (curr != 1)
                return true;
            return false;
        }

        // Deterministic variant of the Rabin-Miller algorithm, combined with optimized brute
        // forcing step, giving you one of the fastest prime testing functions out there.
        auto IsPrime(const size_t & arg)
        {
            size_t number = arg;
            if ( ( (!(number & 1)) && number != 2 ) || (number < 2) || (number % 3 == 0 && number != 3) )
                return (false);

            if (number < 1373653)
            {
                for (size_t k = 1; 36*k*k-12*k < number; ++k)
                    if ( (number % (6*k+1) == 0) || (number % (6*k-1) == 0) )
                        return (false);

                return true;
            }

            if (number < 9080191)
            {
                if (WitnessOf(31, number)) return false;
                if (WitnessOf(73, number)) return false;
                return true;
            }

            if (WitnessOf(2, number)) return false;
            if (WitnessOf(7, number)) return false;
            if (WitnessOf(61, number)) return false;
            return true;

            // WARNING: Algorithm deterministic only for numbers < 4,759,123,141 (unsigned int's max is 4294967296)
            // if n < 1,373,653, it is enough to test a = 2 and 3.
            // if n < 9,080,191, it is enough to test a = 31 and 73.
            // if n < 4,759,123,141, it is enough to test a = 2, 7, and 61.
            // if n < 2,152,302,898,747, it is enough to test a = 2, 3, 5, 7, and 11.
            // if n < 3,474,749,660,383, it is enough to test a = 2, 3, 5, 7, 11, and 13.
            // if n < 341,550,071,728,321, it is enough to test a = 2, 3, 5, 7, 11, 13, and 17.
        }
    };
} //end of namespace


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
//#include "NuerteyNTPClient.h"
#include "NTPClient.h"
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

using namespace std::chrono_literals;
    
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
    std::string result;

    // Prevent the possibility of std::out_of_range exception if the container
    // does not have an error element with the specified key.
    auto iter = gs_MQTTConnectionErrorMap_t.find(key);     
    if (iter != gs_MQTTConnectionErrorMap_t.end())
    {
        result = iter->second;
    }
    else
    {
        result = std::string("\"Warning! Code does not indicate an error and consequently does not exist in gs_MQTTConnectionErrorMap_t!\"");
    }
    
    return result;
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
    std::string result;

    // Prevent the possibility of std::out_of_range exception if the container
    // does not have an error element with the specified key.
    auto iter = gs_ErrorCodesMap.find(key);     
    if (iter != gs_ErrorCodesMap.end())
    {
        result = iter->second;
    }
    else
    {
        result = std::string("\"Warning! Code does not indicate an error and consequently does not exist in gs_ErrorCodesMap!\"");
    }
    
    return result;
}
    
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

//void DisplayLCDCapabilities();
void DHT11SensorAcquisition();

namespace Utility
{
    extern std::string                      g_NetworkInterfaceInfo;
    extern std::string                      g_SystemProfile;
    extern std::string                      g_BaseRegisterValues;
    extern std::string                      g_HeapStatistics;

    extern EventQueue *                     g_pMasterEventQueue;

    extern size_t                           g_MessageLength;
    extern std::unique_ptr<MQTT::Message>   g_pMessage;

    extern PlatformMutex                    g_STDIOMutex;
    extern EthernetInterface                g_EthernetInterface;
    extern NetworkInterface *               g_pNetworkInterface;
    extern NTPClient                        g_NTPClient;
    //extern NuerteyNTPClient                 g_NTPClient;

    void NetworkStatusCallback(nsapi_event_t status, intptr_t param);
    bool InitializeGlobalResources();
    void ReleaseGlobalResources();
    void DisplayStatistics();
    void RetrieveNTPTime();

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

    template <typename T>
    constexpr auto TruncateAndToString = [](const T& x, const int& decimalDigits)
    {
        std::stringstream ss;
        ss << std::fixed;
        ss.precision(decimalDigits);
        ss << x;
        return ss.str();
    };
    
    const auto TemperatureToString = [](const float& temperature)
    {
        std::ostringstream oss;  
        oss << std::fixed;
        oss.precision(2);
        oss << "Temp: " << temperature << " F";
        return oss.str();
    };
    
    const auto HumidityToString = [](const float& humidity)
    {
        std::ostringstream oss;  
        oss << std::fixed;
        oss.precision(2);
        oss << "Humi: " << humidity << " % RH";
        return oss.str();
    };

    template <typename E>
    constexpr auto ToIntegral(E e) -> typename std::underlying_type<E>::type
    {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

    const auto GetNetworkInterfaceProfile = [](NetworkInterface * pInterface)
    {
        std::optional<const char *> ip(std::nullopt);
        std::optional<const char *> netmask(std::nullopt);
        std::optional<const char *> gateway(std::nullopt);
        std::optional<const char *> mac(std::nullopt);
        
        // Retrieve the network address(es):
        SocketAddress socketAddress;
        pInterface->get_ip_address(&socketAddress);
        ip = socketAddress.get_ip_address();

        SocketAddress socketAddress1;
        pInterface->get_netmask(&socketAddress1);        
        netmask = socketAddress1.get_ip_address();
        
        SocketAddress socketAddress2;
        pInterface->get_gateway(&socketAddress2);
        gateway = socketAddress2.get_ip_address();
        
        // "Provided MAC address is intended for info or debug purposes
        // and may be not provided if the underlying network interface
        // does not provide a MAC address."
        mac = pInterface->get_mac_address();
        
        return std::make_tuple(ip, netmask, gateway, mac);
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

    const auto ResolveAddressIfDomainName = [](const std::string & address
                                             , NetworkInterface * pInterface
                                             , SocketAddress * pTheSocketAddress)
    {
        std::optional<std::string> ipAddress(std::nullopt);

        if (!address.empty())
        {
            if (IsDomainNameAddress(address))
            {
                // Note that the MBED_ASSERT macro is only available in the Debug 
                // and Development build profiles and not in the Release build profile. 
                MBED_ASSERT(pInterface);

                printf("Performing DNS lookup for : \"%s\" ...\n", address.c_str());
                nsapi_error_t retVal = pInterface->gethostbyname(address.c_str(), pTheSocketAddress);
                if (retVal < 0)
                {
                    printf("Error! On DNS lookup, Network returned: [%d] -> %s\n", retVal, ToString(retVal).c_str());
                }
                else
                {
                    ipAddress = pTheSocketAddress->get_ip_address();
                }
            }
            else
            {
                // Assume that we are already dealing with an IP address. i.e.,
                // as the designer of this application, I am mandating that the
                // user can choose to specify IP addresses directly in the mbed_app.json
                // echo-server-hostname field. This will facilitate testing with
                // locally hosted Echo Servers, which by necessity, do not have 
                // DNS names. 
                
                // In which case the following is how we can ensure that the requisite 
                // SocketAddress object that is normally created during FQDN resolution,
                // is manually constructed and propagated back to the User:
                SocketAddress tempSockAddress(address.c_str(), 0);
                *pTheSocketAddress = tempSockAddress;

                ipAddress = address.c_str();                
            }
        }

        return ipAddress;
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
        auto [ip, netmask, gateway, mac] = GetNetworkInterfaceProfile(g_pNetworkInterface);
        
        printf("Particular Network Interface IP address: %s\n", ip.value_or("(null)"));
        printf("Particular Network Interface Netmask: %s\n", netmask.value_or("(null)"));
        printf("Particular Network Interface Gateway: %s\n", gateway.value_or("(null)"));
        printf("Particular Network Interface MAC Address: %s\n", mac.value_or("(null)"));
        
        std::string ipString = ip.value_or("(null)");
        std::string netmaskString = netmask.value_or("(null)");
        std::string gatewayString = gateway.value_or("(null)");
        std::string macString = mac.value_or("(null)");

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
        jsonObject1["[c] MAC Address"] = picojson::value(macString);
        jsonObject1["[d] IP Address"] = picojson::value(ipString);
        jsonObject1["[e] Netmask"] = picojson::value(netmaskString);
        jsonObject1["[f] Gateway"] = picojson::value(gatewayString);
        
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
} //end of namespace


/***********************************************************************
* @file      NuerteyDHT11Device.h
*
*    DHT11/DHT22 temperature and humidity sensor driver targetted for  
*    ARM Mbed platform. 
* 
*    For ease of use, flexibility and readability of the code, the driver
*    is written in a modern C++ (C++17/C++20) templatized class/idiom. 
* 
*    As Bjarne Stroustrup is fond of saying, "C++ implementations obey 
*    the zero-overhead principle: What you don’t use, you don’t pay for."
*
* @brief   Drive the DHT11/DHT22 sensor module(s) in order to obtain 
*          temperature/humidity readings.
* 
* @note    The sensor peripheral (DHT11) component's details are as 
*          follows:
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
*      - Power Supply: 3 to 5V DC, 2.5mA max current use during 
*        conversion (while requesting data).
* 
*      - Operating range: Good for 20-80% humidity readings with 5% 
*        accuracy.
* 
*      - Good for 0-50°C temperature readings ±2°C accuracy.
* 
*      - No more than 1 Hz sampling rate (once every second).
* 
*      - Body size: 15.5mm x 12mm x 5.5mm. 
*
* @warning    These warnings are key to successful sensor operation: 
* 
*   [1] When the connecting cable to the data pin is shorter than 20 
*       metres, a 5K pull-up resistor is recommended;
* 
*   [2] When the connecting cable to the data pin is longer than 20 
*       metres, choose an appropriate pull-up resistor as needed.
* 
*   [3] When power is supplied to the sensor, do not send any instructions
*       to the sensor in within one second in order to pass the unstable
*       status phase. 
*
* @author    Nuertey Odzeyem
* 
* @date      April 1, 2021
*
* @copyright Copyright (c) 2021 Nuertey Odzeyem. All Rights Reserved.
***********************************************************************/
#pragma once

#include <type_traits>
#include <system_error>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <array>
#include <time.h> 
#include "mbed.h"
#include "Utilities.h"

#define PIN_HIGH  1
#define PIN_LOW   0

// Enforce that these errors should always be checked whenever and 
// wherever they are returned.

// TBD: Nuertey Odzeyem; double-check if the below actually does imply
// that we dont need a SUCCESS value in the enum and consequent message()
// translation. What would message() print out then?
//
// "Whatever the reason for failure, after create_directory() returns, 
// the error_code object ec will contain the OS-specific error code. On 
// the other hand, if the call was successful then ec contains a zero 
// value. This follows the tradition (used by errno and GetLastError())
// of having 0 indicate success and non-zero indicate failure."
enum class [[nodiscard]] SensorStatus_t : int8_t
{
    SUCCESS              =  0,
    ERROR_BUS_BUSY       = -1,
    ERROR_NOT_DETECTED   = -2,
    ERROR_ACK_TOO_LONG   = -3,
    ERROR_SYNC_TIMEOUT   = -4,
    ERROR_DATA_TIMEOUT   = -5,
    ERROR_BAD_CHECKSUM   = -6,
    ERROR_TOO_FAST_READS = -7
};

enum class TemperatureScale_t : uint8_t
{
    CELCIUS = 0,
    FARENHEIT,
    KELVIN
};

// Register for implicit conversion to error_code:
//
// For the SensorStatus_t enumerators to be usable as error_code constants,
// enable the conversion constructor using the is_error_code_enum type trait:
namespace std
{
    template <>
    struct is_error_code_enum<SensorStatus_t> : std::true_type {};
}

class DHT11ErrorCategory : public std::error_category
{
public:
    virtual const char* name() const noexcept override;
    virtual std::string message(int ev) const override;
};

const char* DHT11ErrorCategory::name() const noexcept
{
    return "DHT11-Sensor-Mbed";
}

std::string DHT11ErrorCategory::message(int ev) const
{
    switch (ToEnum<SensorStatus_t>(ev))
    {
        case SensorStatus_t::SUCCESS:
            return "Success - no errors";
            
        case SensorStatus_t::ERROR_BUS_BUSY:
            return "Communication failure - bus busy";

        case SensorStatus_t::ERROR_NOT_DETECTED:
            return "Communication failure - sensor not detected on bus";

        case SensorStatus_t::ERROR_ACK_TOO_LONG:
            return "Communication failure - ack too long";

        case SensorStatus_t::ERROR_SYNC_TIMEOUT:
            return "Communication failure - sync timeout";

        case SensorStatus_t::ERROR_DATA_TIMEOUT:
            return "Communication failure - data timeout";

        case SensorStatus_t::ERROR_BAD_CHECKSUM:
            return "Checksum error";

        case SensorStatus_t::ERROR_TOO_FAST_READS:
            return "Communication failure - too fast reads";            
        default:
            return "(unrecognized error)";
    }
}

inline const std::error_category& dht11_error_category()
{
    static DHT11ErrorCategory instance;
    return instance;
}

inline auto make_error_code(SensorStatus_t e)
{
    return std::error_code(ToUnderlyingType(e), dht11_error_category());
}

inline auto make_error_condition(SensorStatus_t e)
{
    return std::error_condition(ToUnderlyingType(e), dht11_error_category());
}

// Metaprogramming types to distinguish each sensor module type:
struct DHT11_t {};
struct DHT22_t {};

// Intrinsically enforce our pin requirements with C++20 Concepts.
template <PinName thePinName>
concept IsValidPinName = (thePinName != NC);

// Check and enforce restrictions on device pin at an earlier phase
// --compile-time--instead of at the more costlier runtime phase.
template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
class NuerteyDHT11Device
{
    static_assert(TrueTypesEquivalent<T, DHT11_t>::value
               || TrueTypesEquivalent<T, DHT22_t>::value,
    "Hey! NuerteyDHT11Device in its current form is only designed with DHT11, or DHT22 sensors in mind!!");

public:
    static constexpr uint8_t DHT11_MICROCONTROLLER_RESOLUTION_BITS =  8;
    static constexpr uint8_t SINGLE_BUS_DATA_FRAME_SIZE_BYTES      =  5;
    static constexpr uint8_t MAXIMUM_DATA_FRAME_SIZE_BITS          = 40; // 5x8
    static constexpr double  MINIMUM_SAMPLING_PERIOD_SECONDS       =  3; // Be conservative.

    using DataFrameBytes_t = std::array<uint8_t, SINGLE_BUS_DATA_FRAME_SIZE_BYTES>;
    using DataFrameBits_t  = std::array<uint8_t, MAXIMUM_DATA_FRAME_SIZE_BITS>;

    NuerteyDHT11Device();

    NuerteyDHT11Device(const NuerteyDHT11Device&) = delete;
    NuerteyDHT11Device& operator=(const NuerteyDHT11Device&) = delete;
    // Note that as the copy constructor and assignment operators above 
    // have been designated 'deleted', automagically, the move constructor
    // and move assignment operators would likewise be omitted for us by
    // the compiler, as indeed, we do intend. For after all, this driver  
    // class is not intended to be copied or moved as it has ownership 
    // of a unique hardware pin. Indeed, the Compiler is our friend. We 
    // simply have to play by its stringent rules. Simple!

    virtual ~NuerteyDHT11Device();

    [[nodiscard]] std::error_code ReadData();

    float GetHumidity() const;
    float GetTemperature(const TemperatureScale_t & Scale) const;
    float CalculateDewPoint(const float & celsius, const float & humidity) const;
    float CalculateDewPointFast(const float & celsius, const float & humidity) const;

protected:

private:
    [[nodiscard]] SensorStatus_t ExpectPulse(DigitalInOut & theIO, const int & level, const int & max_time);
    [[nodiscard]] SensorStatus_t ValidateChecksum();

    float CalculateTemperature() const;
    float CalculateHumidity() const;
    float ConvertCelsiusToFarenheit(const float & celcius) const;
    float ConvertCelsiusToKelvin(const float & celcius) const;

    PinName              m_TheDataPinName;
    DataFrameBytes_t     m_TheDataFrame;
    time_t               m_TheLastReadTime;
    std::error_code      m_TheLastReadResult;
    float                m_TheLastTemperature;
    float                m_TheLastHumidity;
};

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
NuerteyDHT11Device<T, thePinName>::NuerteyDHT11Device()
{   
    m_TheDataPinName = thePinName;
    
    // Using this value ensures that time(NULL) - m_TheLastReadTime will
    // be >= MINIMUM_SAMPLING_PERIOD_SECONDS the first time. Note that  
    // this assignment does wrap around, but so will the subtraction.

    // Typically for POSIX, the following formulation is enough since
    // time_t is denoted in seconds:
    m_TheLastReadTime = time(NULL) - MINIMUM_SAMPLING_PERIOD_SECONDS; 
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
NuerteyDHT11Device<T, thePinName>::~NuerteyDHT11Device()
{
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
std::error_code NuerteyDHT11Device<T, thePinName>::ReadData()
{
    // Check if sensor was read less than two seconds ago and return 
    // early to use last reading.
    auto currentTime = time(NULL);

    if (difftime(currentTime, m_TheLastReadTime) < MINIMUM_SAMPLING_PERIOD_SECONDS)
    {
        return m_TheLastReadResult; // return last correct measurement
    }

    std::error_code errorCode;
    auto result = SensorStatus_t::SUCCESS;
    m_TheLastReadTime = currentTime;

    // Reset 40 bits of previously received data to zero.
    m_TheDataFrame.fill(0);

    // DHT11 uses a simplified single-wire bidirectional communication protocol.
    // It follows a Master/Slave paradigm [NUCLEO-F767ZI=Master, DHT11=Slave] 
    // with the MCU observing these states:
    //
    // WAITING, READING.
    DigitalInOut theDigitalInOutPin(m_TheDataPinName);

    // MCU Sends out Start Signal to DHT:
    //
    // "Data Single-bus free status is at high voltage level. When the 
    // communication between MCU and DHT11 begins, the programme of MCU 
    // will set Data Single-bus voltage level from high to low."
    // 
    // https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf
    theDigitalInOutPin.mode(PullUp);
    
    // Just to allow things to stabilize:
    ThisThread::sleep_for(1ms);
    
    theDigitalInOutPin.output();
    theDigitalInOutPin = PIN_LOW;

    // As an alternative to SFINAE template techniques:
    if constexpr (std::is_same<T, DHT11_t>::value) 
    {
        // "...and this process must take at least 18ms to ensure DHT’s 
        // detection of MCU's signal", so err on the side of caution.
        ThisThread::sleep_for(20ms);
    }
    else if constexpr (std::is_same<T, DHT22_t>::value)
    {
        // The data sheet specifies, "at least 1ms", so err on the side 
        // of caution by doubling the amount. Per Mbed docs, spinning
        // with wait_us() on milliseconds here is not recommended as it
        // would affect multi-threaded performance.
        ThisThread::sleep_for(2ms);
    }

    uint8_t i = 0, j = 0, b = 0;
    DataFrameBits_t bitValue = {}; // Initialize to zeros.

    // "...then MCU will pull up voltage and wait 20-40us for DHT’s response."
    theDigitalInOutPin.mode(PullUp);

    // End the start signal by setting data line high for 30 microseconds.
    theDigitalInOutPin = PIN_HIGH;
    wait_us(30);
    theDigitalInOutPin.input();

    // Wait till the sensor grabs the bus.
    if (SensorStatus_t::SUCCESS != ExpectPulse(theDigitalInOutPin, 1, 40))
    {
        result = SensorStatus_t::ERROR_NOT_DETECTED;
        errorCode = make_error_code(result);
        m_TheLastReadResult = errorCode;
        return errorCode;
    }

    // Sensor should signal low 80us and then hi 80us.
    if (SensorStatus_t::SUCCESS != ExpectPulse(theDigitalInOutPin, 0, 100))
    {
        result = SensorStatus_t::ERROR_SYNC_TIMEOUT;
        errorCode = make_error_code(result);
        m_TheLastReadResult = errorCode;
        return errorCode;
    }

    if (SensorStatus_t::SUCCESS != ExpectPulse(theDigitalInOutPin, 1, 100)) [[unlikely]]
    {
        result = SensorStatus_t::ERROR_TOO_FAST_READS;
        errorCode = make_error_code(result);
        m_TheLastReadResult = errorCode;
        return errorCode;
    }
    else [[likely]]
    {
        // Timing critical code.
        {
            // TBD; Nuertey Odzeyem: We CANNOT use the CriticalSectionLock
            // here as ExpectPulse() calls wait_us(). As the Mbed docs 
            // further clarifies:
            //
            // "Note: You must not use time-consuming operations, standard 
            // library and RTOS functions inside critical section."
            //CriticalSectionLock  lock;

            // capture the data
            for (i = 0; i < SINGLE_BUS_DATA_FRAME_SIZE_BYTES; i++)
            {
                for (j = 0; j < DHT11_MICROCONTROLLER_RESOLUTION_BITS; j++)
                {
                    if (SensorStatus_t::SUCCESS != ExpectPulse(theDigitalInOutPin, 0, 75))
                    {
                        result = SensorStatus_t::ERROR_DATA_TIMEOUT;
                        errorCode = make_error_code(result);
                        m_TheLastReadResult = errorCode;
                        return errorCode;
                    }
                    // logic 0 is 28us max, 1 is 70us
                    wait_us(40);
                    bitValue[i*DHT11_MICROCONTROLLER_RESOLUTION_BITS + j] = theDigitalInOutPin;
                    if (SensorStatus_t::SUCCESS != ExpectPulse(theDigitalInOutPin, 1, 50))
                    {
                        result = SensorStatus_t::ERROR_DATA_TIMEOUT;
                        errorCode = make_error_code(result);
                        m_TheLastReadResult = errorCode;
                        return errorCode;
                    }
                }
            }
        } // End of timing critical code.

        // store the data
        for (i = 0; i < SINGLE_BUS_DATA_FRAME_SIZE_BYTES; i++)
        {
            b = 0;
            for (j = 0; j < DHT11_MICROCONTROLLER_RESOLUTION_BITS; j++)
            {
                if (bitValue[i*DHT11_MICROCONTROLLER_RESOLUTION_BITS + j] == 1)
                {
                    b |= (1 << (7-j));
                }
            }
            m_TheDataFrame[i] = b;
        }
        result = ValidateChecksum();
    }

    // Note that we are relying upon default-construction of std::error_code
    // being enough to indicate success as per standard practice.
    if (result != SensorStatus_t::SUCCESS)
    {
        errorCode = make_error_code(result);
    }
    m_TheLastReadResult = errorCode;
    
    return errorCode;
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
SensorStatus_t NuerteyDHT11Device<T, thePinName>::ExpectPulse(DigitalInOut & theIO, const int & level, const int & max_time)
{
    auto result = SensorStatus_t::SUCCESS;
 
    // This method essentially spins in a loop (i.e. polls) for every 
    // microsecond until the expected pulse arrives or we timeout.   
    int count = 0;
    while (level == theIO)
    {
        if (count > max_time)
        {
            result = SensorStatus_t::ERROR_TOO_FAST_READS;
            break;
        }
        count++;
        wait_us(1);
    }
    
    return result;
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
SensorStatus_t NuerteyDHT11Device<T, thePinName>::ValidateChecksum()
{
    auto result = SensorStatus_t::ERROR_BAD_CHECKSUM;
    
    // Per the sensor device specs./data sheet:
    if (m_TheDataFrame[4] == ((m_TheDataFrame[0] + m_TheDataFrame[1] + m_TheDataFrame[2] + m_TheDataFrame[3]) & 0xFF))
    {
        m_TheLastTemperature = CalculateTemperature();
        m_TheLastHumidity = CalculateHumidity();
        result = SensorStatus_t::SUCCESS;
    }
    
    return result;
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::CalculateTemperature() const
{
    auto v = 0;

    // As an alternative to SFINAE template techniques:
    if constexpr (std::is_same<T, DHT11_t>::value)
    {
        v = m_TheDataFrame[2];
    }
    else if constexpr (std::is_same<T, DHT22_t>::value)
    {
        v = m_TheDataFrame[2] & 0x7F;
        v *= 256;
        v += m_TheDataFrame[3];
        v /= 10;

        if (m_TheDataFrame[2] & 0x80)
        {
            v *= -1;
        }
    }

    return (static_cast<float>(v));
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::CalculateHumidity() const
{
    auto v = 0;

    // As an alternative to SFINAE template techniques:
    if constexpr (std::is_same<T, DHT11_t>::value)
    {
        v = m_TheDataFrame[0];
    }
    else if constexpr (std::is_same<T, DHT22_t>::value)
    {
        v = m_TheDataFrame[0];
        v *= 256;
        v += m_TheDataFrame[1];
        v /= 10;
    }

    return (static_cast<float>(v));
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::ConvertCelsiusToFarenheit(const float & celsius) const
{
    return ((celsius * 9/5) + 32);
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::ConvertCelsiusToKelvin(const float & celsius) const
{
    return (celsius + 273.15);
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::GetHumidity() const
{
    return m_TheLastHumidity;
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::GetTemperature(const TemperatureScale_t & Scale) const
{
    auto result = 0.0;

    if (Scale == TemperatureScale_t::FARENHEIT)
    {
        result = ConvertCelsiusToFarenheit(m_TheLastTemperature);
    }
    else if (Scale == TemperatureScale_t::KELVIN)
    {
        result = ConvertCelsiusToKelvin(m_TheLastTemperature);
    }
    else
    {
        result = m_TheLastTemperature;
    }

    return result;
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::CalculateDewPoint(const float & celsius, const float & humidity) const
{
    // dewPoint function NOAA
    // reference: http://wahiduddin.net/calc/density_algorithms.htm
    float A0= 373.15/(273.15 + celsius);
    float SUM = -7.90298 * (A0-1);
    SUM += 5.02808 * log10(A0);
    SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
    SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
    SUM += log10(1013.246);
    float VP = pow(10, SUM-3) * humidity;
    float tempVar = log(VP/0.61078);   // temp var

    return (241.88 * tempVar) / (17.558 - tempVar);
}

template <typename T, PinName thePinName>
    requires IsValidPinName<thePinName>
float NuerteyDHT11Device<T, thePinName>::CalculateDewPointFast(const float & celsius, const float & humidity) const
{
    // delta max = 0.6544 wrt dewPoint()
    // 5x faster than dewPoint()
    // reference: http://en.wikipedia.org/wiki/Dew_point
    float a = 17.271;
    float b = 237.7;
    float temp = (a * celsius) / (b + celsius) + log(humidity/100);
    float Td = (b * temp) / (a - temp);

    return Td;
}

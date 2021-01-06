/*
 *  DHT Library for  Digital-output Humidity and Temperature sensors
 *
 *  Works with DHT11, DHT21, DHT22
 *             SEN11301P,  Grove - Temperature&Humidity Sensor     (Seeed Studio)
 *             SEN51035P,  Grove - Temperature&Humidity Sensor Pro (Seeed Studio)
 *             AM2302   ,  temperature-humidity sensor
 *             RHT01,RHT02, RHT03    ,  Humidity and Temperature Sensor         (Sparkfun)
 *
 *  Copyright (C) Wim De Roeve
 *                based on DHT22 sensor library by HO WING KIT
 *                Arduino DHT11 library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include "mbed.h"

enum class eType : uint8_t
{
    DHT11     = 11,
    SEN11301P = 12,
    RHT01     = 13,
    DHT22     = 22,
    AM2302    = 23,
    SEN51035P = 24,
    RHT02     = 25,
    RHT03     = 26
};

enum class eError : uint8_t
{
    ERROR_NONE = 0,
    BUS_BUSY,
    ERROR_NOT_PRESENT,
    ERROR_ACK_TOO_LONG,
    ERROR_SYNC_TIMEOUT,
    ERROR_DATA_TIMEOUT,
    ERROR_CHECKSUM,
    ERROR_NO_PATIENCE
};

enum class eScale : uint8_t
{
    CELCIUS = 0,
    FARENHEIT,
    KELVIN
};

class DHT
{
public:
    DHT(PinName pin, eType DHTtype);
    ~DHT();
    [[nodiscard]] eError readData(void);
    float ReadHumidity(void);
    float ReadTemperature(eScale const Scale);
    float CalcdewPoint(float const celsius, float const humidity);
    float CalcdewPointFast(float const celsius, float const humidity);

private:
    time_t  _lastReadTime;
    float _lastTemperature;
    float _lastHumidity;
    PinName _pin;
    bool _firsttime;
    eType _DHTtype;
    uint8_t DHT_data[5];
    float CalcTemperature();
    float CalcHumidity();
    float ConvertCelciustoFarenheit(float const);
    float ConvertCelciustoKelvin(float const);
    eError stall(DigitalInOut &io, int const level, int const max_time);
};

/**
 * @brief       DHT11.h
 * @details     Temperature & humidity sensor complex with a calibrated digital signal output.
 *              Header file.
 *
 *
 * @return      N/A
 *
 * @author      Manuel Caballero
 * @date        08/August/2019
 * @version     08/August/2019    The ORIGIN
 * @pre         N/A
 * @warning     N/A
 * @pre         This code belongs to AqueronteBlog ( http://unbarquero.blogspot.com ).
 */
#ifndef DHT11_H_
#define DHT11_H_

#include "mbed.h"


/**
    Example:

@code
#include "mbed.h"
#include "DHT11.h"

DHT11 myDHT11 ( PB_8 );                                                         // DHT11 --> PB_8
Serial pc     ( USBTX, USBRX );                                                 // tx, rx

DigitalOut  myled   ( LED1 );
Ticker      newAction;


//@brief Constants.


//@brief Variables.
volatile uint32_t myState;                                                      // State that indicates when to perform a new sample


//@brief   FUNCTION PROTOTYPES
void changeDATA ( void );


//@brief FUNCTION FOR APPLICATION MAIN ENTRY.
int main()
{
    char                  myChecksum[] = "Ok";
    DHT11::DHT11_status_t aux;
    DHT11::DHT11_data_t   myDHT11_Data;

    pc.baud ( 115200 );

    myled   =   1;
    wait(3);
    myled   =   0;

    // DHT11 starts: Release the bus
    aux  =   myDHT11.DHT11_Init ();


    myState  =   0UL;                                                           // Reset the variable
    newAction.attach( &changeDATA, 1U );                                        // the address of the function to be attached ( changeDATA ) and the interval ( 1s )


    // Let the callbacks take care of everything
    while(1) {
        sleep();

        if ( myState == 1UL ) {
            myled = 1U;

            // Get a new data
            aux  =   myDHT11.DHT11_GetData ( &myDHT11_Data );

            // Check checksum to validate data
            if ( myDHT11_Data.checksumStatus == DHT11::DHT11_CHECKSUM_OK ) {
                myChecksum[0]  =   'O';
                myChecksum[1]  =   'k';
            } else {
                myChecksum[0]  =   'E';
                myChecksum[1]  =   'r';
            }

            // Send data through the UART
            pc.printf ( "T: %d C | RH: %d %% | Checksum: %s\r\n", myDHT11_Data.temperature, myDHT11_Data.humidity, myChecksum );


            // Reset the variables
            myState  =   0UL;
            myled    =   0U;
        }
    }
}


 // @brief       changeDATA ( void  )
 //
 // @details     It changes myState variable
 //
 // @param[in]    N/A
 //
 // @param[out]   N/A.
 //
 // @return       N/A.
 //
 // @author      Manuel Caballero
 // @date        24/June/2019
 // @version     24/June/2019   The ORIGIN
 // @pre         N/A
 // @warning     N/A.
void changeDATA ( void )
{
    myState  =   1UL;
}
@endcode
*/


/*!
 Library for the DHT11 Temperature & humidity sensor complex with a calibrated digital signal output.
*/
class DHT11
{
public:
    /**
    * @brief   DEVICE DELAYS.
    *           NOTE: Values in microseconds.
    */
    typedef enum
    {
        DHT11_START_SIGNAL              =   18000,         /*!<  Master: Start communication                    */
        DHT11_SAMPLE_DATA               =   40,            /*!<  Sample data time                               */
        DHT11_WAIT_FOR_SENSOR_RESPONSE  =   40             /*!<  Master: Wait for sensor response               */
    } DHT11_device_delays_t;


    /**
      * @brief   DEVICE BUS STATUS.
      *           NOTE: N/A.
      */
    typedef enum
    {
        DHT11_PIN_HIGH    =   1U,                           /*!<  Pin high                                        */
        DHT11_PIN_LOW     =   0U,                           /*!<  Pin low                                         */
        DHT11_PIN_UNKNOWN =   2U                            /*!<  Pin unknown                                     */
    } DHT11_device_bus_status_t;


    /**
      * @brief   CHECKSUM STATUS.
      *           NOTE: N/A.
      */
    typedef enum
    {
        DHT11_CHECKSUM_OK    =   0U,                       /*!<  Checksum correct                                */
        DHT11_CHECKSUM_ERROR =   1U                        /*!<  Checksum error                                  */
    } DHT11_checksum_status_t;


    /**
      * @brief   TIMEOUT.
      *           NOTE: This value is just an estimation, the user must check it out.
      */
    typedef enum
    {
        DHT11_TIMEOUT        =   0x23232                  /*!<  Timeout like a counter                          */
    } DHT11_timeout_t;




    /**
      * @brief   ERROR STATUS. INTERNAL CONSTANTS
      */
    typedef enum
    {
        DHT11_SUCCESS               =   0U,   /*!<  Communication success                           */
        DHT11_FAILURE               =   1U,   /*!<  Communication failure                           */
        DHT11_DATA_CORRUPTED        =   2U,   /*!<  Checksum error                                  */
        DHT11_BUS_TIMEOUT           =   3U    /*!<  Bus response timeout/error                      */
    } DHT11_status_t;





#ifndef DHT11_VECTOR_STRUCT_H
#define DHT11_VECTOR_STRUCT_H
    typedef struct
    {
        /* Outputs  */
        uint8_t   temperature;                    /*!<  Temperature value             */
        uint8_t   humidity;                       /*!<  Relative humidity value       */

        uint16_t  rawTemperature;                 /*!<  Temperature raw value         */
        uint16_t  rawHumidity;                    /*!<  Relative humidity raw value   */

        uint8_t   checksum;                       /*!<  Checksum                      */

        /* Checksum status   */
        DHT11_checksum_status_t checksumStatus;   /*!<  Checksum status               */
    } DHT11_data_t;
#endif






    /**
      * @brief   FUNCTION PROTOTYPES
      */
    /** Create an DHT11 object connected to the specified DHT11 device pin.
      *
      * @param dht11   Data pin
      */
    DHT11 ( PinName dht11 );

    /** Delete DHT11 object.
     */
    ~DHT11();

    /** It configures the GPIO peripheral.
      */
    DHT11_status_t DHT11_Init       ( void                    );

    /** It gets the raw data: Temperature, Humidity and Checksum.
      */
    DHT11_status_t DHT11_GetRawData ( DHT11_data_t* myRawData );

    /** It gets and calculates the current data: Temperature, Humidity and Checksum.
      */
    DHT11_status_t DHT11_GetData    ( DHT11_data_t* myData    );


private:
    DigitalInOut _dht11Pin;
};

#endif /* DHT11_H */

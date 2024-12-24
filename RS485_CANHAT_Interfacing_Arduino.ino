/**
 * @brief Expected CAN message data format for battery charge monitoring.
 * 
 * - **CAN ID**: `0x100` (as example desired Battery charge sender device ID).
 * - **Data Length**: 2 bytes.
 * - **Data Payload**:
 *   - **Byte 0**: Lower byte of battery charge value.
 *   - **Byte 1**: Higher byte of battery charge value.
 * 
 * 2-byte data payload will be combined into a single 16-bit integer representing the battery charge.
 * This value will then be scaled by `BATTERY_SCALE` to convert it into a percentage.
 * EXAMPLE output of expected data format is provided below, with some raw values assumed for the sake of example demo.
 * 
 * @example
 *
 *   // - CAN_BUS GET DATA -
 *   // CAN ID: 0x100 
 *   // Data Length: 2
 *   // Data: [0xC8, 0x00]
 *   // Battery Charge: 2.00 %
 */


#include <mcp_can.h>
#include <SPI.h>

/// @brief CAN-BUS Shield Initialisation
MCP_CAN CAN(10); ///< CAN object with CS set to pin 10

const unsigned long BATTERY_CHARGE_CAN_ID = 0x100; ///< I'll store the device ID of the battery info sender device into `BATTERY_CHARGE_CAN_ID`. Assume any value say 0x100*/
const float BATTERY_SCALE = 100.0; ///< We normally use percentage for knowing battery charge, so I'll use this Scale factor.

// Variables
volatile unsigned char Flag_Recv = 0; ///< Interrupt flag to indicate data reception
unsigned char len = 0; ///< Character length of received CAN data
unsigned char buf[8]; ///< 8 char buffer to store received CAN data

/**
 * @brief Interrupt Service Routine for MCP2515
 * 
 * Sets interrupt flag to 1 in order to indicate CAN message is received.
 */
void MCP2515_ISR()
{
    Flag_Recv = 1;
}

/**
 * @brief Setup function
 * 
 * Initialises the CAN-BUS Shield and Serial communication. 
 * Attaches an interrupt to handle CAN message reception.
 */
void setup()
{
    Serial.begin(115200); ///< Initialise Serial Monitor at 115200 baud rate

    /*Initialize the CAN-BUS Shield
    * Why 500 kbps when module supports upto 1Mbps? Just avoids signal integrity issues in PCB.
    * It also allows us to use longer length (approx 100 meteres)
    * I wouldn't want to overload the CAN Transceiver, I think 500Kbps suits well for our application (access battery charge)
    */
    while (CAN_OK != CAN.begin(CAN_500KBPS))
    {
        Serial.println("CAN BUS Shield initialization failed. Retrying...");
        delay(100); // Power on software delay + Initialisation delay + Retry delay
    }
    Serial.println("CAN BUS Shield initialized successfully.");

    /* Attach interrupt that triggers during clock falling edge. This is to handle CAN message reception
     *  Interrupt Pin = 0 is the Arduino Pin 2. So we connect the INT pin of the module to Pin 2 of our Arduino
     *  This executes `MCP2515_ISR` function to set receive interrupt flag initially. 
     */
    attachInterrupt(0, MCP2515_ISR, FALLING);
}

/**
 * @brief Loop function
 * 
 * Main program loop to check for and process received CAN messages.
 * Extracts and logs battery charge information if the CAN ID corresponding to battery charge, is received.
 */
void loop()
{
    // Check if a CAN message was received
    if (Flag_Recv)
    {
        Flag_Recv = 0; ///< Clears interrupt flag
        /* 
        *  To read received CAN messsage
        *  Iterates over the payload `m_nDta` (array with CAN message dat bytes) for copying valid bytes into the buffer array `buf` I provided.
        */
        CAN.readMsgBuf(&len, buf);
        unsigned long canId = CAN.getCanId();

        // Logs CAN ID and length of data received of "any" sender that has sent data 
        Serial.println("- CAN_BUS GET DATA -");
        Serial.print("CAN ID: "); Serial.println(canId, HEX); 
        Serial.print("Data Length: "); Serial.println(len);
        Serial.print("Data: ");
        for (int i = 0; i < len; i++)
        {
            Serial.print(buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();

        // Process battery charge data if CAN ID matches our sender of interest
        if (canId == BATTERY_CHARGE_CAN_ID)
        {
            float batteryCharge = buf[0] + (buf[1] << 8); ///< Combines bytes if charge is sent as 16-bit value
            batteryCharge /= BATTERY_SCALE; ///< Scales charge value to percentage
            Serial.print("Battery Charge: "); Serial.print(batteryCharge); Serial.println(" %");
        }
    }
}


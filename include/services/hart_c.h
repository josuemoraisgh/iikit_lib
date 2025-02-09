#include <HardwareSerial.h>
#include <services/wserial_c.h>


/**
 * @class HART
 * @brief Classe para gerenciamento de atualizações OTA.
 */
class Hart_c
{
private: 
    WSerial_c *wserial;

public:
    void setup(WSerial_c *ws)
    {
        wserial = ws;
        // Initialize UART1 at a baud rate of 115200
        Serial1.begin(115200, SERIAL_8N1, 16, 17); // RX = GPIO16, TX = GPIO17

        // Print a message to UART1
        Serial1.println("Hello, UART1!");
    }
    void loop()
    {
        // Check if data is available to read from UART1
        if (Serial1.available())
        {
            // Read the incoming byte
            char incomingByte = Serial1.read();

            // Print the received byte to the default Serial (UART0)
            wserial->print("Received: ");
            wserial->println(incomingByte);
        }
    }
};
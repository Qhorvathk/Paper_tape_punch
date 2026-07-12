#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

String inputBuffer = "";

void setup()
{
    Serial.begin(9600);
    SerialBT.begin("ESP32_PaperTape");
    Serial.println("Bluetooth started. Waiting for connection...");
}

void loop()
{
    if (SerialBT.available())
    {
        char c = SerialBT.read();

        if (c == '\n' || c == '\r')
        {
            if (inputBuffer.length() > 0)
            {
                Serial.println(inputBuffer);
                inputBuffer = "";
            }
        }
        else if (isAlphaNumeric(c))
        {
            inputBuffer += c;
        }
    }
}

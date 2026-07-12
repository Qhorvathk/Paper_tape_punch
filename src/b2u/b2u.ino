#include <NimBLEDevice.h>

// Nordic UART Service UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

NimBLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

#define BUFFER_SIZE 128
char inputBuffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
    {
        deviceConnected = true;
        Serial.println("Client connected");
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason)
    {
        deviceConnected = false;
        Serial.println("Client disconnected");
        NimBLEDevice::startAdvertising();
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
    {
        std::string rxValue = pCharacteristic->getValue();

        for (int i = 0; i < rxValue.length(); i++)
        {
            char c = rxValue[i];

            if (c == '\n' || c == '\r')
            {
                if (bufferIndex > 0)
                {
                    inputBuffer[bufferIndex] = '\0';
                    Serial.println(inputBuffer);
                    bufferIndex = 0;
                }
            }
            else if (isAlphaNumeric(c) && bufferIndex < BUFFER_SIZE - 1)
            {
                inputBuffer[bufferIndex++] = c;
            }
        }
    }
};

void setup()
{
    Serial.begin(9600);

    NimBLEDevice::init("ESP32_PaperTape");
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY);

    NimBLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pRxCharacteristic->setCallbacks(new RxCallbacks());

    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    // pAdvertising->setPreferredParams(6, 12);
    pAdvertising->start();

    Serial.println("BLE started. Waiting for connection...");
}

void loop()
{
    delay(10);
}

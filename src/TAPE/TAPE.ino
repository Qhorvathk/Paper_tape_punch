#include <NimBLEDevice.h>
///++++++++++++++++++++
#include <Arduino.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "soc/gpio_reg.h"
#include "esp_err.h"
#include "inputs.h"
#include "fonts.h"

///++++++++++++++++++++++++++

// Nordic UART Service UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

NimBLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

#define BUFFER_SIZE 128
char inputBuffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;

///+++++++++++++++++++++++
#define DRS GPIO_NUM_9             // Device Ready for Sending data  HIGH = TRUE
#define WD GPIO_NUM_10             // Data is on the bus             LOW->HIGH Triggering latch
#define READY_GPIO GPIO_NUM_11     // Paper puncher is ready         HIGH = TRUE
#define PAPER_OUT_GPIO GPIO_NUM_12 // Out of ribbon 				 LOW = TRUE
#define D0 GPIO_NUM_4              // GPIO assign for LSB
#define D1 GPIO_NUM_5              // GPIO assign
#define D2 GPIO_NUM_6              // GPIO assign
#define D3 GPIO_NUM_7              // GPIO assign
#define D4 GPIO_NUM_15             // GPIO assign
#define D5 GPIO_NUM_16             // GPIO assign
#define D6 GPIO_NUM_17             // GPIO assign
#define D7 GPIO_NUM_18             // GPIO assign for MSB

// Define your 8 safe GPIO pins in order (D0 to D7)
const int pin_bus[8] = {D0, D1, D2, D3, D4, D5, D6, D7};

// Global bitmask that covers all 8 pins combined
uint32_t set_mask = 0;
uint32_t bus_mask = 0;
unsigned long startTime = millis();

volatile bool Ready = false;
volatile bool PaperOut = false;
volatile bool New_name = false;
bool Request = false;

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
    {
        deviceConnected = true;
        Request = true;
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason)
    {
        deviceConnected = false;
        Request = false;
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
                    New_name = true; // semafor for recieved string
                }
            }
            else if (isPrintable(c) && bufferIndex < BUFFER_SIZE - 1)
            {
                inputBuffer[bufferIndex++] = c;
            }
        }
    }
};
// GPIO11 interrupt handler
static void IRAM_ATTR ready_isr_handler(void *arg)
{
    Ready = (gpio_get_level(READY_GPIO) != 0);
}

// GPIO12 interrupt handler
static void IRAM_ATTR paper_out_isr_handler(void *arg)
{
    PaperOut = (gpio_get_level(PAPER_OUT_GPIO) == 0);
}

void inputs_setup(void)
{
    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask =
        (1ULL << READY_GPIO) |
        (1ULL << PAPER_OUT_GPIO);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Read initial states
    Ready = (gpio_get_level(READY_GPIO) != 0);
    PaperOut = (gpio_get_level(PAPER_OUT_GPIO) == 0);

    // Install ISR service
    esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(err);
    }

    // Attach separate interrupt handlers
    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            READY_GPIO,
            ready_isr_handler,
            nullptr));

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            PAPER_OUT_GPIO,
            paper_out_isr_handler,
            nullptr));
};

// Optimized function to write an 8-bit value to the custom bus
void write8BitBus(uint8_t value)
{
    // Construct the mask based on which bits are 1
    set_mask = 0;
    if (value & (1 << 0))
        set_mask |= (1ULL << D0);
    if (value & (1 << 1))
        set_mask |= (1ULL << D1);
    if (value & (1 << 2))
        set_mask |= (1ULL << D2);
    if (value & (1 << 3))
        set_mask |= (1ULL << D3);
    if (value & (1 << 4))
        set_mask |= (1ULL << D4);
    if (value & (1 << 5))
        set_mask |= (1ULL << D5);
    if (value & (1 << 6))
        set_mask |= (1ULL << D6);
    if (value & (1 << 7))
        set_mask |= (1ULL << D7);
    // Read current register, clear our bus pins, inject new states,
    GPIO.out = (GPIO.out & ~bus_mask) | set_mask;
};
// Following the interface timing.
uint8_t PunchByte(uint8_t value)
{
    startTime = millis();
    while (!Ready && !PaperOut)
    { //  wait for ready or refil paper
        if (millis() - startTime >= 3000)
        {
            sendBleMessage("Device is not ready or Paper consumed!");
            return false;
        }; // Timeout after 3 sec
    };
    write8BitBus(value);
    delayMicroseconds(5);
    digitalWrite(WD, HIGH);
    delay(1);
    digitalWrite(WD, LOW);
    return true;
};

void sendBleMessage(const char *msg)
{
    if (deviceConnected)
    {
        String payload = String(msg) + "\n";
        pTxCharacteristic->setValue((uint8_t *)payload.c_str(), payload.length());
        pTxCharacteristic->notify();
    }
};

//===================== Entry for Setup
void setup()
{

    //  Serial.begin(115200);            // serial monitor

    // setup paperpunch's input
    inputs_setup();
    // setup bloetooth
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
    pAdvertising->setPreferredParams(6, 12);

    NimBLEAdvertisementData scanResponse;
    scanResponse.setName("ESP32_PaperTape");
    pAdvertising->setScanResponseData(scanResponse);

    pAdvertising->start(); // Serial.println("BLE started. Waiting for connection...");

    // setup  paperpunch's data and controll lines
    // Configure pins as outputs and calculate the global master mask
    for (int i = 0; i < 8; i++)
    {
        pinMode(pin_bus[i], OUTPUT);
        bus_mask |= (1ULL << pin_bus[i]);
    };

    // manageing databus default state
    pinMode(DRS, OUTPUT); // Device Ready to Send
    pinMode(WD, OUTPUT);  // WD

    // set lines to default level
    digitalWrite(DRS, LOW);
    digitalWrite(WD, LOW);
    write8BitBus(0x00);
};

//===================== Entry for Duty
void loop()
{
    if (Request && deviceConnected)
    {
        sendBleMessage("Adjon meg egy nevet!");
        Request = false;
    };

    if (New_name && deviceConnected)
    {
        sendBleMessage("Lyukasztas folyamatban...");

        uint8_t i = 0, j = 0, Return_status = false;

        while (New_name)
        {
            StartPunching();

            Return_status = PunchBuffer(inputBuffer, bufferIndex);

            if (!Return_status)
                break;

            Return_status = PunchByteMultiple(0x00, 5);
            if (!Return_status)
                break;

            Return_status = PunchBufferAsPictures(inputBuffer, bufferIndex);
            if (!Return_status)
                break;
        };

        sendBleMessage(!Return_status ? "Sikertelen lyukasztas!" : "Kilyukasztva.");

        bufferIndex = 0; // reset record lenght
        New_name = false;

        delay(1);

        sendBleMessage("Adjon meg egy nevet");
    };

    StopPunching();
};

void StartPunching()
{
    digitalWrite(DRS, HIGH); // Dataset Ready for sending
};

void StopPunching()
{
    digitalWrite(DRS, LOW); // No more punching
};

uint8_t PunchBuffer(char *buffer, uint8_t length)
{
    uint8_t i = 0, Return_status = false;
    for (i = 0; i < length; i++)
    {
        Return_status = PunchByte(buffer[i]); // punch out recor in binary
        if (!Return_status)
            break;
    };
    return Return_status;
};

uint8_t PunchByteMultiple(uint8_t value, uint8_t count)
{
    uint8_t i = 0;
    bool ret = false;
    for (i = 0; i < count; i++)
    {
        ret = PunchByte(0x00); // Punche one space

        if (!ret)
            break;
    };

    return ret;
};

uint8_t PunchBufferAsPictures(char *buffer, uint8_t length)
{
    uint8_t i = 0, j = 0, ret = false;
    for (i = 0; i < length; i++)
    {
        for (j = 0; j < 5; j++)
        {
            ret = PunchByte(Fonts5x7[buffer[i] - 0x20][j]); // relocate caracter to the begining of font table and select the font
            if (!ret)
                break;
        };
        ret = PunchByte(0x00);
        if (!ret)
            break;
    };
    return ret;
};

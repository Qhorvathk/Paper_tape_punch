#include <NimBLEDevice.h>
///++++++++++++++++++++
#include <Arduino.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "soc/gpio_reg.h"
#include "esp_err.h"
#include "C:\Users\karol\Local_Documents\Elektronika\ESP32\ESP_Projects\TAPE\inputs.h"
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
#define DRS GPIO_NUM_9       // Device Ready for Sending data  HIGH = TRUE
#define WD  GPIO_NUM_10      // Data is on the bus             LOW->HIGH Triggering latch 
#define READY_GPIO      GPIO_NUM_11 // Paper puncher is ready         HIGH = TRUE
#define PAPER_OUT_GPIO  GPIO_NUM_12 // Out of ribbon 				 LOW = TRUE
#define D0 GPIO_NUM_4       // GPIO assign for LSB
#define D1 GPIO_NUM_5       // GPIO assign
#define D2 GPIO_NUM_6       // GPIO assign
#define D3 GPIO_NUM_7       // GPIO assign
#define D4 GPIO_NUM_15      // GPIO assign
#define D5 GPIO_NUM_16      // GPIO assign
#define D6 GPIO_NUM_17      // GPIO assign
#define D7 GPIO_NUM_18      // GPIO assign for MSB

// Define your 8 safe GPIO pins in order (D0 to D7)
const int pin_bus[8] = {D0, D1, D2, D3, D4, D5, D6, D7};

uint8_t Fonts5x7[96][5] = {     // ASCII table without control characters.
   {0x00,0x00,0x00,0x00,0x00}, // sp
   {0x00, 0x2f, 0x00, 0x00, 0x00}, // !
   {0x00, 0x00, 0x07, 0x00, 0x00}, // "
   {0x00, 0x14, 0x7f, 0x14, 0x00}, // #
   {0x00, 0x7f, 0x2a, 0x12, 0x00}, // $
   {0x00, 0x08, 0x64, 0x62, 0x00}, // %
   {0x00, 0x55, 0x22, 0x50, 0x00}, // &
   {0x00, 0x03, 0x00, 0x00, 0x00}, // '
   {0x00, 0x22, 0x41, 0x00, 0x00}, // (
   {0x00, 0x41, 0x22, 0x1c, 0x00}, // )
   {0x00, 0x08, 0x3E, 0x08, 0x14}, // *
   {0x00, 0x08, 0x3E, 0x08, 0x08}, // +
   {0x00, 0x00, 0xA0, 0x60, 0x00}, // ,
   {0x00, 0x08, 0x08, 0x08, 0x08}, // -
   {0x00, 0x60, 0x60, 0x00, 0x00}, // .
   {0x00, 0x10, 0x08, 0x04, 0x02}, // /
   {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // 0
   {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
   {0x00, 0x61, 0x51, 0x49, 0x46}, // 2
   {0x00, 0x41, 0x45, 0x4B, 0x31}, // 3
   {0x00, 0x14, 0x12, 0x7F, 0x10}, // 4
   {0x00, 0x45, 0x45, 0x45, 0x39}, // 5
   {0x00, 0x4A, 0x49, 0x49, 0x30}, // 6
   {0x00, 0x71, 0x09, 0x05, 0x03}, // 7
   {0x00, 0x49, 0x49, 0x49, 0x36}, // 8
   {0x00, 0x49, 0x49, 0x29, 0x1E}, // 9
   {0x00, 0x36, 0x36, 0x00, 0x00}, // :
   {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
   {0x00, 0x14, 0x22, 0x41, 0x00}, // <
   {0x00, 0x14, 0x14, 0x14, 0x14}, // =
   {0x00, 0x41, 0x22, 0x14, 0x08}, // >
   {0x00, 0x01, 0x51, 0x09, 0x06}, // ?
   {0x00, 0x49, 0x59, 0x51, 0x3E}, // @
   {0x3E,0x48,0x88,0x48,0x3E}, // A
   {0x82,0xFE,0x92,0x92,0x6C}, // B
   {0x7C,0x82,0x82,0x82,0x44}, // C
   {0x82,0xFE,0x82,0x82,0x7C}, // D
   {0xFE,0x92,0x92,0x92,0x82}, // E
   {0xFE,0x90,0x90,0x90,0x80}, // F
   {0x7C,0x82,0x82,0x92,0x5E}, // G
   {0xFE,0x10,0x10,0x10,0xFE}, // H
   {0x00,0x82,0xFE,0x82,0x00}, // I
   {0x04,0x02,0x82,0xFC,0x80}, // J
   {0xFE,0x10,0x28,0x44,0x82}, // K
   {0xFE,0x02,0x02,0x02,0x02}, // L
   {0xFE,0x40,0x30,0x40,0xFE}, // M
   {0xFE,0x20,0x10,0x08,0xFE}, // N
   {0x7C,0x82,0x82,0x82,0x7C}, // O
   {0xFE,0x90,0x90,0x90,0x60}, // P
   {0x7C,0x82,0x8A,0x84,0x7A}, // Q
   {0xFE,0x90,0x98,0x94,0x62}, // R
   {0x64,0x92,0x92,0x92,0x4C}, // S
   {0x80,0x80,0xFE,0x80,0x80}, // T
   {0xFC,0x02,0x02,0x02,0xFC}, // U
   {0xF8,0x04,0x02,0x04,0xF8}, // V
   {0xFC,0x02,0x1C,0x02,0xFC}, // W
   {0xC6,0x28,0x10,0x28,0xC6}, // X
   {0xE0,0x10,0x0E,0x10,0xE0}, // Y
   {0x86,0x8A,0x92,0xA2,0xC2}, // Z
   {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
   {0x00, 0x2A, 0x55, 0x2A, 0x55}, // 55
   {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
   {0x00, 0x02, 0x01, 0x02, 0x04}, // ^
   {0x00, 0x40, 0x40, 0x40, 0x40}, // _
   {0x00, 0x01, 0x02, 0x04, 0x00}, // '
   {0x00, 0x54, 0x54, 0x54, 0x78}, // a
   {0x00, 0x48, 0x44, 0x44, 0x38}, // b
   {0x00, 0x44, 0x44, 0x44, 0x20}, // c
   {0x00, 0x44, 0x44, 0x48, 0x7F}, // d
   {0x00, 0x54, 0x54, 0x54, 0x18}, // e
   {0x00, 0x7E, 0x09, 0x01, 0x02}, // f
   {0x00, 0xA4, 0xA4, 0xA4, 0x7C}, // g
   {0x00, 0x08, 0x04, 0x04, 0x78}, // h
   {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
   {0x00, 0x80, 0x84, 0x7D, 0x00}, // j
   {0x00, 0x10, 0x28, 0x44, 0x00}, // k
   {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
   {0x00, 0x04, 0x18, 0x04, 0x78}, // m
   {0x00, 0x08, 0x04, 0x04, 0x78}, // n
   {0x00, 0x44, 0x44, 0x44, 0x38}, // o
   {0x00, 0x24, 0x24, 0x24, 0x18}, // p
   {0x00, 0x24, 0x24, 0x18, 0xFC}, // q
   {0x00, 0x08, 0x04, 0x04, 0x08}, // r
   {0x00, 0x54, 0x54, 0x54, 0x20}, // s
   {0x00, 0x3F, 0x44, 0x40, 0x20}, // t
   {0x00, 0x40, 0x40, 0x20, 0x7C}, // u
   {0x00, 0x20, 0x40, 0x20, 0x1C}, // v
   {0x00, 0x40, 0x30, 0x40, 0x3C}, // w
   {0x00, 0x28, 0x10, 0x28, 0x44}, // x
   {0x00, 0xA0, 0xA0, 0xA0, 0x7C}, // y
   {0x00, 0x64, 0x54, 0x4C, 0x44}, // z
   {0x00, 0x08, 0x77, 0x00, 0x00}, // {
   {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
   {0x00, 0x77, 0x08, 0x00, 0x00}, // }
   {0x00, 0x08, 0x10, 0x08, 0x00}, // ~
   {0x14, 0x14, 0x14, 0x14, 0x14}  // DEL
   };
//

// Global bitmask that covers all 8 pins combined
uint32_t set_mask = 0;
uint32_t bus_mask = 0;
unsigned long startTime = millis();


volatile bool Ready = false;
volatile bool PaperOut = false;
volatile bool New_name = false;
bool Request = true;

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
    {
        deviceConnected = true;
    //    Serial.println("Client connected");
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason)
    {
        deviceConnected = false;
    //    Serial.println("Client disconnected");
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
					New_name = true;                    // semafor for recieved string 
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
            nullptr
        )
    );

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            PAPER_OUT_GPIO,
            paper_out_isr_handler,
            nullptr
        )
    );
};

// Optimized function to write an 8-bit value to the custom bus
void write8BitBus(uint8_t  value) {
// Construct the mask based on which bits are 1
   set_mask = 0;
   if (value & (1 << 0)) set_mask |= (1ULL << D0);
   if (value & (1 << 1)) set_mask |= (1ULL << D1);
   if (value & (1 << 2)) set_mask |= (1ULL << D2);
   if (value & (1 << 3)) set_mask |= (1ULL << D3);
   if (value & (1 << 4)) set_mask |= (1ULL << D4);
   if (value & (1 << 5)) set_mask |= (1ULL << D5);
   if (value & (1 << 6)) set_mask |= (1ULL << D6);
   if (value & (1 << 7)) set_mask |= (1ULL << D7);
 // Read current register, clear our bus pins, inject new states,
 GPIO.out = (GPIO.out & ~bus_mask) | set_mask;
};
// Following the interface timing.
 uint8_t PunchByte (uint8_t value) {
    startTime = millis() ;
    while (!Ready && !PaperOut) {       //  wait for ready or refil paper
        if (millis() - startTime >= 3000) {
         sendBleMessage("Device is not ready or Paper consumed!");   
        return false;   };                 // Timeout after 3 sec   
     };   
    write8BitBus(value);
    delayMicroseconds(5);
    digitalWrite(WD, HIGH);
    delay(1);
    digitalWrite(WD, LOW);
    return true ;
};

void sendBleMessage(const char *msg) {
    if (deviceConnected) {
        String payload = String(msg) + "\n";
        pTxCharacteristic->setValue((uint8_t *)payload.c_str(), payload.length());
        pTxCharacteristic->notify();
    }
};



//===================== Entry for Setup
void setup() {

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

    pAdvertising->start();     // Serial.println("BLE started. Waiting for connection...");
	
 // setup  paperpunch's data and controll lines
 // Configure pins as outputs and calculate the global master mask
 for(int i = 0 ; i < 8 ; i++) {
   pinMode(pin_bus[i],OUTPUT);
   bus_mask |= ( 1ULL << pin_bus[i]); };

 // manageing databus default state
    pinMode(DRS, OUTPUT);  // Device Ready to Send
    pinMode(WD,  OUTPUT);  // WD

 // set lines to default level
    digitalWrite(DRS, LOW); 
    digitalWrite(WD, LOW);
    write8BitBus(0x00); 
};


//===================== Entry for Duty
void loop()  {
 // put your main code here, to run repeatedly:
  uint8_t i = 0, j = 0 , Return_status = false;
  while (New_name) {                 // Bluethoot sent a record
        digitalWrite(DRS, HIGH);          // Dataset Ready for sending
        //
        for( i=0; i< bufferIndex; i++) {
             Return_status = PunchByte(inputBuffer[i]);      //punch out recor in binary
             if (!Return_status) {break;}
             };
        for( i=0; i< 5; i++) {
             Return_status = PunchByte(0x00);      // Punche one space
             if (!Return_status) {break;}
            };         
        // punch out the record as a pictures of the record's characters.
        for( i=0; i< bufferIndex; i++) {     // loop for converting record to caracter
             for ( j=0; j<5; j++) {             // loop for converting caracter to font
                 Return_status = PunchByte(Fonts5x7[inputBuffer[i]-0x20] [j] );  // relocate caracter to the begining of font table and select the font
                 if (!Return_status) {break;}
                 };
             Return_status = PunchByte(0x00);               // left one column among the character
             if (!Return_status) {break;}
             };
         //
         if (!Return_status) { sendBleMessage("Sikertelen lyukasztas!");} else {sendBleMessage("Kilyukasztva");}
         New_name = false;      // record has benn punched out
         bufferIndex = 0;       // reset record lenght
         Request = true;        // asking for new name
     } ;
    if(Request && deviceConnected) {delay(1);};  // ??????????????? enélkül nem mehgy ????????, de elsőre így sem írja ki. 
    if(Request && deviceConnected) {
      sendBleMessage("Adjon meg egy nevet");
      Request = false ;
      };
    delay(500);
    digitalWrite(DRS, LOW);  // No more punching
 };
 





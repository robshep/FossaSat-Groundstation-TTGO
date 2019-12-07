/*
 * FOSSA Ground Station Example
 *
 * Tested on Arduino Uno and SX1278, can be used with any LoRa radio
 * from the SX127x or SX126x series. Make sure radio type (line 21)
 * and pin mapping (lines 26 - 29) match your hardware!
 *
 * References:
 *
 * RadioLib error codes:
 * https://jgromes.github.io/RadioLib/group__status__codes.html
 *
 * FOSSASAT-1 Communication Guide:
 * https://github.com/FOSSASystems/FOSSASAT-1/blob/master/FOSSA%20Documents/FOSSASAT-1%20Comms%20Guide.pdf
 *
 */

// include all libraries
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "images.h"

#include <RadioLib.h>
#include <FOSSA-Comms.h>

#define RADIO_TYPE        SX1278  // type of radio module to be used
//#define RADIO_SX126X            // also uncomment this line when using SX126x!!!

//#define SCK     5    // GPIO5  -- SX1278's SCK
//#define MISO    19   // GPIO19 -- SX1278's MISO
//#define MOSI    27   // GPIO27 -- SX1278's MOSI
//#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
//#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)


// pin definitions
#define CS                18
#define DIO0              26
#define DIO1              23 // Rob's specific jumper wiring to IO23
//#define BUSY              

// modem configuration
#define FREQUENCY         436.7   // MHz
#define BANDWIDTH         125.0   // kHz
#define SPREADING_FACTOR  11
#define CODING_RATE       8       // 4/8
#define SYNC_WORD_7X      0xFF    // sync word when using SX127x
#define SYNC_WORD_6X      0x0F0F  //                      SX126x

// set up radio module
#ifdef RADIO_SX126X
  RADIO_TYPE radio = new Module(CS, DIO0, BUSY);
#else
  RADIO_TYPE radio = new Module(CS, DIO0, DIO1);
#endif

SSD1306  display(0x3c, 21, 22); //5,4  21,22 4,15

// flags
volatile bool interruptEnabled = true;
volatile bool transmissionReceived = false;

// satellite callsign
char callsign[] = "FOSSASAT-1";

void updateProgress(int pc) {
    display.clear();
    // draw the progress bar
    display.drawProgressBar(0, 32, 120, 10, pc);

    // draw the percentage as String
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(pc) + "%");
    display.display();
    delay(100);
}

/* whole op does the clear before and render after */
void drawImage(bool whole_op) {
    if(whole_op) display.clear();
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    display.drawXbm(0, 0, Logo_width, Logo_height, Logo_bytes);
    if(whole_op) display.display();
}

// radio ISR
void onInterrupt() {
  if(!interruptEnabled) {
    return;
  }

  transmissionReceived = true;
}

// function to print controls
void printControls() {
  Serial.println(F("------------- Controls -------------"));
  Serial.println(F("p - send ping frame"));
  Serial.println(F("i - request satellite info"));
  Serial.println(F("l - request last packet info"));
  Serial.println(F("r - send message to be retransmitted"));
  Serial.println(F("------------------------------------"));
}

void sendPing() {
  Serial.print(F("Sending ping frame ... "));

  // data to transmit
  uint8_t functionId = CMD_PING;

  // build frame
  uint8_t len = FCP_Get_Frame_Length(callsign);
  uint8_t* frame = new uint8_t[len];
  FCP_Encode(frame, callsign, functionId);

  // send data
  int state = radio.transmit(frame, len);
  delete[] frame;

  // check transmission success
  if (state == ERR_NONE) {
    Serial.println(F("sent successfully!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void requestInfo() {
  Serial.print(F("Requesting system info ... "));

  // data to transmit
  uint8_t functionId = CMD_TRANSMIT_SYSTEM_INFO;

  // build frame
  uint8_t len = FCP_Get_Frame_Length(callsign);
  uint8_t* frame = new uint8_t[len];
  FCP_Encode(frame, callsign, functionId);

  // send data
  int state = radio.transmit(frame, len);
  delete[] frame;

  // check transmission success
  if (state == ERR_NONE) {
    Serial.println(F("sent successfully!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void requestPacketInfo() {
  Serial.print(F("Requesting last packet info ... "));

  // data to transmit
  uint8_t functionId = CMD_GET_LAST_PACKET_INFO;

  // build frame
  uint8_t len = FCP_Get_Frame_Length(callsign);
  uint8_t* frame = new uint8_t[len];
  FCP_Encode(frame, callsign, functionId);

  // send data
  int state = radio.transmit(frame, len);
  delete[] frame;

  // check transmission success
  if (state == ERR_NONE) {
    Serial.println(F("sent successfully!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void requestRetransmit() {
  Serial.println(F("Enter message to be sent:"));
  Serial.println(F("(max 32 characters, end with LF or CR+LF)"));

  // get data to be retransmited
  char optData[32];
  uint8_t bufferPos = 0;
  while(bufferPos < 32) {
    while(!Serial.available());
    char c = Serial.read();
    Serial.print(c);
    if((c != '\r') && (c != '\n')) {
      optData[bufferPos] = c;
      bufferPos++;
    } else {
      break;
    }
  }

  // wait for a bit to receive any trailing characters
  delay(100);

  // dump the serial buffer
  while(Serial.available()) {
    Serial.read();
  }

  Serial.println();
  Serial.print(F("Requesting retransmission ... "));

  // data to transmit
  uint8_t functionId = CMD_RETRANSMIT;
  optData[bufferPos] = '\0';
  uint8_t optDataLen = strlen(optData);

  // build frame
  uint8_t len = FCP_Get_Frame_Length(callsign, optDataLen);
  uint8_t* frame = new uint8_t[len];
  FCP_Encode(frame, callsign, functionId, optDataLen, (uint8_t*)optData);

  // send data
  int state = radio.transmit(frame, len);
  delete[] frame;

  // check transmission success
  if (state == ERR_NONE) {
    Serial.println(F("sent successfully!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}

void setup() {
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  updateProgress(0);
  Serial.begin(115200);
  Serial.println(F("FOSSA Ground Station Demo Code"));
  updateProgress(10);
  
  updateProgress(25);

  
  updateProgress(35);
  pinMode(RST, INPUT);  // Let the pin float
  digitalWrite(RST, LOW);
  pinMode(RST, OUTPUT);
  delayMicroseconds(100);  // Pull low for 100 microseconds to force reset
  pinMode(RST, INPUT);
  delay(5);  // Chip should be ready 5ms after low pulse
  updateProgress(50);
  // initialize the radio
  #ifdef RADIO_SX126X
  int state = radio.begin(FREQUENCY,
                          BANDWIDTH,
                          SPREADING_FACTOR,
                          CODING_RATE,
                          SYNC_WORD_6X);
  #else
  int state = radio.begin(FREQUENCY,
                          BANDWIDTH,
                          SPREADING_FACTOR,
                          CODING_RATE,
                          SYNC_WORD_7X);
  #endif
  if(state == ERR_NONE) {
    Serial.println(F("Radio initialization successful!"));
  } else {
    Serial.print(F("Failed to initialize radio, code: "));
    Serial.println(state);
    while(true);
  }

  updateProgress(60);

  // attach the ISR to radio interrupt
  #ifdef RADIO_SX126X
  radio.setDio1Action(onInterrupt);
  #else
  radio.setDio0Action(onInterrupt);
  #endif

  updateProgress(75);
  // begin listening for packets
  radio.startReceive();

  updateProgress(100);

  drawImage(true);
  delay(1500);
  //display.clear();
  //display.display();

  //printControls();
}

int packets_rx = 0;;
int packets_fwd = 0;
int count_idx = 0;
unsigned long last_update = millis();
unsigned int update_idx = 0;

void updateStatus(){
  display.clear();
  drawImage(false);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  update_idx = (update_idx+1) % 4;
  char dots[5];
  sprintf(dots, "%.*s", update_idx, "......");
  display.drawStringMaxWidth(55, 40, 128, "Receiving" + String(dots) );
  display.display();
}

void loop() {
  
  // RX only for now

  // check if new data were received
  if(transmissionReceived) {
    // disable reception interrupt
    interruptEnabled = false;
    transmissionReceived = false;
    packets_rx++;

    // read received data
    size_t respLen = radio.getPacketLength();
    uint8_t* respFrame = new uint8_t[respLen];
    int state = radio.readData(respFrame, respLen);

    // check reception success
    if (state == ERR_NONE) {
      // print raw data
      Serial.print(F("Received "));
      Serial.print(respLen);
      Serial.println(F(" bytes:"));
      PRINT_BUFF(respFrame, respLen);

      // get function ID
      uint8_t functionId = FCP_Get_FunctionID(callsign, respFrame, respLen);
      Serial.print(F("Function ID: 0x"));
      Serial.println(functionId, HEX);

      // check optional data
      uint8_t* respOptData = nullptr;
      uint8_t respOptDataLen = FCP_Get_OptData_Length(callsign, respFrame, respLen);
      Serial.print(F("Optional data ("));
      Serial.print(respOptDataLen);
      Serial.println(F(" bytes):"));
      if(respOptDataLen > 0) {
        // read optional data
        respOptData = new uint8_t[respOptDataLen];
        FCP_Get_OptData(callsign, respFrame, respLen, respOptData);
        PRINT_BUFF(respFrame, respLen);
      }

      // process received frame
      switch(functionId) {
        case RESP_PONG:
          Serial.println(F("Pong!"));
          break;

        case RESP_SYSTEM_INFO:
          Serial.println(F("System info:"));

          Serial.print(F("batteryChargingVoltage = "));
          Serial.println(FCP_Get_Battery_Charging_Voltage(respOptData));

          Serial.print(F("batteryChargingCurrent = "));
          Serial.println(FCP_Get_Battery_Charging_Current(respOptData), 4);

          Serial.print(F("batteryVoltage = "));
          Serial.println(FCP_Get_Battery_Voltage(respOptData));

          Serial.print(F("solarCellAVoltage = "));
          Serial.println(FCP_Get_Solar_Cell_Voltage(0, respOptData));

          Serial.print(F("solarCellBVoltage = "));
          Serial.println(FCP_Get_Solar_Cell_Voltage(1, respOptData));

          Serial.print(F("solarCellCVoltage = "));
          Serial.println(FCP_Get_Solar_Cell_Voltage(2, respOptData));

          Serial.print(F("batteryTemperature = "));
          Serial.println(FCP_Get_Battery_Temperature(respOptData));

          Serial.print(F("boardTemperature = "));
          Serial.println(FCP_Get_Board_Temperature(respOptData));

          Serial.print(F("mcuTemperature = "));
          Serial.println(FCP_Get_MCU_Temperature(respOptData));

          Serial.print(F("resetCounter = "));
          Serial.println(FCP_Get_Reset_Counter(respOptData));

          Serial.print(F("powerConfig = 0b"));
          Serial.println(FCP_Get_Power_Configuration(respOptData), BIN);
          break;

        case RESP_LAST_PACKET_INFO:
          Serial.println(F("Last packet info:"));

          Serial.print(F("SNR = "));
          Serial.print(respOptData[0] / 4.0);
          Serial.println(F(" dB"));

          Serial.print(F("RSSI = "));
          Serial.print(respOptData[1] / -2.0);
          Serial.println(F(" dBm"));
          break;

        case RESP_REPEATED_MESSAGE:
          Serial.println(F("Got repeated message:"));
          for(uint8_t i = 0; i < respOptDataLen; i++) {
            Serial.write(respOptData[i]);
          }
          Serial.println();
          break;

        default:
          Serial.println(F("Unknown function ID!"));
          break;
      }

      printControls();
      if(respOptDataLen > 0) {
        delete[] respOptData;
      }

    } else {
      Serial.println(F("Reception failed, code "));
      Serial.println(state);

    }

    // enable reception interrupt
    delete[] respFrame;
    radio.startReceive();
    interruptEnabled = true;

    //display.clear();
    //display.setFont(ArialMT_Plain_10);
    
    

    
  }

  if(millis() - last_update > 1111){
    updateStatus();
    last_update = millis();
  }
  
}

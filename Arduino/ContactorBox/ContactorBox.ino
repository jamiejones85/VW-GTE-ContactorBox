#include <ACAN_ESP32.h>
#include <TaskScheduler.h>

Scheduler ts;
void ms10Callback();
void ms1000Callback();

Task ms10Task(10, TASK_FOREVER, &ms10Callback);
Task ms1000Task(1000, TASK_FOREVER, &ms1000Callback);

byte vag_cnt0ba = 0x0;

bool preCharge = false;
bool mainPositiveContactor = false;
bool mainNegativeContactor = false;

//last byte doesn't seem important, can swap to just a count
//unsigned char messages [16][8] = {
//  {0x00, 0x06, 0x28, 0x00, 0x00, 0x00, 0x00, 0x34}, 
//  {0x00, 0x07, 0x28, 0x00, 0x00, 0x00, 0x00, 0x25}, 
//  {0x00, 0x08, 0x28, 0x00, 0x00, 0x00, 0x00, 0x26},
//  {0x00, 0x09, 0x28, 0x00, 0x00, 0x00, 0x00, 0x30},
//  {0x00, 0x0a, 0x28, 0x00, 0x00, 0x00, 0x00, 0xa7},
//  {0x00, 0x0b, 0x28, 0x00, 0x00, 0x00, 0x00, 0xa8},
//  {0x00, 0x0c, 0x28, 0x00, 0x00, 0x00, 0x00, 0xb9},
//  {0x00, 0x0d, 0x28, 0x00, 0x00, 0x00, 0x00, 0xAA},
//  {0x00, 0x0e, 0x28, 0x00, 0x00, 0x00, 0x00, 0xAB},
//  {0x00, 0x0f, 0x28, 0x00, 0x00, 0x00, 0x00, 0xB0},
//  {0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x2c},
//  {0x00, 0x01, 0x28, 0x00, 0x00, 0x00, 0x00, 0xb4},
//  {0x00, 0x02, 0x28, 0x00, 0x00, 0x00, 0x00, 0x2d},
//  {0x00, 0x03, 0x28, 0x00, 0x00, 0x00, 0x00, 0x3e},
//  {0x00, 0x04, 0x28, 0x00, 0x00, 0x00, 0x00, 0x2f},
//  {0x00, 0x05, 0x28, 0x00, 0x00, 0x00, 0x00, 0x21},
//};

void printMenu() {
  Serial.println("p - toggle the precharge relay");
  Serial.println("n - toggle the negative contactor");
  Serial.println("m - toggle the positive contactor");

}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(4000);

  Serial.println("ESP32-VW Contactor Box control");

  ACAN_ESP32_Settings canSettings(500000);
  canSettings.mRxPin = GPIO_NUM_16;
  canSettings.mTxPin = GPIO_NUM_17;
  uint16_t errorCode = ACAN_ESP32::can.begin(canSettings);
  if (errorCode > 0) {
    Serial.print ("Can0 Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }

  ts.addTask(ms10Task);
  ms10Task.enable();

  ts.addTask(ms1000Task);
  ms1000Task.enable();

  printMenu();
}

void ms1000Callback() {
  CANMessage msg;
  msg.id  = 0x1BFFDA19;
  msg.len = 2;
  msg.ext = true;
  msg.data[0] = 0x00;
  msg.data[1] = 0x00;

  ACAN_ESP32::can.tryToSend(msg);    //send on pt can
}

void ms10Callback() {
  CANMessage msg;
  msg.id  = 0x0BA;
  msg.len = 8;
  msg.ext = false;

  msg.data[0] = 0x00;
  msg.data[1] = vag_cnt0ba;
  msg.data[2] = 0x28;
  msg.data[3] = 0x00;
  msg.data[4] = 0x00;
  msg.data[5] = 0x00;
  msg.data[6] = 0x00;
  msg.data[7] = 0xA6;
//
//  msg.data[0] = messages[count][0];
//  msg.data[1] = messages[count][1];
//  msg.data[2] = messages[count][2];
//  msg.data[3] = messages[count][3];
//  msg.data[4] = messages[count][4];
//  msg.data[5] = messages[count][5];
//  msg.data[6] = messages[count][6];
//  msg.data[7] = messages[count][7];

  //contactor control
  if (mainNegativeContactor) {
      msg.data[2] = msg.data[2] | 0x1;
  }

  if (preCharge == 1) {
     msg.data[1] =  msg.data[1] | 0x10;
  }

  if (mainPositiveContactor == 1) {
     msg.data[1] =  msg.data[1] | 0x40;
  }
  
  vw_crc_calc(msg);

  ACAN_ESP32::can.tryToSend(msg);    //send on pt can

//  count++;
//  if (count > 16) {
//    count = 0;
//  }
//  
  vag_cnt0ba++;
//  vag_cnt0ba2++; 
//  
//  
  if(vag_cnt0ba>0x0F) vag_cnt0ba=0x00;
//  if(vag_cnt0ba2>0x0F) vag_cnt0ba2=0x00;

}

void loop() {
  // put your main code here, to run repeatedly:
  ts.execute();

  if (Serial.available()) {
    char inByte = Serial.read();
    switch (inByte)
    {
        case 'p':
          preCharge = !preCharge;
          break;
        case 'm':
          mainPositiveContactor = !mainPositiveContactor;
          break;
        case 'n':
          mainNegativeContactor = !mainNegativeContactor;
          break;
    }
  }
}


void vw_crc_calc(CANMessage& msg)
{
    
    const uint8_t poly = 0x2F;
    const uint8_t xor_output = 0xFF;
    // VAG Magic Bytes
    const uint8_t MB00BA[16] = { 0x6C, 0xAA, 0x01, 0xCF, 0x39, 0x38, 0xDF, 0x4F, 0x13, 0x2A, 0x73, 0x8C, 0xF1, 0x76, 0xF6, 0x70 };

    uint8_t crc = 0xFF;
    uint8_t magicByte = 0x00;
    uint8_t counter = msg.data[1] & 0x0F; // only the low byte of the couner is relevant

    switch (msg.id) 
    {

    case 0x0BA: // ??
        magicByte = MB00BA[counter];
        break;
    default: // this won't lead to correct CRC checksums
        magicByte = 0x00;
        break;
    }

    for (uint8_t i = 1; i < msg.len + 1; i++)
    {
        // We skip the empty CRC position and start at the timer
        // The last element is the VAG magic byte for address 0x187 depending on the counter value.
        if (i < msg.len)
            crc ^= msg.data[i];
        else
            crc ^= magicByte;

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ poly;
            else
                crc = (crc << 1);
        }
    }

    crc ^= xor_output;

    msg.data[0] = crc; // set the CRC checksum directly in the output bytes
}

/*
 *  imuslab 60W Power Bank Battery Information Display
 *  
 *  Author: tobychui
 *  All Right Reserved
 *  
 *  Loading page
 *  st: Status text
 *  
 *  Onscreen elements:
 *  t0: Battery Percentage
 *  t1: Ouput Voltage (5.00V)
 *  t2: Battery Current
 *  t3: OUTPUT / INPUT (OUTPUT)
 *  t4: Voltage Mode (5V Mode)
 *  t5: Power Output (10.0W)
 *  t6: Battery Voltage (14.2V)
 *  t7: label (TYPE C)
 *  t8: label (BATTERY)
 *  
 *  p2: Battery Icon
 *  Battery Icons ID
 *  2 --> 10% left
 *  3 --> 30% left
 *  4 --> 60% left
 *  5 --> 100%
 *  6 --> Charging Icon
 */ 

#include <SoftwareSerial.h>
const int MAX_READ_LENGTH = 10;
int OUTPUT_READPIN = A0;
int BATTERY_READPIN = A1;
int BATTERY_CURRENT = A4;

//Current calibration values, following y=mx+c where y is correct current and x is the analog read voltage value
float currentCalibrationMultiplier = 4.80263; //m
float currentCalibrationOffset = -2.40132; //c


SoftwareSerial HMISerial(2, 3); // RX, TX
bool debugMode = false;      //Enable debug mode
int currentPageNumber = 0;  //Current Page Number
byte incomingByte = 0;      //Incoming byte buffer
int valbuf = 0; //Analog read value buffer
float BATV, BATA, OUTV, OUTW;

// Send command to the UART HMI
void sendCommand(String cmd){
  if (debugMode){
    //Mirror output to serial
    Serial.print(cmd);
    Serial.write(0XFF);        
    Serial.write(0XFF);
    Serial.write(0XFF);
  }
  HMISerial.print(cmd); 
  HMISerial.write(0XFF);
  HMISerial.write(0XFF);
  HMISerial.write(0XFF);
  delay(100);
}

//Example usage: printFloatCommand("t6.txt=", 16.8, "V");
void printFloatCommand(String cmd, float value, String unit){
  if (debugMode){
    //Mirror output to serial
    Serial.print(cmd + "\"");
    if (value <= -10){
      Serial.print(value, 1);
    }else if (value < 0){
      Serial.print(value, 2);
    }else if (value < 10){
       Serial.print(value, 2);
    }else{
      Serial.print(value, 1);
    }
    Serial.print(unit + "\"");
    Serial.write(0XFF);        
    Serial.write(0XFF);
    Serial.write(0XFF);
  }
  HMISerial.print(cmd + "\"");
  if (value <= -10){
    HMISerial.print(value, 1);
  }else if (value < 0){
    HMISerial.print(value, 2);
  }else if (value < 10){
    HMISerial.print(value, 2);
  }else{
    HMISerial.print(value, 1);
  }
  HMISerial.print(unit + "\"");
  HMISerial.write(0XFF);
  HMISerial.write(0XFF);
  HMISerial.write(0XFF);
  delay(100);
}

float getBatteryVoltage(){
  valbuf = 0;
  for ( int i=0; i < 10; i++){
    valbuf += analogRead(BATTERY_READPIN);
    delay(50);
  }
  return float(valbuf) / 10.0 * 0.016455;
}

float getOutputVoltage(){
  valbuf = 0;
  for ( int i=0; i < 10; i++){
    valbuf += analogRead(OUTPUT_READPIN);
    delay(50);
  }
  return float(valbuf) / 10.0 * 0.019537;
}


float getBatteryCurrent(){
  valbuf = analogRead(BATTERY_CURRENT);
  int current = (valbuf / 1024.0 * 500) - 250;
  return (float(current) / 100) * currentCalibrationMultiplier + currentCalibrationOffset; 
}

float getTotalOutputPower(float batV, float batA){
  return batV * batA;
}

int estimateBatteryPercentage(float batV){
  valbuf = 0.0;
  //Cap the max voltage for rendering
  if (batV > 16.8){
    batV = 16.8;
  }else if (batV < 13.09){
    batV = 13.09;
  }

  int batmV = int(batV * 100);

  //Reconstruct the battery capacity curve
  if (batmV > 1660){
    //Near 100% parts
    valbuf = 100;
  }else if (batmV > 1550){
    //60 - 100
    valbuf = map(batmV, 1550, 1660, 60, 100);
  }else if (batmV > 1518){
    //40 - 60
    valbuf = map(batmV, 1518, 1550, 40, 60);
  }else if (batmV > 1483){
    //10 - 40
    valbuf = map(batmV, 1483, 1518, 10, 40);
  }else{
    //Under 10%
    valbuf = map(batmV, 1309, 1483, 1, 10);
  }
  
  return int(valbuf);
}

//Change the battery icon according to charge status
void updateBatteryIcon(int batPercentage, bool isCharging){
  if (isCharging){
    //Charging
    sendCommand("p2.pic=6");
  }else{
    //Discharging
    if (batPercentage >= 80){
      sendCommand("p2.pic=5");
    }else if (batPercentage >= 60){
        sendCommand("p2.pic=4");
    }else if (batPercentage >= 30){
      sendCommand("p2.pic=3");
    }else{
      sendCommand("p2.pic=2");
    }
  }
}


int getPageNumber(){
  //Request the UART HMI to return its current page number
  sendCommand("sendme");

  //Try to parse and catch the return value
  bool nextReadIsPageNumber = false;
  if (debugMode){
    //Debug mode
    while (Serial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
      if (nextReadIsPageNumber){
        //This read is the page number
        nextReadIsPageNumber = false;
        return incomingByte;
      }

      if (incomingByte == 0x66){
        //This is the start of a page response
        nextReadIsPageNumber = true;
      }
    }
  }else{
    while (HMISerial.available() > 0) {
      // read the incoming byte:
      incomingByte = HMISerial.read();
      if (nextReadIsPageNumber){
        //This read is the page number
        nextReadIsPageNumber = false;
        return incomingByte;
      }

      if (incomingByte == 0x66){
        //This is the start of a page response
        nextReadIsPageNumber = true;
      }
    }
  }

  return -1;
}

// initization of display if the screen is resetted
void initDisplayIfPage0(){
  //Check if the current page is page 0
  currentPageNumber = getPageNumber();
  if (currentPageNumber == 0x00){
    //Page 0, run init check
    digitalWrite(LED_BUILTIN, HIGH);
    
    sendCommand("st.txt=\"MCU Connected (OK)\"");
    delay(500);
    sendCommand("st.txt=\"System Check (OK)\"");
    //Do checking here
    delay(200);
    printFloatCommand("st.txt=",getBatteryCurrent()," ACS712 (OK)");  
    delay(200);
    valbuf = getBatteryVoltage();
    if (valbuf > 10){
      printFloatCommand("st.txt=",valbuf," BATV (OK)");  
      delay(200);
      printFloatCommand("st.txt=",float(estimateBatteryPercentage(valbuf))," BAT% (OK)");
    }else{
      printFloatCommand("st.txt=",valbuf," BATV (FAILED)");
      delay(5000);
    }
    delay(200);
    printFloatCommand("st.txt=",getOutputVoltage()," OUTV (OK)");
    //Finally, switch to info page
    delay(1000);
    sendCommand("page 1");
  }else{
    //Other pages
    digitalWrite(LED_BUILTIN, LOW);
  }
}

//Main logic for getting and updating the battery logic
void updateBatteryInformation(){
  //Get basic information
  OUTV = getOutputVoltage();
  BATV = getBatteryVoltage();
  BATA = getBatteryCurrent();
  OUTW = getTotalOutputPower(BATV, BATA);
  printFloatCommand("t1.txt=", OUTV, "V");
  printFloatCommand("t6.txt=", BATV, "V");
  printFloatCommand("t5.txt=", OUTW, "W");
  printFloatCommand("t2.txt=", BATA, "A");

  //Get Battery Capacity
  sendCommand("t0.txt=\"" + String(estimateBatteryPercentage(BATV))+ "%\"");
  
  //Estimate the output mode
  if (OUTV < 7.0){
    //5V Mode
    sendCommand("t4.txt=\"5V Mode\"");
  }else if (OUTV < 10.0){
    //9V Mode
    sendCommand("t4.txt=\"9V Mode\"");
  }else if (OUTV < 13.0){
    //12V Mode
    sendCommand("t4.txt=\"12V Mode\"");
  }else if (OUTV < 17.0){
    //15V Mode
    sendCommand("t4.txt=\"15V Mode\"");
  }else{
    //20V Mode
    sendCommand("t4.txt=\"20V Mode\"");
  }

  if (BATA < -0.1){
    sendCommand("t3.txt=\"INPUT\"");
    sendCommand("t3.pco=61440");
    updateBatteryIcon(estimateBatteryPercentage(BATV), true);
  }else{
    sendCommand("t3.txt=\"OUTPUT\"");
    sendCommand("t3.pco=11928");
    updateBatteryIcon(estimateBatteryPercentage(BATV), false);
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  //Setup the serial and software serial
  Serial.begin(115200);
  HMISerial.begin(9600);

  //Setup debug LED
  pinMode(LED_BUILTIN, OUTPUT);

  /*
  pinMode(A3, OUTPUT);
  pinMode(A5, OUTPUT);
  digitalWrite(A3, HIGH);
  digitalWrite(A5, LOW);
  */
  }

// the loop function runs over and over again forever
void loop() {
  initDisplayIfPage0();
  updateBatteryInformation();
}

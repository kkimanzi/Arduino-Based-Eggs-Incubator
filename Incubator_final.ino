#include <Adafruit_Sensor.h>.

#include <DHT.h>
#include <DHT_U.h>


#include <SoftwareSerial.h>
#include <String.h>

#include <LedControl.h>
#include <avr/pgmspace.h>

/*****FOR DHT*******/
#define DHTPIN 3       // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)                                                                                  
#define RELAY 2       // relay output pin0
float refTemp = 27.0;   //default refTemp
float temp; //Stores temperature value  qzc
bool relayFlag;       /*** CHECK IF REALLY USED***/


int hcount = 0;
int lcount = 0;

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

/**** FOR GSM******/
#define SIM800_RX_PIN 7     //Pin on which GSM module rx is connected to arduino board
#define SIM800_TX_PIN 8     //Pin on which GSM module tx is connected to arduino board
String notification;
bool notificationPresent = false;

SoftwareSerial serialSIM800(SIM800_TX_PIN,SIM800_RX_PIN);

/****FOR LED*******/
typedef const unsigned char prog_uchar;

const int numDevices = 2;      // number of MAX7219s used
const long scrollDelay = 75;   // adjust scrolling speed
unsigned long bufferLong [14] = {0}; 


prog_uchar scrollCurrent[] PROGMEM ={"  Curr Temp  :  "};
prog_uchar scrollReference[] PROGMEM ={"  Ref Temp  :  "};
char ucharTemp[6];

LedControl lc=LedControl(11,13,10,numDevices);



/******   SETUP   ******/

void setup()
{
  /***** FOR DHT  ******/
  pinMode(RELAY, OUTPUT);  
  Serial.begin(9600);
  dht.begin();
  Serial.println("Powering on incubator. Bulb off");

  /******  FOR GSM  ******/
  Serial.begin(9600);
  while(!Serial);
  //Being serial communication with Arduino and SIM800
  serialSIM800.begin(9600);
  delay(1000);
  Serial.println("Setup Complete!");

  delay(500);
  serialSIM800.println("AT+CMGF=1");  //operate in SMS mode

  delay(500);
  waitForResponse();

  /****** FOR LED  ******/
  for (int x=0; x<numDevices; x++){
        lc.shutdown(x,false);       //The MAX72XX is in power-saving mode on startup
        lc.setIntensity(x,8);       // Set the brightness to default value
        lc.clearDisplay(x);         // and clear the display
  } 
}


void setToTextMode(){
  // this function sets the GSM module to operate in SMS mode
  delay(500);
  serialSIM800.println("AT+CMGF=1");  //operate in SMS mode
}

void setRecipientNumber(){
  delay(500);
  serialSIM800.println("AT+CMGS=\"+254791880xxx\"\r");
}

void endTransmission(){
  //terminate a stream input into GSM module
  serialSIM800.write(26);
  serialSIM800.println();
  serialSIM800.flush();
}

void sendGSMMessage(String message){
   // this function sends the String message to recipient number specified
   setToTextMode();
   setRecipientNumber();;
   delay(500);
   serialSIM800.print(message);
   delay(500);
   endTransmission();
}

void sendHighTemperatureAlert(float temp){
   Serial.println("Sending abnormal high temp alert");
   String message = "Temperature abnormally high\t: "+ String(temp);
   //call function to send message
   sendGSMMessage(message);
}

void sendLowTemperatureAlert(float temp){
   Serial.println("Sending abnormal low temp alert");
   String message = "Temperature abnormally low\t: "+ String(temp);
   //call function to send message
   sendGSMMessage(message);
}

/******  LOOP  *******/
void loop(){
  temperatureMonitor();   //float temp set, necesarry adjustments to relay made

  //the following two conditional blocks test if the current temperature is abnormally extreme as in the case of a component malfunction
  if(temp > (refTemp + 5)){              //
    lcount = 0;
    if (hcount == 0){
      sendHighTemperatureAlert(temp); 
    }
    hcount++;
    if (hcount >= 100){
      hcount = 0;
    }
  }                                           //INCASE OF ABNORMAL TEMP (BEYOND NORMAL RANGE)
  else if(temp < (refTemp - 5)){              //
    hcount = 0;
    if (lcount == 0){
      sendLowTemperatureAlert(temp); 
    }
    lcount++;
    if (lcount >= 100){
      lcount = 0;
    }
  }
  
  Serial.println("Going to scroll text");
  scrollMessage(scrollCurrent);
  dtostrf(temp,5,1,ucharTemp);    //converts float variable into a character array to be printed on LED
  int counter = 1;  //first numerical character is at this index
    int myChar=0;
    do {
        // read back a char 
            loadBufferLong(ucharTemp[counter]);
        counter++;
    } 
    while (counter != 6);
    
  delay(2000);
  scrollMessage(scrollReference);
  dtostrf(refTemp,5,1,ucharTemp);
  
  counter = 1;
  myChar = 0;
   do {
        // read back a char 
            loadBufferLong(ucharTemp[counter]);
        counter++;
    } 
    while (counter != 6);
    
  delay(2000);
  
  Serial.println("Going to GSM handler");
  GSMHandler();   
  delay(2000); 
}


void temperatureMonitor(){
    //this funnctions checks current temperature and responds accordingly
    temp= dht.readTemperature();    //read current temperature
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");
    
    if (temp > (refTemp + 3.0) ){
      Serial.println("Turning off bulb");
      turnBulbOff();
    }
    else if (temp < (refTemp - 3.0)){
      Serial.println("Turning on bulb");
      turnBulbOn();
    } 
}

void turnBulbOff(){
    digitalWrite(RELAY, LOW);
}

void turnBulbOn(){
  digitalWrite(RELAY, HIGH);
}

/******  FUNCTIONS FOR LED DISPLAY  ******/
prog_uchar font5x7 [] PROGMEM = {      //Numeric Font Matrix (Arranged as 7x font data + 1x kerning data) ; STORED IN PROGRAM MEMORY
    B00000000,  //Space (Char 0x20)
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    6,

    B10000000,  //!
    B10000000,
    B10000000,
    B10000000,
    B00000000,
    B00000000,
    B10000000,
    2,

    B10100000,  //"
    B10100000,
    B10100000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    4,

    B01010000,  //#
    B01010000,
    B11111000,
    B01010000,
    B11111000,
    B01010000,
    B01010000,
    6,

    B00100000,  //$
    B01111000,
    B10100000,
    B01110000,
    B00101000,
    B11110000,
    B00100000,
    6,

    B11000000,  //%
    B11001000,
    B00010000,
    B00100000,
    B01000000,
    B10011000,
    B00011000,
    6,

    B01100000,  //&
    B10010000,
    B10100000,
    B01000000,
    B10101000,
    B10010000,
    B01101000,
    6,

    B11000000,  //'
    B01000000,
    B10000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    3,

    B00100000,  //(
    B01000000,
    B10000000,
    B10000000,
    B10000000,
    B01000000,
    B00100000,
    4,

    B10000000,  //)
    B01000000,
    B00100000,
    B00100000,
    B00100000,
    B01000000,
    B10000000,
    4,

    B00000000,  //*
    B00100000,
    B10101000,
    B01110000,
    B10101000,
    B00100000,
    B00000000,
    6,

    B00000000,  //+
    B00100000,
    B00100000,
    B11111000,
    B00100000,
    B00100000,
    B00000000,
    6,

    B00000000,  //,
    B00000000,
    B00000000,
    B00000000,
    B11000000,
    B01000000,
    B10000000,
    3,

    B00000000,  //-
    B00000000,
    B11111000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    6,

    B00000000,  //.
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B11000000,
    B11000000,
    3,

    B00000000,  ///
    B00001000,
    B00010000,
    B00100000,
    B01000000,
    B10000000,
    B00000000,
    6,

    B01110000,  //0
    B10001000,
    B10011000,
    B10101000,
    B11001000,
    B10001000,
    B01110000,
    6,

    B01000000,  //1
    B11000000,
    B01000000,
    B01000000,
    B01000000,
    B01000000,
    B11100000,
    4,

    B01110000,  //2
    B10001000,
    B00001000,
    B00010000,
    B00100000,
    B01000000,
    B11111000,
    6,

    B11111000,  //3
    B00010000,
    B00100000,
    B00010000,
    B00001000,
    B10001000,
    B01110000,
    6,

    B00010000,  //4
    B00110000,
    B01010000,
    B10010000,
    B11111000,
    B00010000,
    B00010000,
    6,

    B11111000,  //5
    B10000000,
    B11110000,
    B00001000,
    B00001000,
    B10001000,
    B01110000,
    6,

    B00110000,  //6
    B01000000,
    B10000000,
    B11110000,
    B10001000,
    B10001000,
    B01110000,
    6,

    B11111000,  //7
    B10001000,
    B00001000,
    B00010000,
    B00100000,
    B00100000,
    B00100000,
    6,

    B01110000,  //8
    B10001000,
    B10001000,
    B01110000,
    B10001000,
    B10001000,
    B01110000,
    6,

    B01110000,  //9
    B10001000,
    B10001000,
    B01111000,
    B00001000,
    B00010000,
    B01100000,
    6,

    B00000000,  //:
    B11000000,
    B11000000,
    B00000000,
    B11000000,
    B11000000,
    B00000000,
    3,

    B00000000,  //;
    B11000000,
    B11000000,
    B00000000,
    B11000000,
    B01000000,
    B10000000,
    3,

    B00010000,  //<
    B00100000,
    B01000000,
    B10000000,
    B01000000,
    B00100000,
    B00010000,
    5,

    B00000000,  //=
    B00000000,
    B11111000,
    B00000000,
    B11111000,
    B00000000,
    B00000000,
    6,

    B10000000,  //>
    B01000000,
    B00100000,
    B00010000,
    B00100000,
    B01000000,
    B10000000,
    5,

    B01110000,  //?
    B10001000,
    B00001000,
    B00010000,
    B00100000,
    B00000000,
    B00100000,
    6,

    B01110000,  //@
    B10001000,
    B00001000,
    B01101000,
    B10101000,
    B10101000,
    B01110000,
    6,

    B01110000,  //A
    B10001000,
    B10001000,
    B10001000,
    B11111000,
    B10001000,
    B10001000,
    6,

    B11110000,  //B
    B10001000,
    B10001000,
    B11110000,
    B10001000,
    B10001000,
    B11110000,
    6,

    B01110000,  //C
    B10001000,
    B10000000,
    B10000000,
    B10000000,
    B10001000,
    B01110000,
    6,

    B11100000,  //D
    B10010000,
    B10001000,
    B10001000,
    B10001000,
    B10010000,
    B11100000,
    6,

    B11111000,  //E
    B10000000,
    B10000000,
    B11110000,
    B10000000,
    B10000000,
    B11111000,
    6,

    B11111000,  //F
    B10000000,
    B10000000,
    B11110000,
    B10000000,
    B10000000,
    B10000000,
    6,

    B01110000,  //G
    B10001000,
    B10000000,
    B10111000,
    B10001000,
    B10001000,
    B01111000,
    6,

    B10001000,  //H
    B10001000,
    B10001000,
    B11111000,
    B10001000,
    B10001000,
    B10001000,
    6,

    B11100000,  //I
    B01000000,
    B01000000,
    B01000000,
    B01000000,
    B01000000,
    B11100000,
    4,

    B00111000,  //J
    B00010000,
    B00010000,
    B00010000,
    B00010000,
    B10010000,
    B01100000,
    6,

    B10001000,  //K
    B10010000,
    B10100000,
    B11000000,
    B10100000,
    B10010000,
    B10001000,
    6,

    B10000000,  //L
    B10000000,
    B10000000,
    B10000000,
    B10000000,
    B10000000,
    B11111000,
    6,

    B10001000,  //M
    B11011000,
    B10101000,
    B10101000,
    B10001000,
    B10001000,
    B10001000,
    6,

    B10001000,  //N
    B10001000,
    B11001000,
    B10101000,
    B10011000,
    B10001000,
    B10001000,
    6,

    B01110000,  //O
    B10001000,
    B10001000,
    B10001000,
    B10001000,
    B10001000,
    B01110000,
    6,

    B11110000,  //P
    B10001000,
    B10001000,
    B11110000,
    B10000000,
    B10000000,
    B10000000,
    6,

    B01110000,  //Q
    B10001000,
    B10001000,
    B10001000,
    B10101000,
    B10010000,
    B01101000,
    6,

    B11110000,  //R
    B10001000,
    B10001000,
    B11110000,
    B10100000,
    B10010000,
    B10001000,
    6,

    B01111000,  //S
    B10000000,
    B10000000,
    B01110000,
    B00001000,
    B00001000,
    B11110000,
    6,

    B11111000,  //T
    B00100000,
    B00100000,
    B00100000,
    B00100000,
    B00100000,
    B00100000,
    6,

    B10001000,  //U
    B10001000,
    B10001000,
    B10001000,
    B10001000,
    B10001000,
    B01110000,
    6,

    B10001000,  //V
    B10001000,
    B10001000,
    B10001000,
    B10001000,
    B01010000,
    B00100000,
    6,

    B10001000,  //W
    B10001000,
    B10001000,
    B10101000,
    B10101000,
    B10101000,
    B01010000,
    6,

    B10001000,  //X
    B10001000,
    B01010000,
    B00100000,
    B01010000,
    B10001000,
    B10001000,
    6,

    B10001000,  //Y
    B10001000,
    B10001000,
    B01010000,
    B00100000,
    B00100000,
    B00100000,
    6,

    B11111000,  //Z
    B00001000,
    B00010000,
    B00100000,
    B01000000,
    B10000000,
    B11111000,
    6,

    B11100000,  //[
    B10000000,
    B10000000,
    B10000000,
    B10000000,
    B10000000,
    B11100000,
    4,

    B00000000,  //(Backward Slash)
    B10000000,
    B01000000,
    B00100000,
    B00010000,
    B00001000,
    B00000000,
    6,

    B11100000,  //]
    B00100000,
    B00100000,
    B00100000,
    B00100000,
    B00100000,
    B11100000,
    4,

    B00100000,  //^
    B01010000,
    B10001000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    6,

    B00000000,  //_
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B11111000,
    6,

    B10000000,  //`
    B01000000,
    B00100000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    4,

    B00000000,  //a
    B00000000,
    B01110000,
    B00001000,
    B01111000,
    B10001000,
    B01111000,
    6,

    B10000000,  //b
    B10000000,
    B10110000,
    B11001000,
    B10001000,
    B10001000,
    B11110000,
    6,

    B00000000,  //c
    B00000000,
    B01110000,
    B10001000,
    B10000000,
    B10001000,
    B01110000,
    6,

    B00001000,  //d
    B00001000,
    B01101000,
    B10011000,
    B10001000,
    B10001000,
    B01111000,
    6,

    B00000000,  //e
    B00000000,
    B01110000,
    B10001000,
    B11111000,
    B10000000,
    B01110000,
    6,

    B00110000,  //f
    B01001000,
    B01000000,
    B11100000,
    B01000000,
    B01000000,
    B01000000,
    6,

    B00000000,  //g
    B01111000,
    B10001000,
    B10001000,
    B01111000,
    B00001000,
    B01110000,
    6,

    B10000000,  //h
    B10000000,
    B10110000,
    B11001000,
    B10001000,
    B10001000,
    B10001000,
    6,

    B01000000,  //i
    B00000000,
    B11000000,
    B01000000,
    B01000000,
    B01000000,
    B11100000,
    4,

    B00010000,  //j
    B00000000,
    B00110000,
    B00010000,
    B00010000,
    B10010000,
    B01100000,
    5,

    B10000000,  //k
    B10000000,
    B10010000,
    B10100000,
    B11000000,
    B10100000,
    B10010000,
    5,

    B11000000,  //l
    B01000000,
    B01000000,
    B01000000,
    B01000000,
    B01000000,
    B11100000,
    4,

    B00000000,  //m
    B00000000,
    B11010000,
    B10101000,
    B10101000,
    B10001000,
    B10001000,
    6,

    B00000000,  //n
    B00000000,
    B10110000,
    B11001000,
    B10001000,
    B10001000,
    B10001000,
    6,

    B00000000,  //o
    B00000000,
    B01110000,
    B10001000,
    B10001000,
    B10001000,
    B01110000,
    6,

    B00000000,  //p
    B00000000,
    B11110000,
    B10001000,
    B11110000,
    B10000000,
    B10000000,
    6,

    B00000000,  //q
    B00000000,
    B01101000,
    B10011000,
    B01111000,
    B00001000,
    B00001000,
    6,

    B00000000,  //r
    B00000000,
    B10110000,
    B11001000,
    B10000000,
    B10000000,
    B10000000,
    6,

    B00000000,  //s
    B00000000,
    B01110000,
    B10000000,
    B01110000,
    B00001000,
    B11110000,
    6,

    B01000000,  //t
    B01000000,
    B11100000,
    B01000000,
    B01000000,
    B01001000,
    B00110000,
    6,

    B00000000,  //u
    B00000000,
    B10001000,
    B10001000,
    B10001000,
    B10011000,
    B01101000,
    6,

    B00000000,  //v
    B00000000,
    B10001000,
    B10001000,
    B10001000,
    B01010000,
    B00100000,
    6,

    B00000000,  //w
    B00000000,
    B10001000,
    B10101000,
    B10101000,
    B10101000,
    B01010000,
    6,

    B00000000,  //x
    B00000000,
    B10001000,
    B01010000,
    B00100000,
    B01010000,
    B10001000,
    6,

    B00000000,  //y
    B00000000,
    B10001000,
    B10001000,
    B01111000,
    B00001000,
    B01110000,
    6,

    B00000000,  //z
    B00000000,
    B11111000,
    B00010000,
    B00100000,
    B01000000,
    B11111000,
    6,

    B00100000,  //{
    B01000000,
    B01000000,
    B10000000,
    B01000000,
    B01000000,
    B00100000,
    4,

    B10000000,  //|
    B10000000,
    B10000000,
    B10000000,
    B10000000,
    B10000000,
    B10000000,
    2,

    B10000000,  //}
    B01000000,
    B01000000,
    B00100000,
    B01000000,
    B01000000,
    B10000000,
    4,

    B00000000,  //~
    B00000000,
    B00000000,
    B01101000,
    B10010000,
    B00000000,
    B00000000,
    6,

    B01100000,  // (Char 0x7F)
    B10010000,
    B10010000,
    B01100000,
    B00000000,
    B00000000,
    B00000000,
    5
};

void scrollFont() {
    for (int counter=0x20;counter<0x80;counter++){
        loadBufferLong(counter);
        delay(500);
    }
}

// Scroll Message
void scrollMessage(prog_uchar * messageString) {
    int counter = 0;
    int myChar=0;
    do {
        // read back a char 
        myChar =  pgm_read_byte_near(messageString + counter); 
        if (myChar != 0){
            loadBufferLong(myChar);
        }
        counter++;
    } 
    while (myChar != 0);
}
// Load character into scroll buffer
void loadBufferLong(int ascii){
    if (ascii >= 0x20 && ascii <=0x7f){
        for (int a=0;a<7;a++){                      // Loop 7 times for a 5x7 font
            unsigned long c = pgm_read_byte_near(font5x7 + ((ascii - 0x20) * 8) + a);     // Index into character table to get row data
            unsigned long x = bufferLong [a*2];     // Load current scroll buffer
            x = x | c;                              // OR the new character onto end of current
            bufferLong [a*2] = x;                   // Store in buffer
        }
        byte count = pgm_read_byte_near(font5x7 +((ascii - 0x20) * 8) + 7);     // Index into character table for kerning data
        for (byte x=0; x<count;x++){
            rotateBufferLong();
            printBufferLong();
            delay(scrollDelay);
        }
    }
}
// Rotate the buffer
void rotateBufferLong(){
    for (int a=0;a<7;a++){                      // Loop 7 times for a 5x7 font
        unsigned long x = bufferLong [a*2];     // Get low buffer entry
        byte b = bitRead(x,31);                 // Copy high order bit that gets lost in rotation
        x = x<<1;                               // Rotate left one bit
        bufferLong [a*2] = x;                   // Store new low buffer
        x = bufferLong [a*2+1];                 // Get high buffer entry
        x = x<<1;                               // Rotate left one bit
        bitWrite(x,0,b);                        // Store saved bit
        bufferLong [a*2+1] = x;                 // Store new high buffer
    }
}  
// Display Buffer on LED matrix
void printBufferLong(){
  for (int a=0;a<7;a++){                    // Loop 7 times for a 5x7 font
    unsigned long x = bufferLong [a*2+1];   // Get high buffer entry
    byte y = x;                             // Mask off first character
    lc.setRow(3,a,y);                       // Send row to relevent MAX7219 chip
    x = bufferLong [a*2];                   // Get low buffer entry
    y = (x>>24);                            // Mask off second character
    lc.setRow(2,a,y);                       // Send row to relevent MAX7219 chip
    y = (x>>16);                            // Mask off third character
    lc.setRow(1,a,y);                       // Send row to relevent MAX7219 chip
    y = (x>>8);                             // Mask off forth character
    lc.setRow(0,a,y);                       // Send row to relevent MAX7219 chip
  }
}
/******  END LED DISPLAY FUNCTIONS  ******/

/******  GSM FUNCTIONS  ******/
String waitForResponse(){
  //waits, then checks if the GSM device has received a new notification
  Serial.println("Entering waitForResponse");
  delay(6000);
   if(serialSIM800.available()){    //serailSIM800 only available if it has it's output buffer populated
      Serial.println("Available");
       notification = serialSIM800.readString();
  }
  else{
      //notification = "None";
      notification = "none";
  }
  Serial.print("Leaving waitForResponse. notificationPresent :  ");
  Serial.println((String)notificationPresent);
  
  return notification;
  // NOTE: notification returned could be of any AT-Code, we have to check it further for more info
}

String waitForResponse2(){
  Serial.println("Waiting for new notification");
  while(true){    //waits indefinately till message to be read has been received. 
     if(serialSIM800.available()){
      Serial.println("Available");
      String notification = "";
      notification = serialSIM800.readString();
      //notification = serialSIM800.readString();
      //Serial.println(notification);
      //return message alert text
      return notification;
     }
  }
}

bool isNewMessage(String notification){
  //check if new alert is for a new message
  // AT-Code for new message notification is +CMTI
  Serial.println("Testing if new message");
  //Serial.print(String(strncmp(content.c_str(), "+CMTI",4)));
    char headerCode[] = {'+','C','M','T','I'};
    //removeSpaces(content.c_str());
    bool mismatch = false;
    int comparison;

    //strangely String notification has char ' ' (i.e blank space) at locations [0] and [1].
    //well then, our counter begins at [2]
    for(int a = 2 ; a < 7; a++){
      comparison = ((String)((char)notification[a]) ==  (String)((char)headerCode[a-2]));
      if(comparison != 1){
        mismatch = true;
        break;
      }
    }

    return (!mismatch);
}

int getNewMessageIndex(String notification){
      //Parse new alert notification to get index at which the incoming message that triggered the alert is stored
      //i.e obtains the location at which new message is stored
      Serial.print("Getting index : ");
      //Serial.println(notification.c_str());
      const char s[2] = ",";
      char *token;

      token = strtok(notification.c_str(), s);
      token = strtok(NULL, s);  //this token represents index

      Serial.println(token);
      //readMessageAtIndex(atoi(token));
      return atoi(token);
}


String readMessageAtIndex(int index){
  //Read message stored in memory at index
  serialSIM800.print("AT+CMGR=");
  serialSIM800.println((String)index);
    endTransmission();
    String response = waitForResponse2();
    delay(10);  //prevents race condition to 64byte buffer
    
    return response;
}

String getMessageHeader(String message){
    //Tokenize at '\n'
      //Serial.println(notification.c_str());
      const char s[2] = {'\n','\0'};
      //char *messageBody;
      char *messageHeader;
      
      messageHeader = strtok(message.c_str(), s);
      //Serial.println(messageBody);
      messageHeader = strtok(NULL, s);  //this token represents contains header info
      //Serial.println(messageHeader);
      return messageHeader;
}

String getMessageBody(String message){
  //Tokenize at '\n'
      //Serial.println(notification.c_str());
      Serial.println("Tyrnna access message body");
      const char s[2] = {'\n','\0'};    //tokens generated at new line delimeter
      char *messageBody;
      //char *messageHeader;
      
      messageBody = strtok(message.c_str(), s);
      //Serial.println(messageBody);
      messageBody = strtok(NULL, s);  //this token represents contains header info(
      //Serial.println(messageBody);
      messageBody = strtok(NULL, s);    //only this instance of messageBody is needed
      return messageBody;
}

void sendCurrentTemperature(float temp){
    //this function gets the current temperature value and sends it to the recipient
   Serial.println("Sending current temperature alert");
   String message = "Current temperature : "+ String(temp)+" Celcius";
   Serial.print(message);
   //call function to send message
   sendGSMMessage(message);
}

void GSMHandler(){
    /****  ----This is  the key GSM coordinator function----  ****/
    notification = "";
    Serial.println("Enteering GSMHandler");
    notification = waitForResponse();   //wait for a while then check if module has received a new notification
      if(isNewMessage(notification)){   //test if notification returned is a new message
        int index = getNewMessageIndex(notification);   //if ture, get index at which message is stored
        Serial.println("Reading new message at Index "+(String)index); 
        String rawMessage = readMessageAtIndex(index);    //get whole message at index
        
        //NOTE: rawMessage comprised of both header info(timestamp, sender....) and body(actual message contents)
        Serial.println("Raw message : " + rawMessage);
        Serial.println("Header\t : " + getMessageHeader(rawMessage));
        
        //**** TO DO -- Parse message header (via tokens) to obtain sending number
        
        String msgBody = getMessageBody(rawMessage);
        msgBody.trim();
        Serial.println("Body\t :" + msgBody);
        //Use a switch statement to act on the message 
        if(msgBody == "Turn off bulb"){
          turnBulbOff();
        }
        if (msgBody == "Turn bulb on"){
           turnBulbOn();
        }
        if (msgBody == "Current temperature?"){
          char charTemp[6];
          dtostrf(temp,5,1,charTemp);   //convert current temperaturevalue of float data type to character
          sendCurrentTemperature(temp);    
      }


        /** CHECK IF MESSAGE BODY IS FOR SETTING NEW REFERENCE TEMPERATURE **/
        char setRefCode[] = {'R','e','f',':'};
        //removeSpaces(content.c_str());
        bool mismatch = false;
        int comparison;
    
        //strangely String notification has char ' ' (i.e blank space) at locations [0] and [1].
        //well then, our counter begins at [2]
        for(int a = 0 ; a < 4; a++){
          comparison = ((String)((char)msgBody[a]) ==  (String)((char)setRefCode[a]));
          Serial.println((String)((char)msgBody[a]) + " VS " + (String)((char)setRefCode[a]));
          if(comparison != 1){
            mismatch = true;
            break;
          }
        }

        if(!mismatch){
          Serial.println("No mismatch");
          const char s[2] = {':','\0'};
          char *newRef;
          newRef = strtok(msgBody.c_str(), s);
          Serial.println("1st token : " + (String)newRef);
          newRef = strtok(NULL, s);  //this token represents contains header info(
          Serial.println("2nd toke : " + (String)newRef);

          Serial.println("New Ref : " + (String)atof(newRef));
          refTemp = atof(newRef);   // set new reference temperature
          Serial.println("The ref is now : " + (String)refTemp);
        }
      
    }
    delay(2000);
    Serial.println("Leaving GSMHandler");         
}

/******  END GSM FUNCTIONS  ******/

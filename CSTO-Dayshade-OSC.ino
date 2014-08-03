//Import Libraries
#include "Ethernet.h"
#include "EthernetUdp.h"

#include "SPI.h"
#include "Adafruit_WS2801.h"

#include "OSCBundle.h"
#include "OSCBoards.h"

EthernetUDP Udp;
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // you can find this written on the board of some Arduino Ethernets or shields

//the Arduino's IP
IPAddress ip(10, 1, 69, 200);

//port numbers
const unsigned int inPort = 8000;

//Assign Pins
  //Ditigal
//const int spectrumStep = 4; //Unused in this version
//const int spectrumReset = 5; //Unused in this version
const uint8_t dataPin  = 6; //Light
const uint8_t clockPin = 7;

const int psOn = 8;
const int modeInterupt = 9; //Interupt Pin

  //Analog
//int spectrumLeft = 0; //Unused in this version
//int spectrumRight = 1; //Unused in this version
const int knoba = 2;
const int knobb = 3;

//Init Light Pixels
const int numPixels = 140;
Adafruit_WS2801 dayshade = Adafruit_WS2801(numPixels, dataPin, clockPin, WS2801_GRB);

//Init Global Variables
int mode = 0; //What program is running

bool override = false;

int oscA = 0;
int oscB = 0;

void setup() {
  Ethernet.begin(mac,ip);
  Udp.begin(inPort);
  
  dayshade.begin(); 
  dayshade.show(); // Update LED contents, to start they are all 'off'
  
  digitalWrite(psOn, LOW);
  
  randomSeed(analogRead(5));
}

void loop() {
   OSCBundle bundleIN;
   int size;
 
   if((size = Udp.parsePacket())>0){
     while(size--)
       bundleIN.fill(Udp.read());

      if(!bundleIN.hasError())
        bundleIN.route("/1", appRoute);
   }
  
  int optionA, optionB;
  
  if (!override) {
    optionA = analogRead(knoba);
    optionB = analogRead(knobb);
  } else {
    optionA = oscA;
    optionB = oscB;
  }
  
  switch (mode) {
    case 0:
      makeFire(optionA);
      break;
    case 1:
      sparkle(optionA,optionB); //Default 1023,512
      break;
    case 2:
      white(optionA); //Default 0
      break;
    case 3:
      fillAll(optionA,optionB); //Default 0
      break;
    case 4:
      rainbowChase(optionA);
      break;
    case 5:
      police(optionA,optionB);
      break;
    case 6:
      strobe(optionA);
      break;
  }
  if(digitalRead(modeInterupt)){changeMode();};
}

//************************************************ OSC Routes *************************************************//
void appRoute(OSCMessage & msg, int offset){
  msg.route("/xy", setOptions, offset);
  msg.route("/modes", btnPressed, offset);
}

void setOptions(OSCMessage & options, int offset) {
  if (options.isInt(0)){        
    oscA = options.getInt(0);
  }
  else if(options.isFloat(0)){
    oscA = int(options.getFloat(0));
  }
  
  if (options.isInt(1)){        
    oscB = options.getInt(1);
  }
  else if(options.isFloat(1)){
    oscB = int(options.getFloat(1));
  }
}

void btnPressed(OSCMessage & button, int offset) {
  for(int i=0; i<8; i++){
    int btnMatched = msg.match(numToOSCAddress(i), offset);
    if(btnMatched) {
      
    }
  }
}

//************************************************ Programs *************************************************//
/***** Make Fire ******\
| A faux fire effect.  |
| analoga:             |
\**********************/
void makeFire(int hue){
  if(hue>512){
    for(int i=0; i<numPixels; i++){
      dayshade.setPixelColor(i,Color(random(120)+135,random(40),0));
      int anum = random(2);
      if(anum==1){
        dayshade.setPixelColor(i,Color(0,0,0));
      }
      int anums = random(400);
      if(anums==1){
        dayshade.setPixelColor(i,Color(255,255,0));
      }
    }
  } else {
    for(int i=0; i<numPixels; i++){
      dayshade.setPixelColor(i,Color(0,random(255),random(255)));
      int anum = random(2);
      if(anum==1){
        dayshade.setPixelColor(i,Color(0,0,0));
      }
      int anums = random(400);
      if(anums==1){
        dayshade.setPixelColor(i,Color(0,255,255));
      }
    }
  }
  dayshade.show();   // write all the pixels out
}

/***************************** Sparkle ****************************\
| Makes a random pixel white at random interals that slowly plays  |
| analoga:                                                         |
\******************************************************************/
void sparkle(int likelyhood,int speedArg){
  static byte drop[numPixels];
  int theSpeed = map(speedArg,0,1023,1,20);
  
  // This range may seem random, but my favourite setting is at 10
  // and I wanted 10 to be exatly in the middle.
  if(random(map(likelyhood,0,1023,1,18))==0){
    drop[random(numPixels)]=255;
    drop[random(numPixels)]=255;
    drop[random(numPixels)]=255;
    drop[random(numPixels)]=255;
    drop[random(numPixels)]=255;
  }
  for(int i=0; i<numPixels; i++){
    dayshade.setPixelColor(i,Color(drop[i],map(drop[i],0,255,0,175),map(drop[i],0,255,0,140)));
    
    if(drop[i]>theSpeed){
      drop[i]-=theSpeed;
    }else {
      drop[i]=0;
    };
  }
  dayshade.show();   // write all the pixels out
}

/****************** White ******************\
| A full white fill for full illimunation.  |
| analoga:                                  |
\*******************************************/
void white(int shade){
  for(int i=0; i<numPixels; i++){
    dayshade.setPixelColor(i,Color(255,map(shade,0,1023,175,255),map(shade,0,1023,140,255)));
  }
  dayshade.show();   // write all the pixels out
}

/************ Fill All ************\
| A fill of color for all pixels.  |
| analoga: Color Hue               |
\**********************************/
void fillAll(int argHue, int argSpeed){
static byte offset = 0;
  
  byte WheelPos = map(argHue,0,1023,0,255);
  offset+=map(argSpeed,0,1023,0,20);
  
  for(int i=0; i<numPixels; i++){
    dayshade.setPixelColor(i,Wheel(WheelPos+offset));
  }
  dayshade.show();   // write all the pixels out
}

/************* Rainbow Chase ************\
| A breezy sexy rainbow chase.           |
| argSpeed: The speed at which it cycles |
\****************************************/
void rainbowChase(int argSpeed){
  static byte offset = 0;
  
  for(int i=0;i<20;i++){
    dayshade.setPixelColor(i, Wheel( ((i * 256 / 20) + offset) % 256) );
    dayshade.setPixelColor(i+20, Wheel( ((i * 256 / 20) + offset) % 256) );
    dayshade.setPixelColor(i+40, Wheel( ((i * 256 / 20) + offset) % 256) );
    dayshade.setPixelColor(i+60, Wheel( ((i * 256 / 20) + offset) % 256) );
    dayshade.setPixelColor(99-i, Wheel( ((i * 256 / 20) + offset) % 256) );
    dayshade.setPixelColor(119-i, Wheel( ((i * 256 / 20) + offset) % 256) );
    dayshade.setPixelColor(139-i, Wheel( ((i * 256 / 20) + offset) % 256) );
  }
  dayshade.show();
  offset++;
  delay(map(argSpeed,0,1023,1,50));
}

/******** Police ********\
| Makes Police Lights :D |
| analoga:               |
\************************/
void police(int argColorA, int argColorB){
  byte color[2] = {map(argColorA,0,1023,0,255),map(argColorB,0,1023,0,255)};

  for(int k=0; k<2; k++){
    for(int j=0; j<2; j++){
      for(int i=0; i<numPixels; i++){
        dayshade.setPixelColor(i,Wheel(color[k]));
      }
      dayshade.show();   // write all the pixels out
      
      delay(10);
      for(int i=0; i<numPixels; i++){
        dayshade.setPixelColor(i,Color(0,0,0));
      }
      dayshade.show();   // write all the pixels out
      
      if(j==1){
        delay(160);
      } else {
        delay(20);
      }
    }
  }
}

/******** Strobe ********\
|                        |
| analoga:               |
\************************/
void strobe(int argSpeed){
  for(int i=0;i<numPixels/20;i++){
    dayshade.setPixelColor(random(20)+(i*20),Wheel(random(256)));
  }
  dayshade.show();
  delay(10);

  for(int i=0; i<numPixels; i++){
    dayshade.setPixelColor(i,Color(0,0,0));
  }
  dayshade.show();
  delay(map(argSpeed,0,1023,10,250));
}

//************************************************ Util Functions ************************************************//
//converts the pin to an osc address
char * numToOSCAddress( int pin){
    static char s[10];
    int i = 9;
	
    s[i--]= '\0';
	do
    {
		s[i] = "0123456789"[pin % 10];
                --i;
                pin /= 10;
    }
    while(pin && i);
    s[i] = '/';
    return &s[i];
}
/***************** changeMode *****************\
| This funation changes the mode of the lighs. |
\**********************************************/
// clamb: i'd move control functions like this to the top of the loop() code
// and keep the helper functions Color() and Wheel() down here :)
void changeMode() {
  if(mode==6){
    mode=0;
  } else {
    mode++;
  }
  for(int i=0; i<numPixels; i++){
    dayshade.setPixelColor(i,Color(0,0,0));
  }
  dayshade.show();
  while(digitalRead(2)==HIGH){};
}
/***************** Color *****************\
| Create a 24 bit color value from R,G,B. |
\*****************************************/
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}
/********************** Wheel **********************\
| Input a value 0 to 255 to get a color value.      |
| The colours are a transition r - g -b - back to r |
\***************************************************/
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170; 
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

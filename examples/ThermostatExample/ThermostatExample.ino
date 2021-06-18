/*
  Test Arduino Manager for iPad / iPhone / Mac

   A simple test program to show the Arduino Manager
   features.

   Author: Fabrizio Boco - fabboco@gmail.com

   Version: 1.0

   06/16/2021

   All rights reserved

*/

/*

   AMController library,  example sketches (“The Software”) and the related documentation (“The Documentation”) are supplied to you
   by the Author in consideration of your agreement to the following terms, and your use or installation of The Software and the use of The Documentation
   constitutes acceptance of these terms.
   If you do not agree with these terms, please do not use or install The Software.
   The Author grants you a personal, non-exclusive license, under author's copyrights in this original software, to use The Software.
   Except as expressly stated in this notice, no other rights or licenses, express or implied, are granted by the Author, including but not limited to any
   patent rights that may be infringed by your derivative works or by other works in which The Software may be incorporated.
   The Software and the Documentation are provided by the Author on an "AS IS" basis.  THE AUTHOR MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT
   LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE SOFTWARE OR ITS USE AND OPERATION
   ALONE OR IN COMBINATION WITH YOUR PRODUCTS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE,
   REPRODUCTION AND MODIFICATION OF THE SOFTWARE AND OR OF THE DOCUMENTATION, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
   STRICT LIABILITY OR OTHERWISE, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <AM_Ethernet.h>

/*

   Ethernet Library configuration

*/
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // MAC Address assigned to the board

/*

   IP info

   Using DHCP these parameters are not needed
*/
IPAddress ip(192, 168, 1, 233);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

/*

   Initialize the Ethernet server library
*/
EthernetServer server(80);   // Messages received on port 80

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
#define CHIPSELECT 4
/**

   Other initializations

*/
#define HEATERPIN 8
int heaterActive;

#define COOLERPIN 9
int coolerActive;


#define TEMPERATUREPIN 0
float temperature;

#define CONNECTIONPIN 7
int connectionLed = LOW;

float lowThreshold;
float highThreshold;

/*

   Prototypes of AMController’s callbacks


*/
void doWork();
void doSync(char *variable);
void processIncomingMessages(char *variable, char *value);
void processOutgoingMessages();
void processAlarms(char *variable);
void deviceConnected();
void deviceDisconnected();

/*

   AMController Library initialization

*/
#ifdef ALARMS_SUPPORT
AMController amController(&server, &doWork, &doSync, &processIncomingMessages, &processOutgoingMessages, &processAlarms, &deviceConnected, &deviceDisconnected);
#else
AMController amController(&server, &doWork, &doSync, &processIncomingMessages, &processOutgoingMessages, &deviceConnected, &deviceDisconnected);
#endif

void setup() {
  Serial.begin(9600);

  /*
     SD Card inizialization


     On the Ethernet Shield, CS is pin 4. It's set as an output by default.
     Note that even if it's not used as the CS pin, the hardware SS pin
     (10 on most Arduino boards, 53 on the Mega) must be left as an output
     or the SD library functions will not work.
  */

  pinMode(10, OUTPUT);     // change this to 53 on a mega
  digitalWrite(10, HIGH);  // but turn off the W5100 chip!

  if (!SD.begin(CHIPSELECT))  {

    Serial.println("SD init failed.");
  } else {

    Serial.println("Card present.");
  }

  /*
     Start the Ethernet connection and the server
     ATTENTION: Ethernet Library provided with Arduino 1.0 has the subnet and gateway swapped respect to the documentation

  */
  Ethernet.begin(mac, ip, subnet, gateway);

  server.begin();

  /**

     Other initializations

  */
  //amController.setNTPServerAddress(IPAddress(64,147,116,229));

  heaterActive  = LOW;
  coolerActive  = LOW;

  lowThreshold  = EEPROM_readDouble(0);              // lowThreshold is stored in eprom to be available at board restart
  highThreshold = EEPROM_readDouble(sizeof(float)); // highThreshold is stored in eprom to be available at board restart

  // HEATER
  pinMode(HEATERPIN, OUTPUT);
  digitalWrite(HEATERPIN, heaterActive);

  // COOLER
  pinMode(COOLERPIN, OUTPUT);
  digitalWrite(COOLERPIN, coolerActive);

  // Red LED OFF
  pinMode(CONNECTIONPIN, OUTPUT);
  digitalWrite(CONNECTIONPIN, connectionLed);
}

/**

   Standard loop function

*/
void loop() {

  //amController.loop();
  amController.loop(500);
}

/**


  This function is called periodically and its equivalent to the standard loop() function

*/
void doWork() {

  temperature = getVoltage(TEMPERATUREPIN);  //getting the voltage reading from the temperature sensor
  temperature = (temperature - 0.5) * 100;  // converting from 10 mv per degree with 500 mV offset
  // to degrees ((voltage – 500mV) times 100

  // Cooler is activated when the temperature is slightly higher than the highThreshold

  if (temperature > (highThreshold + 0.5))
    coolerActive = HIGH;
  else
    coolerActive = LOW;

  // Heater is activated when the temperature is slightly lower than the lowThreshold

  if (temperature < (lowThreshold - 0.5))
    heaterActive = HIGH;
  else
    heaterActive = LOW;

  digitalWrite(HEATERPIN, heaterActive);
  digitalWrite(COOLERPIN, coolerActive);
}

/**


  This function is called when the ios device connects and needs to initialize the position of switches and knobs

*/
void doSync() {
    amController.writeMessage("S1", digitalRead(HEATERPIN));
}

/**


  This function is called when a new message is received from the iOS device

*/
void processIncomingMessages(char *variable, char *value) {

  if (strcmp(variable, "S1") == 0) {
    heaterActive = atoi(value);
  }

  if (strcmp(variable, "Thermo1L") == 0) {
    lowThreshold = atof(value);
    EEPROM_writeDouble(0, lowThreshold);
  }

  if (strcmp(variable, "Thermo1H") == 0) {
    highThreshold = atof(value);
    EEPROM_writeDouble(sizeof(float), highThreshold);
  }
}

/**


  This function is called periodically and messages can be sent to the iOS device

*/
void processOutgoingMessages() {
  amController.writeMessage("Thermo1", temperature);
  amController.writeMessage("Cooler", coolerActive);
  amController.writeMessage("Heater", heaterActive);
}

/**


  This function is called when a Alarm is fired

*/
void processAlarms(char *alarm) {

}

/**


  This function is called when the iOS device connects

*/
void deviceConnected () {
  digitalWrite(CONNECTIONPIN, HIGH);
  Serial.println("Device connected");
}

/**


  This function is called when the iOS device disconnects

*/
void deviceDisconnected () {
  digitalWrite(CONNECTIONPIN, LOW);
}

/**

  Auxiliary functions

*/

/*
   getVoltage() – returns the voltage on the analog input defined by pin

*/
float getVoltage(int pin) {

  return (analogRead(pin) * 4.45 / 1023); // converting from a 0 to 1023 digital range
  // to 0 to 5 volts (each 1 reading equals ~ 5 millivolts)
}

void EEPROM_writeDouble(int ee, float value) {
  byte* p = (byte*)(void*)&value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
}

float EEPROM_readDouble(int ee) {
  double value = 0.0;
  byte* p = (byte*)(void*)&value;
  for (unsigned int i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return value;
}

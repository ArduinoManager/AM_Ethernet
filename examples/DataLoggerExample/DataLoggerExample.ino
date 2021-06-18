/*
  Test Arduino Manager for iPad / iPhone / Mac

   Data Logger for Arduino Manager

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
IPAddress ip(192, 168, 1, 230);
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


#define MAXNUMBEROFSAMPLES 20
int samplingCounter;
int currentNumberOfSamples;
boolean gettingSample;

#define SAMPLEINTERVAL .25 // Minutes 

float temp1;
float temp2;

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
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  pinMode(10, OUTPUT);     // change this to 53 on a mega
  digitalWrite(10, HIGH);  // but turn off the W5100 chip!

  if (!SD.begin(CHIPSELECT))  {

    Serial.println(F("SD init failed."));
  } else {

    Serial.println(F("Card present."));
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

  // Red LED OFF

  /** Configure and start Timer1 **/

  cli();//stop interrupts

  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; //initialize counter value to 0

  // set compare match register
  OCR1A = 32534; // = (16*10^6) / (0.25*1024) - 1 (must be <65536)

  TCCR1B |= (1 << WGM12); // turn on CTC mode

  TCCR1B |= (1 << CS12) | (1 << CS10); // Set CS10 and CS12 bits for 1024 prescaler

  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt

  sei();//allow interrupts


  samplingCounter = 0;
  currentNumberOfSamples = MAXNUMBEROFSAMPLES;
  gettingSample = false;
}

/**

   Standard loop function

*/
void loop()
{
  //amController.loop();
  amController.loop(400);
}

/**


  This function is called periodically and its equivalent to the standard loop() function

*/
void doWork() {

  /***** *****/

  temp1 = random(0, 255);
  temp2 = random(0, 255);

  /***** *****/

  if (gettingSample) {
    Serial.println("S");
    unsigned long now = amController.now();

    cli();
    getSample("TEMPS", now);  // TEMPS is the name of the Logged Data Graph Widget
    sei();

    gettingSample = false;
  }

}

/**


  This function is called when the ios device connects and needs to initialize the position of switches and knobs

*/
void doSync () {
}

/**


  This function is called when a new message is received from the iOS device

*/
void processIncomingMessages(char *variable, char *value) {

  //amController.log("processIncomingMessages "); amController.logLn(value);

}

/**


  This function is called periodically and messages can be sent to the iOS device

*/
void processOutgoingMessages() {

  //amController.writeMessage("C",0);
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

}

/**


  This function is called when the iOS device disconnects

*/
void deviceDisconnected () {

}

/**

  Auxiliary functions

*/

/***
  Interrupt Handler
***/
ISR(TIMER1_COMPA_vect) {

  if (samplingCounter == (int)(SAMPLEINTERVAL * 60 / 4)) {

    gettingSample = true;
    samplingCounter = 0;
  }
  else {

    samplingCounter++;
  }

}

/***
  Function to write samples to file
***/
void getSample(const char *variable, unsigned long now1) {

  if (currentNumberOfSamples == MAXNUMBEROFSAMPLES) {

    amController.sdPurgeLogData(variable);

    amController.sdLogLabels(variable, "Temp1", "Temp2");
    currentNumberOfSamples = 0;
  }

  amController.sdLog(variable, now1, temp1, temp2);

  currentNumberOfSamples += 1;
}

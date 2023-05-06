float analogPin = 0; //Analogue input connected to pin 12 from the data output of the Capacittive sensor
float val = 0; // variable to store the value read
float mappedVal = 50; // varialbe to store the mapped moisture sensor value
int OpenVal = 50;
float Open_legth = 5.8 ;
int valve = 0;
float Open_Length_Total = 0;


// Server details
// The server variable can be just a domain name or it can have a subdomain. It depends on the service you are using
const char server[] = "thingspeak.com"; // domain name: example.com, maker.ifttt.com, etc
const char resource[] = "http://api.thingspeak.com/update?api_key=VIM3NJUHXE4NB3AR";  
const int  port = 80;                             // server port number

// Your GPRS credentials (leave empty, if not needed)
const char apn[]      = "three.co.uk"; // APN (example: airtelgprs.com)
const char gprsUser[] = ""; // GPRS User
const char gprsPass[] = ""; // GPRS Password


// SIM card PIN (leave empty, if not defined)
const char simPIN[]   = "";
// get the API key from ThingSpeak.com.
// If you change the apiKeyValue value, 
String apiKeyValue = "VIM3NJUHXE4NB3AR";

// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
// BME280 pins
#define I2C_SDA_2            18
#define I2C_SCL_2            19

// Set serial for debug console (to Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to SIM800 module)
#define SerialAT Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb

// Define the serial console for debug prints, if needed
//#define DUMP_AT_COMMANDS

#include <Wire.h>
#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif



// I2C for SIM800 (to keep it running when powered from battery)
TwoWire I2CPower = TwoWire(0);



// TinyGSM Client for Internet connection
TinyGsmClient client(modem);

#define uS_TO_S_FACTOR 1000000UL   /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  3600        /* Time ESP32 will go to sleep (in seconds) 3600 seconds = 1 hour */

#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

bool setPowerBoostKeepOn(int en) {
  I2CPower.beginTransmission(IP5306_ADDR);
  I2CPower.write(IP5306_REG_SYS_CTL0);
  if (en) {
    I2CPower.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    I2CPower.write(0x35); // 0x37 is default reg value
  }
  return I2CPower.endTransmission() == 0;
}

void setup() {
  // Set serial monitor debugging window baud rate to 115200
  SerialMon.begin(115200);

  // Start I2C communication
  I2CPower.begin(I2C_SDA, I2C_SCL, 400000);

  // Keep power when running from battery
  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Restart SIM800 module, it takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // use modem.init() if you don't need the complete restart

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }

pinMode(analogPin, INPUT); //set pin 12 as an input
pinMode(14,OUTPUT);  //set pin 14 as output
pinMode(13,OUTPUT);  //set pin 14 as output
digitalWrite(13,HIGH);


}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





void thingspeak() {
  SerialMon.print("Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
  }
  else {
    SerialMon.println(" OK");

    SerialMon.print("Connecting to ");
    SerialMon.print(server);
    if (!client.connect(server, port)) {
      SerialMon.println(" fail");
    }
    else {
      SerialMon.println(" OK");
      // Convert the value to a char array
      
      // Making an HTTP POST request
      SerialMon.println("Performing HTTP POST request...");
      // Prepare your HTTP POST request data (Temperature in Celsius degrees)
      
     String httpRequestData = "api_key=" + apiKeyValue + "&field1=" + String(mappedVal)
                               + "&field2=" + String(Open_Length_Total) + "";

      client.print(String("POST ") + resource + " HTTP/1.1\r\n");
      client.print(String("Host: ") + server + "\r\n");
      client.println("Connection: close");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(httpRequestData.length());
      client.println();
      client.println(httpRequestData);

      unsigned long timeout = millis();
      while (client.connected() && millis() - timeout < 10000L) {
        // Print available data (HTTP response from server)
        while (client.available()) {
          char c = client.read();
          SerialMon.print(c);
          timeout = millis();
        }
      }
      SerialMon.println();

      // Close client and disconnect
      client.stop();
      SerialMon.println(F("Server disconnected"));
      modem.gprsDisconnect();
      SerialMon.println(F("GPRS disconnected"));
    }
  }
}


float moisture_sens(){
// put your main code here, to run repeatedly:
val = analogRead(analogPin); //read pin 12 and store it in the variable called val
//Serial.println(val);
val = (1/(val))*100000;
//Serial.println(val); 
float minimum = 26;
int maximum = 650;
int newmin = 0;
int newmax = 100;
mappedVal = ((val-minimum)/maximum)*100;
Serial.println(mappedVal);  //prints the value of the variable val onto the serial monitor
return mappedVal;
}


int valve_control(){
  if (mappedVal <= OpenVal){
      digitalWrite(14,HIGH);
      delay(Open_legth*1000);
      digitalWrite(14,LOW);
      Open_Length_Total = Open_Length_Total + Open_legth;
   
  }
  return Open_Length_Total;
}



void loop() {
moisture_sens();
valve_control();
thingspeak();
delay(15000);
}

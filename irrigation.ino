//#include <ESP8266WiFi.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <EEPROM.h>

// http://lucstechblog.blogspot.com/2020/02/alarm-using-time-and-timealarms.html

/*
   TO DO
   - mDNS ✓ (http://irrigation.local)
   - add information and instructions page ✓
   - wifiManager ✓
   - print wifi qr code (ssid= irrigation, password= sugarloaf)
   - print a box
*/


int sHour1 = 6;        // the hour of day it will be watered
int sMinute1 = 0;     // the minute of the hour it will be watered
int sDuration1 = 5; //duration of system being watered in minutes
int sHour2 = 18;        // the hour of day it will be watered
int sMinute2 = 0;     // the minute of the hour it will be watered
int sDuration2 = 5; //duration of system being watered in minutes

int address1 = 1; //start address where the "sHour1" will be saved
int address2 = 2; //start address where the "sMinute1" will be saved
int address3 = 3; //start address where the "sDuration1" will be saved
int address4 = 4; //start address where the "sHour2" will be saved
int address5 = 5; //start address where the "sMinute2" will be saved
int address6 = 6; //start address where the "sDuration2" will be saved

#define relayPin 5

#define NTP_OFFSET   36000      // In seconds utc+10 hours 
#define NTP_ADDRESS  "au.pool.ntp.org"

WiFiUDP ntpUDP;
ESP8266WebServer server(80); //Server on port 80
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET);

void setup()
{

  sHour1 = EEPROM.read(address1);
  sMinute1 = EEPROM.read(address2);
  sDuration1 = EEPROM.read(address3);
  sHour2 = EEPROM.read(address4);
  sMinute2 = EEPROM.read(address5);
  sDuration2 = EEPROM.read(address6);

  WiFiManager wm;
  WiFi.mode(WIFI_STA);

  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);

  wm.setClass("invert");

  bool res;
  res = wm.autoConnect("irrigation", "sugarloaf"); // password protected ap
  if (!res) {
    Serial.println("Failed to connect or hit timeout");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected");
    if (MDNS.begin("irrigation")) {              // Start the mDNS responder for esp8266.local
      Serial.println("mDNS responder started");
    } else {
      Serial.println("Error setting up MDNS responder!");
    }
  }

  Alarm.alarmRepeat(sHour1, sMinute1, 0, sTimer1);
  Alarm.alarmRepeat(sHour2, sMinute2, 0, sTimer2);

  Alarm.alarmRepeat(1, 0, 0, ntpUpdate);        //update ntp time
  Alarm.alarmRepeat(12, 0, 0, ntpUpdate);


  server.on( "/", HTTP_GET, handleRoot );
  server.on( "/save1", HTTP_POST, handleForm1 );
  server.on( "/save2", HTTP_POST, handleForm2 );
  server.on( "/info", HTTP_GET, handleInfo );

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Webserver started");

  timeClient.begin();
  timeClient.update();
  String newtime = timeClient.getFormattedTime();
  Serial.print("the time is : ");
  Serial.println(newtime);
  Serial.print("Hour    : ");
  Serial.println((newtime.substring(0, 2)).toInt());
  Serial.print("Minute  : ");
  Serial.println((newtime.substring(3, 5)).toInt());
  Serial.print("Seconds : ");
  Serial.println((newtime.substring(6, 8)).toInt());
  //  Serial.println(timeClient.getFormattedDate());
  setTime((newtime.substring(0, 2)).toInt(), (newtime.substring(3, 5)).toInt(), (newtime.substring(6, 8)).toInt(), 1, 1, 20);
}

void loop()
{
  //  Serial.print(hour());
  //  printDigits(minute());
  //  printDigits(second());
  //  Serial.println();
  //Alarm.delay(1000); // show clock every second
  server.handleClient();
}


void sTimer1()
{
  Serial.println("irrigation activated (timer1)");
  digitalWrite(relayPin, HIGH);   // open the valve
  Alarm.delay(sDuration1 * 1000 * 60);         // wait for half a second
  digitalWrite(relayPin, LOW);    // close valve
}

void sTimer2()
{
  Serial.println("irrigation activated (timer2)");
  digitalWrite(relayPin, HIGH);   // open the valve
  Alarm.delay(sDuration2 * 1000 * 60);         // wait for half a second
  digitalWrite(relayPin, LOW);    // close valve
}

void ntpUpdate()
{
  Alarm.delay(1000);
  timeClient.update();
  String newtime = timeClient.getFormattedTime();
  setTime((newtime.substring(0, 2)).toInt(), (newtime.substring(3, 5)).toInt(), (newtime.substring(6, 8)).toInt(), 1, 1, 20);
}

void handleRoot()
{
  server.send(200, "text/html", "<p>the time is:" + timeClient.getFormattedTime() + "</p></br><p>click the button below to view detailed instructions</p><form action=\"/info\" method=\"GET\"><input type=\"submit\" value=\"View Instructions\"></form></br><p>timer 1</p><form action=\"/save\" method=\"POST\"><input type=\"text\" name=\"hour1\" placeholder=\"hour\"></br><input type=\"text\" name=\"minute1\" placeholder=\"minutes\"></br><input type=\"text\" name=\"duration\" placeholder=\"duration eg. 3\"></br><input type=\"submit\" value=\"submit\"></form></br><p>timer 2 (set duration to 0 to disable)</p><form action=\"/save\" method=\"POST\"><input type=\"text\" name=\"hour\" placeholder=\"hour\"></br><input type=\"text\" name=\"minute\" placeholder=\"minute\"></br><input type=\"text\" name=\"duration\" placeholder=\"duration\"></br><input type=\"submit\" value=\"submit\"></form>");
}

void handleForm1()
{
  //https://forum.arduino.cc/t/8266-class-eepromclass-has-no-member-named-update/533379/10
  if (server.arg("hour1") != "") {
    Serial.println("Hours: " + server.arg("hour1"));
    //EEPROM.update(address1, server.arg("hour1"));
    if (server.arg("hour1").toInt() != EEPROM.read(address1)) {
      EEPROM.write(address1, server.arg("hour1").toInt());
    }
  }
  if (server.arg("minute1") != "") {
    Serial.println("Minutes: " + server.arg("minute1"));
    // EEPROM.update(address2, server.arg("minute1"));
    if (server.arg("minute1").toInt() != EEPROM.read(address2)) {
      EEPROM.write(address2, server.arg("minute1").toInt());
    }
  }
  if (server.arg("duration1") != "") {
    Serial.println("Seconds: " + server.arg("duration1"));
    // EEPROM.update(address3, server.arg("duration1"));
    if (server.arg("duration1").toInt() != EEPROM.read(address3)) {
      EEPROM.write(address3, server.arg("duration1").toInt());
    }
  }
}

void handleForm2()
{
  if (server.arg("hour2") != "") {
    Serial.println("Hours: " + server.arg("hour2"));
    //EEPROM.update(address4, server.arg("hour2"));
    if (server.arg("hour2").toInt() != EEPROM.read(address4)) {
      EEPROM.write(address4, server.arg("hour2").toInt());
    }
  }
  if (server.arg("minute2") != "") {
    Serial.println("Minutes: " + server.arg("minute2"));
    // EEPROM.update(address5, server.arg("minute2"));
    if (server.arg("minute2").toInt() != EEPROM.read(address5)) {
      EEPROM.write(address5, server.arg("minute2").toInt());
    }
  }
  if (server.arg("duration2") != "") {
    Serial.println("Seconds: " + server.arg("duration2"));
    // EEPROM.update(address6, server.arg("duration2"));
    if (server.arg("duration2").toInt() != EEPROM.read(address6)) {
      EEPROM.write(address6, server.arg("duration2").toInt());
    }
  }
}


void handleInfo()
{
  server.send(200, "text/html", "<p>click the button below to<strong> go back</strong></p><form action=\"/\" method=\"GET\"><input type=\"submit\" value=\"back\"></br></br><h3>general</h3><p>fill in the fields of timer one if you want to use one timer daily; if you want to use two, fill them both out seperatly and submit with their respective submit buttons.</p><h3>hours</h3><p>the hours can range from 0-23. it is the hour of day you want it watered (in 24 hours).</p><h3>minutes<br></h3><p>can range from 0-59. this is the minute of the pre-specified hour. eg. if you entered 6 in the hour field and 44 in the minute field the timer would go off at 6:44am</p><h3>duration</h3><p>how long the timer would go for (in minutes). following on from the previous example, if duration was 6, it would run from 6:44am until 6:50am (as the duration is 6 minutes.)</p><h3>submit</h3><p>click the button to submit what you have entered in the boxes above. note: this only works for one timer so, if you want two timers you need to fill in timer1, press submit, and then fill in timer2.</p></br><p>hopefully this helps. if unsure please message.</p>");
}

void handleNotFound() {
  server.send(404, "text/html", "<h1>404: Not found</h1>"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

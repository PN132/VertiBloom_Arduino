#include <WiFi.h> //imports wifi library
#include "time.h" //imports time library
#include "sntp.h" //imports library that works with ESP32 (microprocessor)

const char* ssid       = "Wifi-Name"; //wifi network name
const char* password   = "Wifi-Password"; //wifi network password

const char* ntpServer1 = "pool.ntp.org"; //website for time zones 
const char* ntpServer2 = "time.nist.gov"; //website for taking time
const long  gmtOffset_sec = 3600; //offset timer 
const int   daylightOffset_sec = 3600; //used for daylight savings time when needed

const int outputPin = 23; //ouput pin that leads to other electronics 
const int ledPin = 23; //LED indicator for 

const char* time_zone = "PST8PDT";  // TimeZone rule for Pacific time 

//--------------------------------------------------------------------------------------

class specificTime {

private:
  struct tm timeinfo;
  int daysofWeek[7];                 //Array of days where plant should be pollinated 
  int hour;                         //Hour that plant should be pollinated 
  int min;                          //Minute that plant should be pollinated 
  int duration;                     //How long plant should be shaken for, in seconds

public: 
  //Default constructor 
  specificTime() {
    daysofWeek[0] = 1;              //Default day to pollinate is Sundays
    for(int i = 1; i < 7; i++) { 
      daysofWeek[i] = 0;            //Rest of the days of the week are not pollination days 
    }
    hour = 0;                       //Pollinate at hour zero (12a)
    min = 0;                        //Pollinate at minute zero (:00)
    duration = 10;                  //Pollinate for 10 seconds 
  }

  //Parameterized constructor 1
  specificTime(int days[], int d) {
    for(int i = 0; i < 7; i++) {
      daysofWeek[i] = days[i];      //Sets days to pollinate to match days passed in
    }
    hour = timeinfo.tm_hour;        //Sets hour to current hour 
    min = timeinfo.tm_min;          //Sets minute to current minute 
    duration = d;                   //Sets duration to duration passed into constructor 
  }

  //Parameterized constructor 2
  specificTime(int days[], int h, int m, int d) {
    for(int i = 0; i < 7; i++) {
      daysofWeek[i] = days[i];      //Sets days to pollinate to match days passed in
    }
    hour = h;                       //Sets hour to hour passed in 
    min = m;                        //Sets minute to minute passed in
    duration = d;                   //Sets duration to duration passed in 
  }

  //Print out the local time 
  void printLocalTime()
  {
    if(!getLocalTime(&timeinfo)){                             //if time has not been synced 
      Serial.println("No time available (yet)");              //Prints out no time available yet message 
      return;                                                 //Ends the function 
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");       //Prints current time using components of timeinfo
  }

  //Determine if plant needs to be pollinated or not 
  //Should be called once a minute, on the top 
  void checkStatus() {
    if(!getLocalTime(&timeinfo)){                             //if time has not been synced
      Serial.println("No time available (yet)");              //Prints out no time available yet message 
      return;                                                 //Ends the function 
    }

    //Checks if the day, hour, and minute are correct and if the current time is still in the duration of shaking 
    if(daysofWeek[timeinfo.tm_wday] == 1 && hour == timeinfo.tm_hour && timeinfo.tm_min == min && timeinfo.tm_sec <= (duration)) { //Time to pollinate :)
      digitalWrite(outputPin, HIGH); //Turns shaker on 
      digitalWrite(ledPin, HIGH); //Turns indicator light on
      Serial.println("ON"); //Prints on messgae to serial monitor
    } else {                                                                                                                       //Not time to pollinate :(
      digitalWrite(outputPin, LOW); //Turns shaker off 
      digitalWrite(ledPin, LOW); //Turns indicator light off 
      Serial.println("OFF"); //Prints off message to serial monitor
    }
  }

};

//--------------------------------------------------------------------------------------

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
}

specificTime timer; //creates specificTime object as global variable

void setup()
{
  Serial.begin(115200); //starts serial monitor
  pinMode(outputPin, OUTPUT); //Output pin to electrical components controlling shaker 
  pinMode(ledPin, OUTPUT); //LED that indicates whether shaker should be on or not 

  //BELOW IS FROM ARDUINO TEMPLATE "SimpleTime" THAT SETS UP THE TIMER AND WIFI
  //----------------------------------------------------------------------------------------------------------
  // set notification call-back function
  sntp_set_time_sync_notification_cb( timeavailable );

  /**
   * NTP server address could be aquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE aquired NTP server address
   */
  sntp_servermode_dhcp(1);    // (optional)

  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagicaly.
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  /**
   * A more convenient approach to handle TimeZones with daylightOffset 
   * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  //configTzTime(time_zone, ntpServer1, ntpServer2);
  
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);                   //Attempts to connect to Wifi
  while (WiFi.status() != WL_CONNECTED) {       //Checks if Wifi connection was successful 
      delay(500);                               //Prints until wifi is connected 
      Serial.print(".");
  }
  Serial.println(" CONNECTED");                 //Confirmation message for when Wifi is connected

  //----------------------------------------------------------------------------------------------------------

  //My code below 
  int days[] = {0,0,0,0,0,0,0}; //Creates array for days that plant should be pollinated
  Serial.println("Enter days of week you want to pollinate: ");
  int input = Serial.parseInt(); //Takes user input as int
  while(input != -1) { //-1 ends the list of days for plant to be pollinated 
    days[input] = 1; //Marks day indicated as a day to pollinate 
    input = Serial.parseInt(); //Takes in new input from user 
  }

  Serial.println("Enter duration of pollination: ");
  int duration = Serial.parseInt(); //Takes user input for time that plant is pollinated for 
  timer = specificTime(days, duration); //Creates timer object with days and duration parameters

}

struct tm time1; //creats tm struct as global variable

//Content in loop occurs over and over again 
void loop()
{
  delay(1000); //Delays calling the functions
  if(time1.tm_sec == 0) { //checks status at top of every minute 
    timer.printLocalTime(); //Prints out current time
    timer.checkStatus(); //Checks if motor should be running 
  }
  
}

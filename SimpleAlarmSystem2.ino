/*******************************************************************
    A Telegram App bot for ESP8266 that controls the 
    433 MHz home alarm system.

 *******************************************************************/

// The version of ESP8266 core needs to be 2.5 or higher
// or else your bot will not connect.

// ----------------------------
// Standard ESP8266 Libraries
// ----------------------------
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------
#include <RCSwitch.h>// https://github.com/sui77/rc-switch
#include <UniversalTelegramBot.h> //https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot

// Initialize Wifi connection to the router
char ssid[] = "XXX";     // your network SSID (name)
char password[] = "XXX"; // your network key

// Initialize Telegram BOT
#define BOTtoken "XXX:XXXXX" // your Bot Token (Get from Botfather)
String chat_id = "XXX"; // your telegram user id (to chat with)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Checks for new messages every 1 second.
int botRequestDelay = 5000; //1000; //[ms]
unsigned long lastTimeBotRan;

RCSwitch rfReceiver = RCSwitch();

const int ledPin = LED_BUILTIN;
const int rfReceiverPin = 4; //pin D2, works. Connect RFReceiver module to this pin. Power it from 3V3 (not from 5V because of risk of killing GPIO pin).
//const int rfReceiverPin = 14; //pin D5, works. Connect RFReceiver module to this pin. Power it from 3V3 (not from 5V because of risk of killing GPIO pin).
int ledStatus = 0;

typedef struct {
  int remoteID;
  String remoteName;
  int group; // 0 - disarming; 1 - arming; 2 - reported 24h, 3 - reported when armed
  long lastTime;
} device_struct;

#define NUM_DEVICES 33 //<<<
device_struct devices[NUM_DEVICES] = { // add your device RF codes and names here
  
  {000,  "RC1 disarm", 0},
  {000,  "RC1 arm", 1},
  {000,  "RC1 flash", 2},
  {000,  "RC1 SOS", 2},
  {000,  "RC2 disarm", 0},
  {000,  "RC2 arm", 1},
  {000,  "RC2 flash", 2},
  {000,  "RC2 SOS", 2},
  {000,  "RC3 disarm", 0},
  {000,  "RC3 arm", 1},
  {000,  "RC3 flash", 2},
  {000,  "RC3 SOS", 2},
  {000, "RC4 disarm", 0},
  {000, "RC4 arm", 1},
  {000, "RC4 flash", 2},
  {000, "RC4 SOS", 2},
  
  {000,   "26 water spare", 2},
  {000,   "25 water EG", 2},
  {000, "24 water UG", 2},
  
  {000,  "23 smoke UG", 2},
  {000,  "22 smoke EG", 2},
  {000,  "21 smoke UG", 2},
  {000,  "20 smoke EG", 2},
  {000,  "19 smoke OG", 2},
  {000, "18 smoke OG", 2},
  {000,  "17 smoke OG", 2},
  {000, "16 smoke DG", 2},
  {000,  "15 smoke DG", 2},
  {000, "14 smoke DG", 2},
  
  {000, "13 motion spare", 3},
  {000,  "12 motion spare", 3}, //danger of false alarms (foreign sensor)
  {000,  "11 motion OG", 3},
  {000,  "10 motion EG", 3}
  
}; //make the last entry above without a column

bool enable_system = true; //start armed
bool enable_sniffing = false;

int idSearch(int remoteID) {
  for (int i = 0; i < NUM_DEVICES; i++) {
    if (devices[i].remoteID == remoteID)
      return i;
  }
  return -1;
}

void parseRemote(int remoteID) {
  int currentDevice = idSearch(remoteID);
  if (currentDevice >= 0 && currentDevice < NUM_DEVICES ) {
    if (millis() - devices[currentDevice].lastTime > 1500) {
      devices[currentDevice].lastTime = millis();
      Serial.println(String(remoteID) + ": " + devices[currentDevice].remoteName);
      if ((devices[currentDevice].group <= 2) || enable_system)
        bot.sendMessage(chat_id, String(remoteID) + ": " + devices[currentDevice].remoteName, "");
      if (devices[currentDevice].group == 1)
        enable_system = true;
      if (devices[currentDevice].group == 0)
        enable_system = false;  
    }
  } else {
    Serial.println(String(remoteID) + ": " + "unknown code");
    if (enable_sniffing)
      bot.sendMessage(chat_id, String(remoteID) + ": " + "unknown code", ""); //use this to get the sensor id codes 
  }
}

void handleNewMessages(int numNewMessages) {
  //Serial.println("handleNewMessages");
  //Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    //String chat_id = String(bot.messages[i].chat_id);

    //String from_name = bot.messages[i].from_name;
    //if (from_name == "") from_name = "Guest";

    if (bot.messages[i].chat_id != chat_id) { // process commands only from me
      //bot.sendMessage(bot.messages[i].chat_id, "You are not authorized", "");
      return;
    }
    
    String text = bot.messages[i].text;
    
    if (text == "/arm") {
      enable_system = true;
      bot.sendMessage(chat_id, "armed", "");
    }

    else if (text == "/disarm") {
      enable_system = false;
      bot.sendMessage(chat_id, "disarmed", "");
    }
    
    else if (text == "/status") {
      if(enable_system){
        bot.sendMessage(chat_id, "status is armed", "");
      } else {
        bot.sendMessage(chat_id, "status is disarmed", "");
      }
    }
    
    else if (text == "/list") {
      String list = "Device codes, names and groups:\n";
      for (int currentDevice = 0; currentDevice < NUM_DEVICES; currentDevice++ ) {
        list += String(devices[currentDevice].remoteID) + ": " + devices[currentDevice].remoteName + " (" + String(devices[currentDevice].group) + ")\n";
      }
      bot.sendMessage(chat_id, list, "");
    }
    
    else if (text == "/sniff") {
      enable_sniffing = !enable_sniffing;
      if(enable_sniffing){
        bot.sendMessage(chat_id, "sniffing turned on", "");
      } else {
        bot.sendMessage(chat_id, "sniffing turned off", "");
      }
    }
        
    else if (text == "/ledon") {
      digitalWrite(ledPin, LOW);   // turn the LED on (HIGH is the voltage level)
      ledStatus = 1;
      bot.sendMessage(chat_id, "LED turned on", "");
    }

    else if (text == "/ledoff") {
      ledStatus = 0;
      digitalWrite(ledPin, HIGH);    // turn the LED off (LOW is the voltage level)
      bot.sendMessage(chat_id, "LED turned off", "");
    }

    else if ((text == "/start") || (text == "/help")) {
      //String welcome = "Welcome, " + from_name + ".\n";
      String welcome = "Commands:\n";
      welcome += "/arm : to arm\n";
      welcome += "/disarm : to disarm\n";
      welcome += "/status : returns current status\n";
      welcome += "/list : lists all known devices \n";
      welcome += "/sniff : to toggle RF codes sniffing mode\n";
      welcome += "/ledon : to switch the built-in LED on\n";
      welcome += "/ledoff : to switch the built-in LED off\n";
      welcome += "/start or /help : to show commands\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}


void setup() {
  Serial.begin(115200);

  // This is the simplest way of getting this working
  // if you are passing sensitive information, or controlling
  // something important, please either use certStore or at
  // least client.setFingerPrint
  client.setInsecure();

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(chat_id, "booted", "");

  rfReceiver.enableReceive(rfReceiverPin);

  pinMode(ledPin, OUTPUT); // initialize digital ledPin as an output.
  delay(10);
  digitalWrite(ledPin, HIGH); // initialize pin as off (active high)
}

void loop() {

  // listen to rfReceiver
  if (rfReceiver.available()) {
    parseRemote(rfReceiver.getReceivedValue());
    rfReceiver.resetAvailable();
  }

  // check for new Telegram messages
  // This seems to take a second, the rfReception seems to be not listerning for that busy time, it is missing RF codes.
  // I do not have a solution for this problem. A radical solution is not to check messages at all, as in "SimleAlarmSystem" project.
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    //while(numNewMessages) { //AMA commented this out, check only one message per loop cycle
      //Serial.println("got response");
      handleNewMessages(numNewMessages);
      //numNewMessages = bot.getUpdates(bot.last_message_received + 1); //AMA commented this out, process only one message per turn
    //}
    lastTimeBotRan = millis();
  }
}

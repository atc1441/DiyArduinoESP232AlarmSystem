#include <RCSwitch.h>// https://github.com/sui77/rc-switch
#include <WiFi.h>
#include <WiFiClientSecure.h>

char ssid[] = "*****";     // your network SSID (name)
char password[] = "*****"; // your network key
String bot_token = "*****"; // your telegram token, see here how to get one: https://www.youtube.com/watch?v=wP2J5BaQIWs
String user_id = "*****"; // your telegram user id, see here how to get your id: https://youtu.be/-IC-Z78aTOs?t=591

WiFiClientSecure client;
RCSwitch rfReceiver = RCSwitch();

typedef struct {
  int remoteID;
  String remoteName;
  long lastTime;
} device_struct;

#define NUM_DEVICES 2
device_struct devices[NUM_DEVICES] = { // add your devices and names here
  {3202694, "ExampleDevice"},
  {7199161, "Reed_Contact"}
};

bool enable_system = true;
int id_enable_system = 12281112; // add your enable system id here
int id_disable_system = 12281105; // add your disable system id here

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
      Serial.println("Name: " + devices[currentDevice].remoteName);
      if (enable_system)
        Serial.println(sendTelegramMSG(devices[currentDevice].remoteName));
    }
  } else if (remoteID == id_enable_system) {
    enable_system = true;
    Serial.println(sendTelegramMSG("System Enabled"));
  } else if (remoteID == id_disable_system) {
    enable_system = false;
    Serial.println(sendTelegramMSG("System Disabled"));
  } else {
    Serial.println("Got signal from unknown: " + String(remoteID));
    Serial.println(sendTelegramMSG("UNKNOWN_rf_Device_" + String(remoteID)));// uncomment this when you are done programming
  }
}

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.begin(115200);
  Serial.println("Reboot");
  rfReceiver.enableReceive(digitalPinToInterrupt(18));
  Serial.println(sendTelegramMSG("REBOOT"));
}

void loop() {
  if (rfReceiver.available()) {
    parseRemote(rfReceiver.getReceivedValue());
    rfReceiver.resetAvailable();
  }
}

// This function is copied together from the Universal-Arduino-Telegram-Bot made By BrianLough
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot check it out!!
String sendTelegramMSG(String command) {
  String mess = "";
  long now;
  bool avail;
  command = bot_token + "/sendMessage?chat_id=" + user_id + "&text=" + command;
  if (!client.connected()) {
    Serial.println(F("[BOT]Connecting to server"));
    if (!client.connect("api.telegram.org", 443)) {
      Serial.println(F("[BOT]Conection error"));
    }
  }
  if (client.connected()) {
    Serial.println(F(".... connected to server"));
    String a = "";
    char c;
    int ch_count = 0;
    client.println("GET /" + command);
    now = millis();
    avail = false;
    while (millis() - now < 6000) {
      while (client.available()) {
        char c = client.read();
        if (ch_count < 1300) {
          mess = mess + c;
          ch_count++;
        }
        avail = true;
      }
      if (avail) {
        break;
      }
    }
  }
  return mess;
}

/*
Water Sensor to MQTT
By Andy Susanto

Requires PubSubClient found here: https://github.com/knolleary/pubsubclient

ESP8266 Simple MQTT Water Sensor

Wifi.status:
============
0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
3 : WL_CONNECTED after successful connection is established
4 : WL_CONNECT_FAILED if password is incorrect
6 : WL_DISCONNECTED if module is not configured in station mode
*/

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define MQTT_SERVER "10.0.0.30"

// Debug level: 0 (no debug), 1 (standard), 2 (detailed)
int debug = 0;
boolean SerialEnabled = false;

char* ssid1 = "MySSID1";
char* password1 = "MYSSIDpassword";
char* ssid2 = "MySSID";
char* password2 = "MYSSIDpassword";
char* ssid = ssid1;
char* password = password1;

const int DiagPin = LED_BUILTIN;
// LED_BUILTIN in ESP-01 is GPIO-1 (also used as Serial TX)
const int BuzzerPin = 0;
const int SensorPin = 2;
unsigned int BeepFreq=900;  //set to 0 to disable beep
unsigned int BeepFreq1=900;
unsigned int BeepFreq2=100;
boolean BeepState = false;
unsigned long BeepTime;

int LastSumpWaterSensorState = HIGH;
int CurrentSumpWaterSensorState = HIGH;
int TimerFlag=false;
char Uptime[]="{\"Uptime\":\"00000000\"}";  
unsigned long time2;

const char* CPUstateTopic = "stat/SumpWaterSensor/LWT";
const char* SumpWaterSensorInfoTopic = "stat/SumpWaterSensorInfo";
const char* SumpWaterSensorInfoReqTopic = "cmnd/SumpWaterSensorInfo";
const char* SumpWaterSensorRestartTopic = "cmnd/SumpWaterSensorCPUrestart";
const char* SumpWaterSensorResetTopic = "cmnd/SumpWaterSensorCPUreset";
const char* SensorStateTopic = "tele/SumpWaterSensor";
const char* SumpWaterSensorBeepTopic = "cmnd/SumpWaterSensorBeep";
const char* SumpWaterSensorBeepFreqTopic = "cmnd/SumpWaterSensorBeepFreq";
const char* SumpWaterSensorDebugTopic = "cmnd/SumpWaterSensorDebug";

void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
boolean debounce1(boolean last);
String macToStr(const uint8_t* mac);

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

void setup() {
  delay(1000);
  if (!SerialEnabled) {pinMode(DiagPin, OUTPUT);}
  pinMode(SensorPin, INPUT);

if (debug>=1) { 
  Serial.begin(115200);
  Serial.println("Booting");
  SerialEnabled = true;
}  
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
//attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

//wait a bit before starting the main loop
  delay(1000);
  time2 = millis()+1000;
}

void loop() {
  static int TimerCount=0;
  delay(100);

// User Loop Starts Here 

  if (millis()>=time2) {
    TimerFlag=true;
    time2 = millis()+10000;
  }
//reconnect if connection is lost
  if (TimerFlag) {
      if (client.connected()) {
        if (debug>=2) {Serial.print("~");}
      } else {
        if (debug>=2) {Serial.print("x");}
      }
      TimerCount++;
//only fires after 6 timercount (60s)
    if (TimerCount>=6) {   
      if (client.connected() && WiFi.status() == WL_CONNECTED) {
        client.publish(CPUstateTopic,"Online",true);
        sprintf (Uptime, "{\"Uptime\":\"%d\"}", millis()/1000);
        client.publish(SumpWaterSensorInfoTopic,Uptime);
        if (debug>=2) {Serial.print(" esp1 uptime: ");}
        if (debug>=2) {Serial.println(Uptime);}
      }
      TimerCount=0;
    }
  }

  CurrentSumpWaterSensorState = debounce1(LastSumpWaterSensorState);
  if (LastSumpWaterSensorState != CurrentSumpWaterSensorState) {
    if (debug>=2) {Serial.print(" Sensor State: ");}
    if (debug>=2) {Serial.println(CurrentSumpWaterSensorState);}
    if (CurrentSumpWaterSensorState==LOW) {
        client.publish(SensorStateTopic, "WET");
        BeepTime=millis()+400;
        if(BeepFreq>0) {tone(BuzzerPin,BeepFreq1,3000);}
    }
    if (CurrentSumpWaterSensorState==HIGH) {
        client.publish(SensorStateTopic, "DRY");
        tone(BuzzerPin,BeepFreq,300);
    }
    LastSumpWaterSensorState = CurrentSumpWaterSensorState;
  }
  if(CurrentSumpWaterSensorState==LOW && BeepFreq>0){
    if(millis()>=BeepTime){
      if(BeepFreq1==BeepFreq){BeepFreq1=BeepFreq2;} else {BeepFreq1=BeepFreq;}
      tone(BuzzerPin,BeepFreq1);
      BeepTime=millis()+400;
    }
  }
  
  TimerFlag=false;

  //reconnect if connection is lost
  if (!(client.connected() && (WiFi.status() == 3))) {reconnect();}

  //maintain MQTT connection
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10); 

}

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  char payloadChar[length+1];
  
  //Print out some debugging info
  if (debug>=2) {
    Serial.println("Callback update.");
    Serial.print("Topic: ");
    Serial.println(topicStr);
    Serial.println("Payload / Length");
    Serial.print(payload[0]);
    Serial.print("/");
    Serial.println(length);
  }

  if((topicStr == SumpWaterSensorInfoReqTopic) && (payload[0] == 49)){ 
    client.publish(CPUstateTopic,"Online",true);
    sprintf (Uptime, "{\"Uptime\":\"%d\"}", millis()/1000);
    client.publish(SumpWaterSensorInfoTopic,Uptime);
    if (debug>=2) {Serial.print(" WaterSensor ESP1 uptime: ");}
    if (debug>=2) {Serial.println(Uptime);}
  }
  if((topicStr==SumpWaterSensorRestartTopic) && (payload[0] == 49)){ ESP.restart();}
  if((topicStr==SumpWaterSensorResetTopic) && (payload[0] == 49)){ ESP.reset();}
  if(topicStr==SumpWaterSensorBeepFreqTopic) {
    strncpy(payloadChar,(char*)payload,length);
    payloadChar[length]=0x0;
    BeepFreq=atoi(payloadChar);
    if (debug>=1) {Serial.println(BeepFreq);}
  }
  if(topicStr==SumpWaterSensorDebugTopic) {
    strncpy(payloadChar,(char*)payload,length);
    payloadChar[length]=0x0;
    debug=atoi(payloadChar);
    if(debug<=0) { 
    	if(SerialEnabled) {Serial.end();}
    	SerialEnabled = false;
    } else {
    	if(!SerialEnabled) {Serial.begin(115200);}
    	SerialEnabled = true;
		}
    if (debug>=1) {Serial.println(debug);}
  }
  if(topicStr==SumpWaterSensorBeepTopic) {
    if (payload[0] == 49) {
      if (debug>=2) {Serial.println(" Beep Beep ");}
      if(BeepFreq>0) {tone(BuzzerPin,BeepFreq,5000);}
    } else {
      noTone(BuzzerPin);
    }
  }
}


void reconnect() {
  int attempt = 0;
  int DiagState = 0;
  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    if (debug>=1) {Serial.print("Connecting to ");}
    if (debug>=1) {Serial.println(ssid);}

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
			if (!SerialEnabled) {digitalWrite(DiagPin, DiagState);}
      attempt++;
      if (debug>=1) {Serial.println(attempt);}
      if(attempt%30==0){
        if(ssid==ssid1){
          ssid=ssid2;
          password=password2;
        } else {
          ssid=ssid1;
          password=password1;
        }
        WiFi.disconnect();
        delay(500);
        WiFi.begin(ssid, password);
        if (debug>=1) {Serial.print("Switching SSID to ");}
        if (debug>=1) {Serial.println(ssid);}
      }
      if(attempt>60){ESP.restart();}
      if (debug>=1) {Serial.print("WiFi.status()=");}
      if (debug>=1) {Serial.print(WiFi.status());}
      if (debug>=1) {Serial.print(" Attempt=");}
      DiagState+=1;
      if(DiagState>1){DiagState=0;}
    }
    //print out some more debug once connected
    if (debug>=1) {
    Serial.println("");
    Serial.print("WiFi connected to ssid: ");  
    Serial.println(ssid);  
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    }
  }
  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    attempt=0;
    while (!client.connected()) {
      if (debug>=1) {Serial.print("Attempting MQTT connection...");}

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "SumpSensorESP1-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);
      attempt++;
      if(attempt>20){ESP.reset();}
      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str(),CPUstateTopic,2,true,"Offline")) {
        if (debug>=1) {Serial.println("\tMQTT Connected");}
        if (!SerialEnabled) {digitalWrite(DiagPin, HIGH);}
        client.publish(CPUstateTopic,"Online",true);
        client.subscribe(SumpWaterSensorRestartTopic);
        client.subscribe(SumpWaterSensorResetTopic);
        client.subscribe(SumpWaterSensorInfoReqTopic);
        client.subscribe(SumpWaterSensorBeepTopic);
        client.subscribe(SumpWaterSensorBeepFreqTopic);
        client.subscribe(SumpWaterSensorDebugTopic);
        
  // otherwise print failed for debugging
      } else {
 //       Serial.println("\tFailed."); 
        abort();
      }
    }
  }
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }
  return result;
}

boolean debounce1(boolean last)
{
  boolean current = digitalRead(SensorPin);
  if (last != current)
  {
    delay(5);
    current = digitalRead(SensorPin);
  }
  return current;
}



#include <SocketIOClient.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

// #include <Adafruit_MCP23X08.h>
// #include <Adafruit_MCP23X17.h>
// #include <Adafruit_MCP23XXX.h>

// #include <Adafruit_BusIO_Register.h>
// #include <Adafruit_I2CDevice.h>
// #include <Adafruit_I2CRegister.h>
// #include <Adafruit_SPIDevice.h>

// #include <Adafruit_MCP4728.h>


#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
//#include <WebSocketsClient.h>
//#include <WebSocketsServer.h>
//#include <SocketIOClient.h>
//#include <ArduinoJson.h>
//#include "FS.h"
#include "LittleFS.h" // LittleFS is declared
//#include <ESP8266WiFiMulti.h>

///////////// Vadiraj code begin ////////////////////////////
#include <Wire.h>
#include "mcp4728.h"
#include "Adafruit_MCP23017.h"

#define M_A0 0
#define M_A1 1
#define M_A2 2
#define M_A3 3
#define M_A4 4
#define M_A5 5
#define M_A6 6
#define M_A7 7

#define M_B0 8
#define M_B1 9
#define M_B2 10
#define M_B3 11
#define M_B4 12
#define M_B5 13
#define M_B6 14
#define M_B7 15

#define NUM_SW 3
#define FALSE 0
#define TRUE 1


static uint8_t SW0_CTRL_PIN=M_A6;
static uint8_t SW1_CTRL_PIN=M_A7;
static uint8_t SW2_CTRL_PIN=M_A0;

static uint8_t LED1_CTRL_PIN=M_A3;
static uint8_t LED2_CTRL_PIN=M_A2;
static uint8_t LED3_CTRL_PIN=M_A1;


static uint8_t SW0_IP_PIN=M_B0;
static uint8_t SW1_IP_PIN=M_B1;
static uint8_t SW2_IP_PIN=M_B2;

//typedef enum  states{ON,OFF}state_t;
typedef enum  states{OFF,ON}state_t;

typedef struct {
  boolean dimmable;
  boolean s_lvl_chg;
  boolean h_lvl_chg;
  uint16_t s_lvl;  
  uint16_t h_lvl;  
  uint16_t lvl;  
}dim_t;

typedef struct {
  uint8_t ip_pin;
  uint8_t ctrl_pin;
  boolean pos;
  state_t state;
  boolean toggled;
  dim_t dim;
}switch_t;

switch_t sw[NUM_SW];

//instantiate mcp4728 object, Device ID = 0
mcp4728 dac = mcp4728(0); 
Adafruit_MCP23017 mcp;
/////// Vadiraj code end /////////////////////////

/* Set these to your desired credentials. */
//const char *ssid = "RoomSwitch";
//const char *password = "thirdeye";//"thereisnospoon";
//char *ssid = "";
char ssid[50];
//char *password = "";
char password[50];
//char *mac = "";
String mac = "";
String IP = "";
//char *rssid = "";
String rssid = "";
String rpassword = "";
String serssid = "";
String serpwd = "";

bool isConnected = false;
//ESP8266WebServer server(80);
//WiFiServer server(6000);
WiFiServer server(2000);

//ESP8266WiFiMulti WiFiMulti;

SocketIOClient socket;
String host = "192.168.0.8";
//String host = "192.168.1.2";
int port = 5000;

String serIP = "192.168.4.100";
//IPAddress server_ip(192, 0, 0, 100); 
IPAddress server_ip(192, 168, 4, 100); 
//IPAddress gateway(192, 0, 0, 100); 
IPAddress gateway(192, 168, 4, 100);
IPAddress subnet(255, 255, 255, 0);
//String server_ip = "192.168.100.100";
//int ser_port = 6000;
int ser_port = 2000;
String cIP = "";

//WiFiServer deviceServer(7000);
WiFiClient deviceClient;
WiFiClient userClient;
bool isServer = false;
bool isClient = false;
bool switchControl = false;
bool bothClientServer = true;
bool deviceRegistered = false;
static uint32_t prevMilli = 0;

ArduinoJson::StaticJsonBuffer<200> jsonBuffer;

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleRoot() {
	//server.send(200, "text/html", "<h1>You are connected</h1>");
}

void updateComponent(String mac, String cname, int mod, int stat, int val);
void updateSock(String mac, uint8_t sid);
void initSwitch(String mac);
int readComponentMode(String mac, String cname);
void processCommand(String req);
uint8_t getSid(String mac);
int readComponentStat(String mac, String cname);
int readComponentVal(String mac, String cname);
void socketLoop();
void sendHTTPResponse(String val, WiFiClient client);
void consumeRemainder(WiFiClient client);

//Vadiraj code begin
void initialize_mcp23017();
void initialize_mcp4728_dac();
void  initialize_switches();
//Vadiraj code end

void WIFI_Reconnect() {
  Serial.println("WIFI_Reconnect...");
  WiFi.disconnect();
  while (WiFi.status() != WL_CONNECTED) {
  //while (WiFi.status() != 7) {
    WiFi.begin(ssid, password);
    delay(500);
    Serial.print(".");
  }
    
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, sep);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}

bool isUsersPresent(){
    String ip;
    bool present = false;
    //Serial.println("Inside isUsersPresent");
    //File f = SPIFFS.open("/users.txt", "r");
    File f = LittleFS.open("/users.txt", "r");
    if (!f) {
      //Serial.println("isUsersPresent: file open failed");
    }
    else {
      if(f.available()) {
        present = true;
      }
    }
    return present;
}

bool isDevicesPresent(){
    String ip;
    bool present = false;
    //Serial.println("Inside isDevicesPresent");
    //File f = SPIFFS.open("/devices.txt", "r");
    File f = LittleFS.open("/devices.txt", "r");
    if (!f) {
      //Serial.println("isDevicesPresent: file open failed");
    }
    else {
      if(f.available()) {
        present = true;
      }
    }
    return present;
}

void sendToConnectedUsers(String url){
    String ip;
    WiFiClient client;
    char host[50];
    int pos;

    uint32_t prevMilli2 = millis();
    //Serial.println("Inside sendToConnectedUsers");
    /*
    // Check if a client has connected
    client = server.available();
    if (!client) {
      return;
    }
    */
    //File f = SPIFFS.open("/users.txt", "r+");
    File f = LittleFS.open("/users.txt", "r+");
    if (!f) {
      //Serial.println("sendToConnectedUsers: file open failed");
    }
    else {
      while(f.available()) {
        pos = f.position();
        ip = f.readStringUntil('\n');
        
        if ((ip.indexOf("0.0.0.0") == -1) && (ip != "")) {

          //WiFiClient client;
          //const int httpPort = 6000;
          const int httpPort = ser_port;

          //Serial.print("sendToConnectedUsers ip: ");
          //Serial.print(ip);
          //Serial.println();
          ip.toCharArray(host,ip.length()+1);
          if (!client.connect(host, httpPort)) {
            //Serial.println("sendToConnectedUsers: connection failed");
            delay(1);
            f.seek(pos,SeekSet);
            f.print("");
            client.flush();
            continue;
            //return;
          }
          else {
            /*
            // We now create a URI for the request
            String url = "/input/";
            url += streamId;
            url += "?private_key=";
            url += privateKey;
            url += "&value=";
            url += value;
            */
            //Serial.print("sendToConnectedUsers Requesting URL: ");
            //Serial.println(url);

            client.print(url);
            client.flush();
            // This will send the request to the server
            /*
            client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
                 
            unsigned long timeout = millis();
            while (client.available() == 0) {
              if (millis() - timeout > 5000) {
                Serial.println(">>> Client Timeout !");
                client.stop();
                return;
              }
            }
      
            // Read all the lines of the reply from server and print them to Serial
            while(client.available()){
              String line = client.readStringUntil('\r');
              Serial.print(line);
            }
            */
          }
        }
      }
   }
   
   Serial.print("Time taken for sendToConnectedUsers: ");
   Serial.print(millis() - prevMilli2);
   Serial.println();
   
}

void sendFromClient(String ip, int port, String url){
      WiFiClient client;
      char host[50];
      int tries = 3;
      ip.toCharArray(host,ip.length()+1);
      const int httpPort = port;

      Serial.println("serssid="+serssid+" serpwd="+serpwd);
      Serial.println("sendFromClient: url="+url);
      // ESP.eraseConfig();
      // WiFi.disconnect(true);
      WiFi.mode(WIFI_STA);
      if (WiFi.status() != WL_CONNECTED) {
         WiFi.begin(serssid,serpwd);
         delay(1000);
      }
      while(WiFi.status() != WL_CONNECTED) 
        delay(500);


      Serial.print("sendFromClient-wifi client status: ");
      Serial.print(WiFi.status());
      Serial.println();
      
      //if (!client.connect(host, httpPort)) {
      while (!client.connect(host, httpPort)) {
        Serial.println("sendFromClient: connection failed");
        //return;
        tries--;
        if(tries == 0) break;//return;
        delay(1000);
      }
      /*
      // We now create a URI for the request
      String url = "/input/";
      url += streamId;
      url += "?private_key=";
      url += privateKey;
      url += "&value=";
      url += value;
      */
      /*
      if (url.indexOf("pgn=") == -1) {
        Serial.print("Requesting URL: ");
        Serial.println(url);
      }
      */
      //Serial.print("host: "+ip+" port: "+httpPort);
      //Serial.println();
      //delay(500);
      //client.print(url);
      //client.print(String("GET ") + url + " HTTP/1.1\r\n");
      //if (client.connect(host,httpPort))
        client.print(url + "\r\n");
      client.stop();
      delay(500);
      // ESP.eraseConfig();
      // WiFi.disconnect(true);
      // byte bIP[4];
      // parseBytes(cIP.c_str(), '.', bIP, 4, 10);
      // String sid = "RSW-"+mac;
      WiFi.mode(WIFI_AP_STA);
      // WiFi.softAP(sid.c_str(), mac.c_str());//tmac);
      // WiFi.softAPConfig(bIP,bIP,subnet);

      //delay(500);
      /*
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
               
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
        }
      }
      
      // Read all the lines of the reply from server and print them to Serial
      while(client.available()){
        String line = client.readStringUntil('\r');
        Serial.print(line);
      }
      */
}

/*
void deviceServerLoop() {

  delay(10);
  //server.handleClient();
  // Check if a client has connected
  WiFiClient client = deviceServer.available();
  if (!client) {
    return;
  }
  
  Serial.print("client IP: ");
  Serial.print(client.remoteIP());
  Serial.println();
  
  // Wait until the client sends some data
  Serial.println("deviceServerLoop new client");
  if (!client.available()){
    delay(1);
  }
  else {
    
    delay(10);
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();
    // Match the request
    String val;
    if ((req.indexOf("res=") != -1) || (req.indexOf("sta=") != -1)){
      int ind2;
      if (req.indexOf("res=") != -1)
        ind2 = req.indexOf("res=");
      else if (req.indexOf("sta=") != -1)
        ind2 = req.indexOf("sta=");

      String dev1 = req.substring(ind2 + 4);
      int ind1 = dev1.indexOf(";");
      String dev = dev1.substring(0, ind1); 
      //uint8_t sid = getSid(dev);
      int ind = req.indexOf("Comp");
      String comp = req.substring(ind, ind + 4);
      Serial.print("deviceServerLoop res/sta comp: ");
      Serial.print(comp);
      Serial.println();
  
      //String mVal = req.substring(ind+6,ind+7);
      int mod = readComponentMode(dev,comp);//mVal.toInt();
      Serial.print("deviceServerLoop res/sta mod: ");
      Serial.print(mod);
      Serial.println();

      String sStat = req.substring(ind+6,ind+7);
      int stat = sStat.toInt();
      Serial.print("deviceServerLoop res/sta stat: ");
      Serial.print(stat);
      Serial.println();
  
      String sVal = req.substring(ind + 9);
      int val = sVal.toInt();
      Serial.print("deviceServerLoop res/sta val: ");
      Serial.print(val);
      Serial.println();

      updateComponent(dev,comp,mod,stat,val);
      if (socket.connected()) {
        String packet;
        JsonObject& root = jsonBuffer.createObject();
        root["devID"] = dev;
        root["comp"] = comp;
        root["mod"] = mod;
        root["stat"] = stat;
        root["val"] = val;
        root.printTo(packet);
        //socket.emit("response", "{\"devID\":dev, \"comp\":comp, \"val\":val}");
        socket.emit("response", packet);
      }
      sendToConnectedUsers(req);
    }
    else if (req.indexOf("dev=") != -1) {
      char s[80],c[50];
      int ind = req.indexOf("dev=");
      String dev1 = req.substring(ind + 4);
      //int ind1 = dev1.indexOf(" ");
      //String dev = dev1.substring(0, ind1);
      String ip = client.remoteIP().toString();
      insertDevice(dev1,ip);
      initSwitch(dev1);
      for (int i = 0; i < 3; i++) {
        sprintf(c,"%s%d","Comp",i);
        sprintf(s,"req=%s",c); 
        client.print(s);
        delay(100);
      }
    }
    else if (req.indexOf("req=") != -1) {
      char s[80],c[50];
      int ind = req.indexOf("req=");
      String comp = req.substring(ind + 4);
      if (comp.indexOf("Comp0") != -1) {
        sprintf(s,"sta=%s;%s;%d;%d",mac.c_str(),comp.c_str(),sw[1].state,sw[1].dim.lvl);
        //set the component value to val
        //Serial.print("deviceServerLoop cmd sw[0].state: ");
        //Serial.print(sw[1].state);
        //Serial.println();
                      
      }                      
      else if (comp.indexOf("Comp1") != -1) {
        //set the component value to val
        sprintf(s,"sta=%s;%s;%d;%d",mac.c_str(),comp.c_str(),sw[0].state,sw[0].dim.lvl);
        //Serial.print("deviceServerLoop cmd sw[1].state: ");
        //Serial.print(sw[1].state);
        //Serial.println();
                  
      }                      
      else if (comp.indexOf("Comp2") != -1) {
        //set the component value to val
        sprintf(s,"sta=%s;%s;%d;%d",mac.c_str(),comp.c_str(),sw[2].state,sw[2].dim.lvl);
      }
      client.print(s);
    }
    else if (req.indexOf("cmd=") != -1) {
      int ind2 = req.indexOf("cmd=");
      String dev1 = req.substring(ind2 + 4);
      int ind1 = dev1.indexOf(";");
      String dev = dev1.substring(0, ind1); 
      //uint8_t sid = getSid(dev);
      int ind = req.indexOf("Comp");
      String comp = req.substring(ind, ind + 4);
      Serial.print("deviceServerLoop cmd comp: ");
      Serial.print(comp);
      Serial.println();
  
      String mVal = req.substring(ind+6,ind+7);
      int mod = mVal.toInt();
      Serial.print("deviceServerLoop cmd mod: ");
      Serial.print(mod);
      Serial.println();

      String sStat = req.substring(ind+9,ind+10);
      int stat = sStat.toInt();
      Serial.print("deviceServerLoop cmd stat: ");
      Serial.print(stat);
      Serial.println();
  
      String sVal = req.substring(ind + 11);
      int val = sVal.toInt();
      Serial.print("deviceServerLoop cmd val: ");
      Serial.print(val);
      Serial.println();

      if (comp.indexOf("Comp0") != -1) {
        //set the component value to val
        Serial.print("deviceServerLoop cmd sw[0].state: ");
        Serial.print(sw[1].state);
        Serial.println();
                      
      if (((sw[1].state == OFF) && (stat == 1)) || ((sw[1].state == ON) && (stat == 0)))
        sw[1].toggled = TRUE;
      if ((sw[1].state == ON) && (stat == 1)) {  
        sw[1].dim.lvl = val;
        sw[1].dim.s_lvl = val;
        sw[1].dim.s_lvl_chg = TRUE;
      }
                      
      }                      
      else if (comp.indexOf("Comp1") != -1) {
        //set the component value to val
    
        Serial.print("deviceServerLoop cmd sw[1].state: ");
        Serial.print(sw[1].state);
        Serial.println();
                  
        //if (val == 0)
        if (((sw[0].state == OFF) && (stat == 1)) || ((sw[0].state == ON) && (stat == 0)))
          sw[0].toggled = TRUE;
        if ((sw[0].state == ON) && (stat == 1)) {  
          sw[0].dim.lvl = val;
          sw[0].dim.s_lvl = val;
          sw[0].dim.s_lvl_chg = TRUE;
        }                
      }                      
      else if (comp.indexOf("Comp2") != -1) {
        //set the component value to val
        sw[2].toggled = TRUE;
      }
      String response = "res="+dev1;
      //sendFromClient("192.168.4.100",7000,response);
      client.print(response);
    }
    else {
      Serial.println("deviceServerLoop invalid request");
      //client.stop();
      return;
    }

    // Set GPIO2 according to the request
    //digitalWrite(2, val);
    //sendHTTPResponse(val,client);
    
    client.flush();
    consumeRemainder(client);
    //sendHTTPResponse(val,client);
    //Serial.println("Client disonnected");

    // The client will actually be disconnected 
    // when the function returns and 'client' object is detroyed
  }

}
*/

void sendManualResponse(String comp, int stat, int val)
{
  char tmac[50];
  char s[100];
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  int mod = 1;//readComponentMode(tmac,comp);
  //sprintf(s,"sta=%s;%s;%d;%d;%d;",tmac,comp.c_str(),mod,stat,val); 
  Serial.println("Inside sendManualResponse");
  //if (!bothClientServer){
  if (!isClient){
    //if (isUsersPresent()){
      mod = readComponentMode(mac,comp);
      if (mod != -1) {
        sprintf(s,"sta=%s;%s;%d;%d;%d;",tmac,comp.c_str(),mod,stat,val); 
        Serial.print("Inside sendManualResponse : ");
        Serial.println(s);
        updateComponent(mac,comp,mod,stat,val);
      
        if (socket.connected()) {
          /*
          String packet;
          JsonObject& root = jsonBuffer.createObject();
          root["devID"] = mac;
          root["comp"] = comp;
          root["mod"] = mod;
          root["stat"] = stat;
          root["val"] = val;
          root.printTo(packet);
          //socket.emit("response", "{\"devID\":dev, \"comp\":comp, \"val\":val}");
          */
          //String packet = "{\"devID\":\"" + devid + "\", \"comp\":\"" + comp + "\", \"mod\":" + mod + ", \"stat\":" + stat + ", \"val\":" + val + "}";
          String packet = "{\"devID\":\"" + mac + "\", \"comp\":\"" + comp + "\", \"mod\":" + mod + ", \"stat\":" + stat + ", \"val\":" + val + "}";
          socket.emit("response", packet);
        }
      
        //Serial.println("Inside sendManualResponse before calling sendToConnectedUsers");
        sendToConnectedUsers(s);
    }
  }
  else {
    //if (isDevicesPresent())  
    sprintf(s,"sts=%s;%s;%d;%d",tmac,comp.c_str(),stat,val); 
    //sendFromClient("192.168.4.100",6000,s);
    sendFromClient(serIP,ser_port,s);
  }
}

void handleCommand(String data){
  char s[80];
  String ip;
  Serial.print("handleCommand data: " + data);
  Serial.println();
 
  //JsonObject& root = jsonBuffer.parseObject(data);
  //sprintf(s, "cmd  %s  %d  %d  %d",root["comp"],root["mod"],root["stat"],root["val"]);
  //String devid = root["devID"];
  int ind = data.indexOf(":");
  String devstr = data.substring(ind+2);
  int ind1 = devstr.indexOf("\"");
  String devid = devstr.substring(0,ind1);//root[String("devID")];
  Serial.print("devID: " + devid);
  Serial.println();
  ind = devstr.indexOf("comp");
  String compstr = devstr.substring(ind+6);
  Serial.print("compstr: " + compstr);
  Serial.println();
  ind1 = compstr.indexOf("\"");
  int ind2 = compstr.indexOf(",");
  String comp = compstr.substring(ind1+1,ind2-1);//root[String("comp")];
  Serial.print("comp: " + comp);
  Serial.println();
  
  ind = compstr.indexOf("mod");
  String str1 = compstr.substring(ind+5);
  Serial.print("modstr: " + str1);
  Serial.println();
  ind1 = str1.indexOf(",");
  String modstr = str1.substring(0,ind1);
  int mod = modstr.toInt();
  Serial.print("mod: " + modstr);
  Serial.println();
  
  ind = compstr.indexOf("stat");
  str1 = compstr.substring(ind+6);
  Serial.print("statstr: " + str1);
  Serial.println();
  ind1 = str1.indexOf(",");
  String statstr = str1.substring(0,ind1);
  int stat = statstr.toInt();
  Serial.print("stat: " + statstr);
  Serial.println();

  ind = compstr.indexOf("val");
  str1 = compstr.substring(ind+5);
  Serial.print("valstr: " + str1);
  Serial.println();
  //ind1 = str1.indexOf(",");
  //String valstr = str1.substring(0,ind1);
  int val = str1.toInt();
  Serial.print("val: " + val);
  Serial.println();
  
  if (mac.indexOf(devid) != -1) {
    //sprintf(s, "cmd=%s  %s  %d  %d  %d",root["devID"],root["comp"],root["mod"],root["stat"],root["val"]);
    sprintf(s, "cmd=%s;%s;%d;%d;%d;",devid.c_str(),comp.c_str(),mod,stat,val);
    processCommand(s);
  }
  else {
    //uint8_t sid = getSid(root["devID"]);
    //sprintf(s, "cmd  %s  %d  %d  %d",root["comp"],root["mod"],root["stat"],root["val"]);
    //ip = getDeviceIP(root["devID"]);
    ip = getDeviceIP(devid);
    //sendFromClient(ip,6000,s);
    sendFromClient(ip,ser_port,s);
  }
  //socket.emit("response", data);
  //socket.emit("response", "{\"devID\":devid, \"light\":master, \"response\":response}");
}

void device(String data) {
  Serial.println("Device successfully added");  
}

void handleStatus(String data){
  char s[50];
  //Serial.print("handleStatus data: " + data);
  //Serial.println();
  
  //JsonObject& root = jsonBuffer.parseObject(data.c_str());
  //uint8_t sid = getSid(root["devID"]);
  int ind = data.indexOf(":");
  String devstr = data.substring(ind+2);
  int ind1 = devstr.indexOf("\"");
  String devid = devstr.substring(0,ind1);//root[String("devID")];
  Serial.print("devID: " + devid);
  Serial.println();
  ind = devstr.indexOf("comp");
  String compstr = devstr.substring(ind+6);
  Serial.print("compstr: " + compstr);
  Serial.println();
  ind1 = compstr.indexOf("\"");
  int ind2 = compstr.lastIndexOf("\"");
  String comp = compstr.substring(ind1+1,ind2);//root[String("comp")];
  Serial.print("comp: " + comp);
  Serial.println();
  
  int val = 0;
  int mod = 0;
  int stat = 0;
  
  if (devid.indexOf(mac) != -1) {
     if (!switchControl) {
        initSwitch(devid);
        //updateSwitch(dev);
        switchControl = true;
        delay(100);
      }
      //updateSwitch(dev);
  }
  
  if (comp.indexOf("BYE") == -1){
    //mod = readComponentMode(root["devID"],root["comp"]);
    mod = readComponentMode(devid,comp);
    stat = readComponentStat(devid,comp);
    val = readComponentVal(devid,comp);
  }
  //sprintf(s, "sta  %s",root["comp"]);
  //webSocketServer.sendTXT(sid,s);
  //socket.emit("response", data);
  //String packet;
  //JsonObject& root1 = jsonBuffer.createObject();
  //root1["devID"] = devid;
  //root1["comp"] = comp;
  //root1["mod"] = mod;
  //root1["stat"] = stat;
  //root1["val"] = val;
  //root1.printTo(packet);
  //String packet = "{\"name\":\"" + mac + "\", \"master\":\"" + mac +"\"}";
  String packet = "{\"devID\":\"" + devid + "\", \"comp\":\"" + comp + "\", \"mod\":" + mod + ", \"stat\":" + stat + ", \"val\":" + val + "}";
  Serial.print("packet: " + packet);
  Serial.println();
  
  //socket.emit("staresult", "{\"devID\":devid, \"comp\":comp, \"val\":val}");
  socket.emit("staresult", packet);
}

void addSock(String mac, uint8_t sid) {
  char buffer[50];
  char tmac[50];
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  //File f = SPIFFS.open("/socks.txt", "a");
  File f = LittleFS.open("/socks.txt", "a");
  if (!f) {
      Serial.println("file creation failed");
  }
  sprintf(buffer,"%s  %u",tmac,sid);
  f.println(buffer);
  f.close();  
  
}

void updateSock(String mac, uint8_t sid) {
  char s[50],line;
  int pos;
  bool found = false;
  char tmac[50];
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  //File f = SPIFFS.open("/socks.txt", "w+");
  File f = LittleFS.open("/socks.txt", "w+");
  if (!f) {
      Serial.println("updateSock: file creation failed");
  }
  else {
    //while(f.available()) {
      //pos = f.position();
      //line = f.readStringUntil('\n');   
      if (f.find(tmac)) {
        found = true;
        sprintf(s,"  %u",sid);
        //sprintf(s,"%s  %d",mac,sid);
        //f.seek(-pos, SeekCur);
        f.println(s);
      }
    //}
    if (!found) {
      addSock(mac, sid);
    }
  }
  f.close();
  
}

uint8_t getSid(String mac) {
  uint8_t sid;
  char s[50];
  char tmac[50];
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  //File f = SPIFFS.open("/socks.txt", "r");
  File f = LittleFS.open("/socks.txt", "r");
  if (!f) {
      Serial.println("getSid: file open failed");
      return -1;
  }
  else {
    //while(f.available()) {
  
      if (f.find(tmac)) {
        //s = f.readStringUntil('\n');
        sid = f.parseInt();
        f.close();
        return sid;
      //}
    }
    f.close();
    return -1;
  }
  //f.close();
}

int searchComponentEntry(String mac, String cname) {
  String ltext;
  int pos = 0;
  Serial.println("Inside searchComponentEntry");
  //File f = SPIFFS.open("/switches.txt", "r");
  File f = LittleFS.open("/switches.txt", "r");
  if (!f) {
      Serial.println("searchComponentEntry: file open failed");
      //return -1;
  }
  else {
    while(f.available()) {
      pos = f.position();
      ltext = f.readStringUntil('\n');
      
      Serial.print("searchComponentEntry ltext: ");
      Serial.print(ltext);
      Serial.println();
      
      if ((ltext.indexOf(mac) != -1) && (ltext.indexOf(cname) != -1)) {
        
        Serial.print("searchComponentEntry found pos: ");
        Serial.print(pos);
        Serial.println();
        
        break;
      }
    }
  }
  f.close();
  return pos;
}

bool searchDeviceEntry(String mac) {
  String ltext;
  bool found = false;
  Serial.println("Inside searchDeviceEntry");
  //File f = SPIFFS.open("/switches.txt", "r");
  File f = LittleFS.open("/switches.txt", "r");
  if (!f) {
      Serial.println("searchDeviceEntry: file open failed");
      return found;
  }
  else {
    while(f.available()) {
      ltext = f.readStringUntil('\n');
      if (ltext.indexOf(mac) != -1) {
        found = true;
        break;
      }
    }
  }
  f.close();
  return found;
}

bool searchDeviceIPEntry(String mac) {
  String ltext;
  bool found = false;
  Serial.println("Inside searchDeviceIPEntry");
  //File f = SPIFFS.open("/devices.txt", "r");
  File f = LittleFS.open("/devices.txt", "r");
  if (!f) {
      Serial.println("searchDeviceIPEntry: file open failed");
      return found;
  }
  else {
    while(f.available()) {
      ltext = f.readStringUntil('\n');
      if (ltext.indexOf(mac) != -1) {
        found = true;
        break;
      }
    }
  }
  f.close();
  return found;
}

bool searchUserEntry(String mac) {
  String ltext;
  bool found = false;
  Serial.println("Inside searchUserEntry");
  //File f = SPIFFS.open("/users.txt", "r");
  File f = LittleFS.open("/users.txt", "r");
  if (!f) {
      Serial.println("searchUserEntry: file open failed");
      return found;
  }
  else {
    while(f.available()) {
      ltext = f.readStringUntil('\n');
      if (ltext.indexOf(mac) != -1) {
        found = true;
        break;
      }
    }
  }
  f.close();
  return found;
}

String getDeviceIP(String mac) {
  int pos;
  String ip = "";
  //File f = SPIFFS.open("/devices.txt", "r");
  File f = LittleFS.open("/devices.txt", "r");
  if (!f) {
      Serial.println("getDeviceIP: file open failed");
      return ip;
  }
  else {
    pos = searchDeviceIPEntry(mac);
    /*
    Serial.print("getDeviceIP pos: ");
    Serial.print(pos);
    Serial.println();
    Serial.print("getDeviceIP mac: ");
    Serial.print(mac);
    Serial.println();
    */
    f.seek(pos,SeekSet);
    String str1 = f.readStringUntil(' ');
    //Serial.print("getDeviceIP str1: ");
    //Serial.print(str1);
    //Serial.println();
  
    ip = f.readStringUntil('\n');
    //Serial.print("getDeviceIP ip: ");
    //Serial.print(ip);
    //Serial.println();

    f.close();
    return ip;
  }
  
}

void insertDevice(String mac, String ip) {
  char s[50],c[50];
  char tmac[50];
  Serial.println("Inside insertDevice");
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  //File f = SPIFFS.open("/devices.txt", "w+");
  File f = LittleFS.open("/devices.txt", "w+");
  if (!f) {
      Serial.println("insertDevice: file creation failed");
  }
  else {
    //if (f.find(tmac) == -1) {
    if (!searchDeviceIPEntry(mac)) {
      //sprintf(c,"%s",ip);
      sprintf(s,"%s %s",tmac,ip.c_str());
      f.println(s);
      Serial.print("insertDevice Entry: ");
      Serial.print(s);
      Serial.println();
    }
  }
  f.close();
}

void insertUser(String mac) {
  char s[50],c[50];
  char tmac[50];
  Serial.println("Inside insertUser");
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  //File f = SPIFFS.open("/users.txt", "w+");
  File f = LittleFS.open("/users.txt", "w+");
  if (!f) {
      Serial.println("insertUser: file creation failed");
  }
  else {
    //if (f.find(tmac) == -1) {
    if (!searchUserEntry(mac)) {
      //sprintf(c,"%s",ip);
      sprintf(s,"%s",tmac);
      f.println(s);
      Serial.print("insertUser Entry: ");
      Serial.print(s);
      Serial.println();
    }
  }
  f.close();
}

void pingServer()
{
  //Serial.println("Inside pingServer");
  if ((millis() - prevMilli) > 60000) {
      prevMilli = millis();
      //sendFromClient("192.168.4.100",6000,"pgn=");
      sendFromClient(serIP,ser_port,"pgn=");
   }
}

void updateSwitch(String mac) {
  char s[50];
  Serial.println("Inside updateSwitch");
  for (int i=0;i<3;i++) {
    Serial.print("updateSwitch dim.lvl: ");
    Serial.print(sw[i].dim.lvl);
    Serial.print(" stat: ");
    Serial.print(sw[i].state);
    Serial.println();
  }
  int val1;
  if (sw[1].dim.lvl > 1000) 
    val1 = 1000;
  else val1 = sw[1].dim.lvl;
  updateComponent(mac,"Comp0",1,sw[1].state,val1);
  if (sw[0].dim.lvl > 1000) 
    val1 = 1000;
  else val1 = sw[0].dim.lvl;
  updateComponent(mac,"Comp1",1,sw[0].state,val1);
  updateComponent(mac,"Comp2",1,sw[2].state,1000);
  for (int j=0;j<3;j++) {
    sprintf(s,"Comp%d",j);
    readComponent(mac,s);
  }
  /*
  int mod = readComponentMode(mac,"Comp0");
  Serial.print("updateSwitch mod: ");
  Serial.print(mod);
  Serial.println();
  int stat = readComponentStat(mac,"Comp0");
  Serial.print("updateSwitch stat: ");
  Serial.print(stat);
  Serial.println();
  int val = readComponentVal(mac,"Comp0");
  Serial.print("updateSwitch val: ");
  Serial.print(val);
  Serial.println();
  */
}

void initSwitch(String mac) {
  char s[50],c[50];
  char tmac[50];
  Serial.println("Inside initSwitch");
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  if (LittleFS.exists("/users.txt"))
    LittleFS.remove("/users.txt");
  if (LittleFS.exists("/devices.txt"))
    LittleFS.remove("/devices.txt");
  if (LittleFS.exists("/serverc.txt"))
    LittleFS.remove("/serverc.txt");
  if (LittleFS.exists("/switches.txt"))
    LittleFS.remove("/switches.txt");
  bool searchRes = searchDeviceEntry(mac);
  //File f = SPIFFS.open("/switches.txt", "r+");
  File f = LittleFS.open("/switches.txt", "r+");
  if (!f) {
      Serial.println("initSwitch: file creation failed");
  }
  else {
    //if (f.find(tmac) == -1) {
    //if (!searchDeviceEntry(mac)) {
    if (!searchRes) {
      for(int i=0;i<3;i++) {
        sprintf(c,"%s%d","Comp",i);
        sprintf(s,"%s %s %d %d %d",tmac,c,1,0,1000);
        f.println(s);
        Serial.print("initSwitch Entry: ");
        Serial.print(s);
        Serial.println();
      }
    }
  }
  f.close();
}

void updateComponent(String mac, String cname, int mod, int stat, int val) {
  char s[80];
  char tmac[50];
  //int mod,stat;
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  Serial.println("Inside updateComponent");
  int pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r+");
  File f = LittleFS.open("/switches.txt", "r+");
  if (!f) {
      Serial.println("updateComponent: file updation failed");
  }
  else {
    //int pos = searchComponentEntry(mac,cname);
    
    // Serial.print("updateComponent pos: ");
    // Serial.print(pos);
    // Serial.println();
    // Serial.print("updateComponent mac: ");
    // Serial.print(mac);
    // Serial.println();
    // Serial.print("updateComponent cname: ");
    // Serial.print(cname);
    // Serial.println();
    
    f.seek(pos,SeekSet);
    //String str1 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str1: ");
    //Serial.print(str1);
    //Serial.println();
    //String str2 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str2: ");
    //Serial.print(str2);
    //Serial.println();

    //if (f.find(tmac)) {
    //if (f.find(tcname)) {
      //sprintf(s,"%s %s %d %d %d",tmac,tcname,mod,stat,val);
      sprintf(s, "%-17s %-5s %1d %1d %4d", tmac, tcname, mod, stat, val);
      f.println(s);
    //}
    //}
    f.close();
    // int pos = searchComponentEntry(mac,"Comp3");
    // Serial.print("updateComponent pos: ");
    // Serial.println(pos);
  }
}

void updateComponentVal(String mac, String cname, int val) {
  char s[50];
  char tmac[50];
  int mod,stat,pos;
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r+");
  File f = LittleFS.open("/switches.txt", "r+");
  if (!f) {
      Serial.println("updateComponentVal: file updation failed");
  }
  else {
    //pos = searchComponentEntry(mac,cname);
    /*
    Serial.print("readComponentVal pos: ");
    Serial.print(pos);
    Serial.println();
    Serial.print("readComponentVal mac: ");
    Serial.print(mac);
    Serial.println();
    Serial.print("readComponentVal cname: ");
    Serial.print(cname);
    Serial.println();
    */
    f.seek(pos,SeekSet);
    String str1 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str1: ");
    //Serial.print(str1);
    //Serial.println();
    String str2 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str2: ");
    //Serial.print(str2);
    //Serial.println();

    //if (f.find(tmac)) {
    //if (f.find(tcname)) {
      int pos1 = f.position();
      String sMod = f.readStringUntil(' ');
      mod = sMod.toInt();
      String sStat = f.readStringUntil(' ');
      stat = sStat.toInt();
      f.seek(-pos1,SeekCur); 
      sprintf(s," %d %d %d",mod,stat,val);
      f.println(s);
    //}
    //}
    f.close();
  }
}

int readComponentVal(String mac, String cname) {
  int val,mod,stat;
  char tmac[50];
  int pos;
  //mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  //cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  sprintf(tmac,"");
  Serial.println("Inside readComponentVal");
  pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r");
  File f = LittleFS.open("/switches.txt", "r");
  if (!f) {
      Serial.println("readComponentVal: file open failed");
      return -1;
  }
  else {
    //pos = searchComponentEntry(mac,cname);
    /*
    Serial.print("readComponentVal pos: ");
    Serial.print(pos);
    Serial.println();
    Serial.print("readComponentVal mac: ");
    Serial.print(mac);
    Serial.println();
    Serial.print("readComponentVal cname: ");
    Serial.print(cname);
    Serial.println();
    */
    f.seek(pos,SeekSet);
    String str1 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str1: ");
    //Serial.print(str1);
    //Serial.println();
    String str2 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str2: ");
    //Serial.print(str2);
    //Serial.println();
  
    //if (f.find(tmac)) {
    //if (f.find(tcname)) {
      String sMod = f.readStringUntil(' ');
      mod = sMod.toInt();
      String sStat = f.readStringUntil(' ');
      stat = sStat.toInt();
      String sVal = f.readStringUntil('\n');
      // Serial.print("readComponentVal sVal: ");
      // Serial.print(sVal);
      // Serial.println();

      val = sVal.toInt();
      f.close();
      return val;
    //}
    //}
  }
}

void updateComponentMode(String mac, String cname, int mod) {
  char s[50];
  char tmac[50];
  int val,stat,mod1,pos;
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r+");
  File f = LittleFS.open("/switches.txt", "r+");
  if (!f) {
      Serial.println("updateComponentMode: file updation failed");
  }
  else {
    //pos = searchComponentEntry(mac,cname);
    /*
    Serial.print("readComponentVal pos: ");
    Serial.print(pos);
    Serial.println();
    Serial.print("readComponentVal mac: ");
    Serial.print(mac);
    Serial.println();
    Serial.print("readComponentVal cname: ");
    Serial.print(cname);
    Serial.println();
    */
    f.seek(pos,SeekSet);
    String str1 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str1: ");
    //Serial.print(str1);
    //Serial.println();
    String str2 = f.readStringUntil(' ');
    //Serial.print("readComponentVal str2: ");
    //Serial.print(str2);
    //Serial.println();

    //if (f.find(tmac)) {
    //if (f.find(tcname)) {
      int pos1 = f.position();
      String sMod = f.readStringUntil(' ');
      mod1 = sMod.toInt();
      String sStat = f.readStringUntil(' ');
      stat = sStat.toInt();
      String sVal = f.readStringUntil('\n');
      val = sVal.toInt();
      f.seek(-pos1,SeekCur); 
      sprintf(s,"  %d  %d  %d",mod,stat,val);
      f.println(s);
    //}
    //}
    f.close();
  }
}

int readComponentMode(String mac, String cname) {
  int mod,pos;
  char tmac[50];
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  Serial.println("Inside readComponentMode");
  pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r");
  File f = LittleFS.open("/switches.txt", "r");
  if (!f) {
      Serial.println("readComponentMode: file open failed");
      return -1;
  }
  else {
    //pos = searchComponentEntry(mac,cname);
    /*
    Serial.print("readComponentMode pos: ");
    Serial.print(pos);
    Serial.println();
    Serial.print("readComponentMode mac: ");
    Serial.print(mac);
    Serial.println();
    Serial.print("readComponentMode cname: ");
    Serial.print(cname);
    Serial.println();
    */
    f.seek(pos,SeekSet);
    String str1 = f.readStringUntil(' ');
    //Serial.print("readComponentMode str1: ");
    //Serial.print(str1);
    //Serial.println();
  
    String str2 = f.readStringUntil(' ');
    //Serial.print("readComponentMode str2: ");
    //Serial.print(str2);
    //Serial.println();
  
    //if (f.find(tmac)) {
    //if (f.find(tcname)) {
      String sMod = f.readStringUntil(' ');
      mod = sMod.toInt();
      return mod;
    //}
    //}
    f.close();
  }
  
}

void updateComponentStat(String mac, String cname, int stat) {
  char s[50];
  char tmac[50];
  int mod,stat1,val,pos;
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r+");
  File f = LittleFS.open("/switches.txt", "r+");
  if (!f) {
      Serial.println("updateComponentStat: file updation failed");
  }
  else {
      //pos = searchComponentEntry(mac,cname);
      /*
      Serial.print("readComponentVal pos: ");
      Serial.print(pos);
      Serial.println();
      Serial.print("readComponentVal mac: ");
      Serial.print(mac);
      Serial.println();
      Serial.print("readComponentVal cname: ");
      Serial.print(cname);
      Serial.println();
     */
     f.seek(pos,SeekSet);
     String str1 = f.readStringUntil(' ');
     //Serial.print("readComponentVal str1: ");
     //Serial.print(str1);
     //Serial.println();
     String str2 = f.readStringUntil(' ');
     //Serial.print("readComponentVal str2: ");
     //Serial.print(str2);
     //Serial.println();

  //if (f.find(tmac)) {
    //if (f.find(tcname)) {
      int pos1 = f.position();
      String sMod = f.readStringUntil(' ');
      mod = sMod.toInt();
      String sStat = f.readStringUntil(' ');
      stat1 = sStat.toInt();
      String sVal = f.readStringUntil('\n');
      val = sVal.toInt();
      f.seek(-pos1,SeekCur); 
      sprintf(s,"  %d  %d  %d",mod,stat,val);
      f.println(s);
      //}
      //}
      f.close();
  }
}

int readComponentStat(String mac, String cname) {
  int stat,mod,pos;
  char tmac[50];
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  Serial.println("Inside readComponentStat");
  pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r");
  File f = LittleFS.open("/switches.txt", "r");
  if (!f) {
      Serial.println("readComponentStat: file open failed");
      return -1;
  }
  else {
      //pos = searchComponentEntry(mac,cname);
      /*
      Serial.print("readComponentStat pos: ");
      Serial.print(pos);
      Serial.println();
      Serial.print("readComponentStat mac: ");
      Serial.print(mac);
      Serial.println();
      Serial.print("readComponentStat cname: ");
      Serial.print(cname);
      Serial.println();
      */
      f.seek(pos,SeekSet);
      String str1 = f.readStringUntil(' ');
      //Serial.print("readComponentStat str1: ");
      //Serial.print(str1);
      //Serial.println();
  
      String str2 = f.readStringUntil(' ');
      //Serial.print("readComponentStat str2: ");
      //Serial.print(str2);
      //Serial.println();

      //if (f.find(tmac)) {
      //if (f.find(tcname)) {
      String sMod = f.readStringUntil(' ');
      mod = sMod.toInt();
      String sStat = f.readStringUntil(' ');
      stat = sStat.toInt();
      return stat;
      //}
      //}
      f.close();
  }
  
}

void readComponent(String mac, String cname) {
  int stat,mod,val,pos;
  char tmac[50];
  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  char tcname[50];
  cname.toCharArray(tcname,cname.length()+1);
  //tcname += '\0';
  Serial.println("Inside readComponent");
  pos = searchComponentEntry(mac,cname);
  //File f = SPIFFS.open("/switches.txt", "r");
  File f = LittleFS.open("/switches.txt", "r");
  if (!f) {
      Serial.println("readComponent: file open failed");
  }

  //pos = searchComponentEntry(mac,cname);
  /*
  Serial.print("readComponentStat pos: ");
  Serial.print(pos);
  Serial.println();
  Serial.print("readComponentStat mac: ");
  Serial.print(mac);
  Serial.println();
  Serial.print("readComponentStat cname: ");
  Serial.print(cname);
  Serial.println();
  */
  f.seek(pos,SeekSet);
  String str1 = f.readStringUntil(' ');
  // Serial.print("readComponent dev: ");
  // Serial.print(str1);
  // Serial.println();
  
  String str2 = f.readStringUntil(' ');
  // Serial.print("readComponentStat comp: ");
  // Serial.print(str2);
  // Serial.println();

  //if (f.find(tmac)) {
    //if (f.find(tcname)) {
      String sMod = f.readStringUntil(' ');
      mod = sMod.toInt();
      // Serial.print("readComponentStat mod: ");
      // Serial.print(mod);
      // Serial.println();
      String sStat = f.readStringUntil(' ');
      stat = sStat.toInt();
      // Serial.print("readComponentStat stat: ");
      // Serial.print(stat);
      // Serial.println();
      String sVal = f.readStringUntil('\n');
      val = sVal.toInt();
      // Serial.print("readComponentStat val: ");
      // Serial.print(val);
      // Serial.println();

    //}
  //}
  f.close();
  
}

void processCommand(String req)
{
  int ind2 = req.indexOf("cmd=");
  String dev1 = req.substring(ind2 + 4);
  int ind1 = dev1.indexOf(";");
  String dev = dev1.substring(0, ind1); 
  //uint8_t sid = getSid(dev);
  int ind = req.indexOf("Comp");
  String comp = req.substring(ind, ind + 5);
  Serial.print("processCommand comp: ");
  Serial.print(comp);
  Serial.println();
  
  String mVal = req.substring(ind+6,ind+7);
  int mod = mVal.toInt();
  //Serial.print("processCommand mod: ");
  //Serial.print(mod);
  //Serial.println();

  String sStat = req.substring(ind+8,ind+9);
  int stat = sStat.toInt();
  //Serial.print("processCommand stat: ");
  //Serial.print(stat);
  //Serial.println();
  
  String sVal = req.substring(ind + 10);
  int val = sVal.toInt();
  //Serial.print("processCommand val: ");
  //Serial.print(val);
  //Serial.println();

  if (comp.indexOf("Comp0") != -1) {
  //set the component value to val
    //Serial.print("processCommand sw[1].state: ");
    //Serial.print(sw[1].state);
    //Serial.println();
                      
    //if (val == 0)
    
    if (((sw[1].state == OFF) && (stat == 1)) || ((sw[1].state == ON) && (stat == 0)))
      sw[1].toggled = TRUE;
    if ((sw[1].state == ON) && (stat == 1)) {  
      sw[1].dim.lvl = val;
      sw[1].dim.s_lvl = val;
      sw[1].dim.s_lvl_chg = TRUE;
    }
    /*                  
    if (((sw[0].state == OFF) && (stat == 1)) || ((sw[0].state == ON) && (stat == 0)))
      sw[0].toggled = TRUE;
    if ((sw[0].state == ON) && (stat == 1)) {  
      sw[1].dim.lvl = val;
      sw[1].dim.s_lvl = val;
      sw[1].dim.s_lvl_chg = TRUE;
    } 
    */
    //sprintf(s,"res  %s  Comp0  %d  %d  %d",tmac,mod,stat,val); 
    //webSocketClient.sendTXT(s);
  }                      
  else if (comp.indexOf("Comp1") != -1) {
    //set the component value to val
    
    //Serial.print("processCommand sw[0].state: ");
    //Serial.print(sw[0].state);
    //Serial.println();
                  
    //if (val == 0)
    
    if (((sw[0].state == OFF) && (stat == 1)) || ((sw[0].state == ON) && (stat == 0)))
      sw[0].toggled = TRUE;
    if ((sw[0].state == ON) && (stat == 1)) {  
      sw[0].dim.lvl = val;
      sw[0].dim.s_lvl = val;
      sw[0].dim.s_lvl_chg = TRUE;
    } 
    /*               
    if (((sw[1].state == OFF) && (stat == 1)) || ((sw[1].state == ON) && (stat == 0)))
      sw[1].toggled = TRUE;
    if ((sw[1].state == ON) && (stat == 1)) {  
      sw[1].dim.lvl = val;
      sw[1].dim.s_lvl = val;
      sw[1].dim.s_lvl_chg = TRUE;
    }
    */
    //sprintf(s,"res  %s  Comp1  %d  %d  %d",tmac,mod,stat,val); 
    //webSocketClient.sendTXT(s);
  }                      
  else if (comp.indexOf("Comp2") != -1) {
    //set the component value to val
    sw[2].toggled = TRUE;
    //sprintf(s,"res  %s  Comp2  %d  %d  %d",tmac,mod,stat,val); 
    //webSocketClient.sendTXT(s);
  }
  
  Serial.println("Inside processCommand");
  updateComponent(dev,comp,mod,stat,val);
  
  //if (socket.connected()) {
    /*
    String packet;
    JsonObject& root = jsonBuffer.createObject();
    root["devID"] = dev;
    root["comp"] = comp;
    root["mod"] = mod;
    root["stat"] = stat;
    root["val"] = val;
    root.printTo(packet);
    //socket.emit("response", "{\"devID\":dev, \"comp\":comp, \"val\":val}");
    */
    //String packet = "{\"devID\":\"" + dev + "\", \"comp\":\"" + comp + "\", \"mod\":" + mod + ", \"stat\":" + stat + ", \"val\":" + val + "}";
    //socket.emit("response", packet);
  //}
  
}

void setup() {
  const char * preamble = "RSW-";
  char s[50];
  //char*tmac = "";
  char tmac[50];
  strcpy(ssid, "");
  strcpy(password, "");
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("Configuring access point...");
  Serial.println();
	/* You can remove the password parameter if you want the AP to be open. */
  mac = WiFi.macAddress();
  Serial.print("mac: ");
  Serial.print(mac);
  Serial.println();

  mac.toCharArray(tmac,mac.length()+1);
  //tmac += '\0';
  Serial.print("tmac: ");
  Serial.print(tmac);
  Serial.println();
  strcpy(password, tmac);


  sprintf(s,"%s%s",preamble,tmac);
  //Serial.setDebugOutput(true);
  
  //Vadiraj code begin
  initialize_mcp23017();
  initialize_mcp4728_dac();
  initialize_switches();  
  //Vadiraj code end

  //strcpy(ssid, s);
  //sprintf(ssid,"%s",s);


  //strcpy(password, tmac);
  //char*r_ssid = "";
  //char*r_password = "";
  //rpassword.toCharArray(r_password,rpassword.length()+1);
  //rpassword += '\0';
  ESP.eraseConfig();
  //wifi_set_phy_mode(PHY_MODE_11G);
  //WiFi.setAutoConnect(false);
  WiFi.disconnect(true);
  
  WiFi.mode(WIFI_AP_STA);
	WiFi.softAP(s, password);//tmac);
  //WiFi.softAPConfig(server_ip,gateway,subnet);
  //WiFi.config(IPAddress(192, 168, 4, 110), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

	IPAddress myIP = WiFi.softAPIP();
  IP = myIP.toString();
	Serial.print("AP IP address: ");
	Serial.print(myIP);
  Serial.println();
  Serial.print("SSID: ");
  Serial.print(s);
  Serial.println();
  Serial.print("PWD: ");
  Serial.print(password);
  Serial.println();
  Serial.flush();
  strcpy(ssid, s);

	//server.on("/", handleRoot);
	server.begin();
  server.setNoDelay(true);
	Serial.println("HTTP server started");
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  bool result = LittleFS.begin();
  Serial.println("LittleFS opened: " + result);
  if (LittleFS.exists("/users.txt"))
    LittleFS.remove("/users.txt");
  if (LittleFS.exists("/devices.txt"))
    LittleFS.remove("/devices.txt");
  if (LittleFS.exists("/switches.txt"))
    LittleFS.remove("/switches.txt");
  //added by viru on 1-5-2017
  if (LittleFS.exists("/serverc.txt"))
    LittleFS.remove("/serverc.txt");
  
}

void CommandLoop() {
  delay(10);
  //socketLoop();
	//server.handleClient();
  //Serial.print("Server status: ");
  //Serial.print(server.status());
  //Serial.println();

  //added by viru on 1-5-2017
  if (WiFi.status() != WL_CONNECTED) {
  //if (WiFi.status() != 7) {
    //if (isClient) {
      //File g = SPIFFS.open("/serverc.txt", "r");
      File g = LittleFS.open("/serverc.txt", "r");
      if (!g) {
        //Serial.println("CommandLoop: serverc.txt file open failed");
      }
      else {
        //String serssid = g.readStringUntil(' ');
        serssid = g.readStringUntil(' ');
        //String serpwd = g.readStringUntil(' ');
        serpwd = g.readStringUntil(' ');
        String myIP = g.readStringUntil('\n');
        //Serial.println("serssid: "+serssid+" serpwd: "+serpwd+" myIP: "+myIP);
        //commented by viru on 2-5-2017
        /*
        ESP.eraseConfig();
        WiFi.disconnect(true);
        byte bIP[4];
        parseBytes(myIP.c_str(), '.', bIP, 4, 10);
        String sid = "RSW-"+mac;
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(sid.c_str(), mac.c_str());//tmac);
        WiFi.softAPConfig(bIP,bIP,subnet);
        */
        //added by viru on 2-5-2017
        WiFi.begin(serssid.c_str(), serpwd.c_str());
        //String sid = "RSW-"+mac;
        //WiFi.begin(sid, mac);
        delay(10);
        if (WiFi.status() == WL_CONNECTED) {
        //if (WiFi.status() == 7) {
          String newdev = "dev="+mac+";"+myIP+";";
          //sendFromClient("192.168.4.100",6000,newdev);
          sendFromClient(serIP,ser_port,newdev);
        }
        g.close();
      }
    //}
  }
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
   // socketLoop();
    return;
  }

  String cliStr = client.remoteIP().toString();
  Serial.print("client IP: ");
  //Serial.print(client.remoteIP().toString());
  Serial.print(cliStr);
  Serial.println(' ');
  //Serial.println();
  //if (cliStr.indexOf("0.0.0.0") == -1)
    //insertUser(client.remoteIP().toString());
  
  // Wait until the client sends some data
  //Serial.println("new client");
  //if (!client.available()){
    //socketLoop();
    //delay(1);
  //}
  //else {
    /*
    if (isConnected || isClient) {
      if (WiFi.status() != WL_CONNECTED) {
        WIFI_Reconnect();
      }
    }
    */
    
    delay(10);
    Serial.println("RSSI = " + String(WiFi.RSSI()));
    //socketLoop();
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    if (req.indexOf("png=") == -1) 
      Serial.println(req);
    client.flush();
    /*
    String dummy,cmd,pid;
    dummy = client.readStringUntil(':');
    Serial.println("IPD recieved: "+dummy);
    pid = client.readStringUntil('+');
    Serial.println("PID: "+pid);
    cmd = client.readStringUntil('=');
    */
    // Match the request
    String val;
    
    if (req.indexOf("cmd=") != -1) {
      int ind = req.indexOf("cmd=");
      String dev1 = req.substring(ind + 4);
      int ind1 = dev1.indexOf(";");
      String dev = dev1.substring(0, ind1);
      if (dev.indexOf(mac) != -1) {
        processCommand(req); 
      }
      else {
        String ip = getDeviceIP(dev);
        //sendFromClient(ip,6000,"cmt="+dev1);
        sendFromClient(ip,ser_port,"cmt="+dev1);
      }
      val = dev1;
    }
    else if (req.indexOf("ini=") != -1) {
      Serial.print("status query: ");
      Serial.print(req);
      Serial.println();
      int ind = req.indexOf("ini=");
      String dev1 = req.substring(ind + 4);
      Serial.print("dev1: ");
      Serial.print(dev1);
      Serial.println();
      int ind1 = dev1.indexOf(";");
      String dev = dev1.substring(0, ind1); 
      //String dev = dev1.readStringUntil(';');
      Serial.print("dev: ");
      Serial.print(dev);
      Serial.println();
      Serial.print("mac: ");
      Serial.print(mac);
      Serial.println();

      if (dev.indexOf(mac) != -1) {
        if (!switchControl) {
          initSwitch(dev);
          //updateSwitch(dev);
          switchControl = true;
          delay(100);
        }
        //updateSwitch(dev);
      }
      if((dev.compareTo(mac) != 0) && isServer) {
        bothClientServer = false; 
      }
      
      if(dev.indexOf("BYE") == -1){
        String comp = dev1.substring(ind1 + 1, ind1 + 6);
        Serial.print("comp: ");
        Serial.print(comp);
        Serial.println();
        consumeRemainder(client);
        int mod = readComponentMode(dev,comp);
        int stat = readComponentStat(dev,comp);
        int val1 = readComponentVal(dev,comp);
        //webSocketServer.sendTXT(sid,req);
        val = dev+";"+comp+";"+String(mod)+";"+String(stat)+";"+String(val1);
      }
      else {
        val = "BYE;BYE";
      }
    }
    else if (req.indexOf("rem=") != -1) {
    //else if (cmd == "con") {
      //String rssid = client.readStringUntil(';');  
      //const char* rssid = client.readStringUntil(';');  
      int i1,i2;
      String str1,str2;
      i1 = req.indexOf("=");
      i2 = req.indexOf(";");
      str1 = req.substring(i1+1,i2);
      Serial.print("str1: ");
      Serial.print(str1);
      Serial.println();
      //char* rssid = "";
      char rssid[50];
      str1.toCharArray(rssid,str1.length()+1);
      //char c;
      //while ((c = client.read()) != ';') rssid += c;
      //rssid += '\0';
      Serial.print(" SSID: ");
      Serial.print(rssid);
      Serial.println();
      //String pwd = client.readStringUntil(';');
      //const char* pwd = client.readStringUntil(';');
      i1 = req.lastIndexOf(";");
      str2 = req.substring(i2+1,i1);
      Serial.print(" str2: ");
      Serial.print(str2);
      Serial.println();
      //char* pwd = "";
      char pwd[50];
      str2.toCharArray(pwd,str2.length()+1);
      //while ((c = client.read()) != ';') pwd += c;
      //pwd += '\0';
      Serial.print("PWD: ");
      Serial.print(pwd);
      Serial.println();

      Serial.flush();

      //WiFi.mode(WIFI_AP_STA);
      //WiFi.softAP(ssid, password);
      //server.begin();
      //delay(1);

      //WiFi.mode(WIFI_STA);
      //if (WiFi.status() != WL_CONNECTED)
        //WiFi.begin(rssid, pwd);
      //ssid = rssid;
      //password = pwd;
      val = "Remote Configured";
      sendHTTPResponse(val, client);
      
      //WiFi.persistent(false);
      //WiFi.mode(WIFI_OFF);   // this is a temporary line, to be removed after SDK update to 1.5.4
      //WiFi.mode(WIFI_AP_STA);
      //WiFi.setOutputPower(0);
      //WiFi.begin(rssid, pwd);
          
      client.flush();
      
      //ESP.eraseConfig();
      //WiFi.disconnect(true);
        
      if (WiFi.status() != WL_CONNECTED) {  // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
      //if (WiFi.status() != 7) {
        //WiFi.begin(rssid, pwd);       // connect to the network
        Serial.println("rem=Before wifi begin");
        WiFi.begin("Home", "MyHome-2007");
        //WiFi.begin("RSC_5G", "Rscsys@123");
      }    
      
      Serial.print("WiFi Stat: ");
      Serial.print(WiFi.status());
      Serial.println();

      //while (WiFi.status() != WL_CONNECTED) {
        //WiFi.begin(rssid, pwd);
        //delay(500);
        //Serial.print(".");
      //}
      
      
      //if (WiFi.status() != WL_CONNECTED) {
        //WIFI_Reconnect();
      //}
      
      Serial.println("");
      Serial.println("WiFi connected");
      //val = "Remote Configured";
      //WiFi.mode(WIFI_AP_STA);
      //WiFi.softAP(ssid, password);
      isConnected = true;
      //socket.connectHTTP(host, port);
      
      //while (!socket.connected()){
        //socket.connect(host, port);
        //delay(10);
      //}
      
      Serial.flush();
      client.flush();
      consumeRemainder(client);
      String packet = "";
      
      //JsonObject& root = jsonBuffer.createObject();
      //root["name"] = mac;
      //root["master"] = mac;
      //root.printTo(packet);
      
      packet = "{\"name\":" + mac + ", \"master\":" + mac +"}";
      //packet = "{name: " + mac + ", master: " + mac +"}";
      //socket.emit("device", "{\"name\":mac, \"master\":mac}");
      //if (socket.connect(host, port))
      //{
        //socket.emit("device", packet);
        socket.on("command", handleCommand);
        socket.on("status", handleStatus);
        //socket.on("device", device);
        //socket.on("light", light);
      //}
      socket.connect(host, port);
      //isServer = true;
      //webSocketServer.begin();
      //webSocketServer.onEvent(webSocketEventServer);
    }
    else if (req.indexOf("ser=") != -1) {
      //isServer = true;
      //webSocketServer.begin();
      //webSocketServer.onEvent(webSocketEventServer);
      val = "Server Configured";
      //sendHTTPResponse(val,client);
      //deviceServer.begin();
      //deviceServer.setNoDelay(true);
      
      //WiFi.softAPConfig(server_ip,gateway,subnet);
      //IPAddress myIP = WiFi.softAPIP();
      //Serial.print("AP IP address: ");
      //Serial.print(myIP);

      //server.begin();
      
      //WiFi.config(&server_ip);
    }
    else 
    if (req.indexOf("png=") != -1) {
      //  Serial.println("device pinged");
      return;
    }
    else if (req.indexOf("cli=") != -1) {
    //else if (cmd == "con") {
      //String rssid = client.readStringUntil(';');  
      //const char* rssid = client.readStringUntil(';');  
      int i1,i2;
      String str1,str2;
      i1 = req.indexOf("=");
      i2 = req.indexOf(";");
      str1 = req.substring(i1+1,i2);
      //Serial.print("str1: ");
      //Serial.print(str1);
      //Serial.println();
      char crssid[50];
      str1.toCharArray(crssid,str1.length()+1);
      //char c;
      //while ((c = client.read()) != ';') rssid += c;
      //crssid += '\0';
      //Serial.print("SSID: ");
      //Serial.print(crssid);
      //Serial.println();
      //String pwd = client.readStringUntil(';');
      //const char* pwd = client.readStringUntil(';');
      i1 = req.lastIndexOf(";");
      str2 = req.substring(i2+1,i1);
      //Serial.print("str2: ");
      //Serial.print(str2);
      //Serial.println();
      char cpwd[50];
      str2.toCharArray(cpwd,str2.length()+1);
      int i3 = req.indexOf("192.168.4");
      //added by viru on 1-5-2017
      int i4 = req.indexOf("HTTP");
      //modified by viru on 1-5-2017
      cIP = req.substring(i3,i4-1);
      Serial.println("client IP: "+cIP);
      //while ((c = client.read()) != ';') pwd += c;
      //pwd += '\0';
      //Serial.print("PWD: ");
      //Serial.print(cpwd);
      //Serial.println();
      //WiFi.mode(WIFI_STA);
      //WiFi.begin(crssid, pwd);
      Serial.flush();
      client.flush();
      
      //WiFiMulti.addAP("Home", "Shivam-2016");

      //while(WiFiMulti.run() != WL_CONNECTED) {
        //delay(100);
      //}
      /*
      if (WiFi.status() != WL_CONNECTED) {  // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
        WiFi.begin(crssid, cpwd);       // connect to the network
        //WiFi.begin("Jago_India", "9901033225");
      }    
      //delay(3000);
      Serial.print("wifi client status: ");
      Serial.print(WiFi.status());
      Serial.println();
      */
      /*
      Serial.print("remote ip: ");
      Serial.print(WiFi.remoteIP());
      Serial.println();
      */
      //ssid = crssid;
      //password = pwd;

      /*
      while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(crssid, pwd);
        delay(500);
        Serial.print(".");
      }
      */
      /*
      if (WiFi.status() != WL_CONNECTED) {
        WIFI_Reconnect();
      }
      */
    
      //Serial.println("");
      //Serial.println("WiFi connected");
      val = "Client Configured";
      sendHTTPResponse(val,client);
      delay(500);
      serssid = crssid;
      serpwd = cpwd;
      //WiFi.mode(WIFI_AP_STA);
      //WiFi.softAP(ssid, password);
      /*
      char trssid[50];
      rssid.toCharArray(trssid,rssid.length()+1);
      //trssid += '\0';
      char trpassword[50];
      rpassword.toCharArray(trpassword,rpassword.length()+1);
      //trpassword += '\0';
    
      WiFi.softAP(trssid, trpassword,1,1);
      */
      
      //ESP.eraseConfig();
      //wifi_set_phy_mode(PHY_MODE_11G);
      //WiFi.setAutoConnect(false);
      //WiFi.disconnect(true);

      //WiFi.mode(WIFI_STA);
      // byte bIP[4];
      // parseBytes(cIP.c_str(), '.', bIP, 4, 10);
      // String sid = "RSW-"+mac;
      // WiFi.mode(WIFI_AP_STA);
      // WiFi.softAP(sid.c_str(), mac.c_str());//tmac);
      // WiFi.softAPConfig(bIP,bIP,subnet);
      
      //if (WiFi.status() != WL_CONNECTED) {  // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
      //if (WiFi.status() != WIFI_EVENT_STAMODE_GOT_IP) {
      //while(WiFi.status() != WL_CONNECTED) {  // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
        // Serial.print("crssid = ");
        // Serial.print(crssid);
        // Serial.print(" cpwd = ");
        // Serial.println(cpwd);
        //WiFi.enableInsecureWEP();
        // WiFi.begin(crssid, cpwd);       // connect to the network
        //WiFi.begin(sid, mac);       // connect to the network
        //WiFi.begin("RSW-5C:CF:7F:C5:F0:9B", "5C:CF:7F:C5:F0:9B");
        //WiFi.begin("Jago_India", "9901033225");
        // delay(1000);
      // }

      //added by viru on 1-5-2017
      char serc[80];
      //File f = SPIFFS.open("/serverc.txt", "r+");
      File f = LittleFS.open("/serverc.txt", "r+");
      if (!f) {
        Serial.println("serverc.txt: file creation failed");
      }
      else {
        sprintf(serc,"%s %s %s",crssid,cpwd,cIP.c_str());
        f.println(serc);
      }
      f.close();
      //if (WiFi.status() != WL_CONNECTED)
      //if (WiFi.status() != WIFI_EVENT_STAMODE_GOT_IP)
        //WiFi.waitForConnectResult(1000);
      //delay(10);
      //while(WiFi.status() != WL_CONNECTED)
        //delay(500);

      //delay(3000);
      // Serial.print("Commandloop:cli=wifi client status: ");
      // Serial.print(WiFi.status());
      // Serial.println();

      //IPAddress myIP = WiFi.softAPIP();
      //IP = myIP.toString();
	    //Serial.print("AP IP address after reconfigure: ");
	    //Serial.println(myIP);
      //Serial.println("WiFi connected");
      isClient = true;
      String newdev = "dev="+mac+";"+cIP+";";
      //sendFromClient("192.168.4.100",6000,newdev);
      Serial.print("Command loop client configured: ");
      Serial.println(newdev);
      sendFromClient(serIP,ser_port,newdev);
      consumeRemainder(client);

      ESP.eraseConfig();
      WiFi.disconnect(true);
      byte bIP[4];
      parseBytes(cIP.c_str(), '.', bIP, 4, 10);
      String sid = "RSW-"+mac;
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(sid.c_str(), mac.c_str());//tmac);
      WiFi.softAPConfig(bIP,bIP,subnet);
      
      return;
      //deviceServer.begin();
      //deviceServer.setNoDelay(true);

    }
    else if (req.indexOf("req=") != -1) {
      char s[80],c[50];
      int ind = req.indexOf("req=");
      String comp = req.substring(ind + 4);
      if (comp.indexOf("Comp0") != -1) {
        sprintf(s,"sts=%s;%s;%d;%d",mac.c_str(),comp.c_str(),sw[1].state,sw[1].dim.lvl);
        //set the component value to val
        Serial.print("commandloop req sw[0].state: ");
        Serial.print(sw[1].state);
        Serial.println();
                      
      }                      
      else if (comp.indexOf("Comp1") != -1) {
        //set the component value to val
        sprintf(s,"sts=%s;%s;%d;%d",mac.c_str(),comp.c_str(),sw[0].state,sw[0].dim.lvl);
        //Serial.print("commandLoop cmd sw[1].state: ");
        //Serial.print(sw[1].state);
        //Serial.println();
                  
      }                      
      else if (comp.indexOf("Comp2") != -1) {
        //set the component value to val
        sprintf(s,"sts=%s;%s;%d;%d",mac.c_str(),comp.c_str(),sw[2].state,sw[2].dim.lvl);
      }
      //client.print(s);
      //sendFromClient("192.168.4.100",6000,s);
      sendFromClient(serIP,ser_port,s);
      consumeRemainder(client);
      //return;
    }
    else if (req.indexOf("cmt=") != -1) {
      int ind2 = req.indexOf("cmt=");
      String dev1 = req.substring(ind2 + 4);
      int ind1 = dev1.indexOf(";");
      String dev = dev1.substring(0, ind1); 
      //uint8_t sid = getSid(dev);
      int ind = req.indexOf("Comp");
      String comp = req.substring(ind, ind + 5);
      Serial.print("commandLoop cmt comp: ");
      Serial.println(comp);
      //Serial.println();
  
      String mVal = req.substring(ind+6,ind+7);
      int mod = mVal.toInt();
      //Serial.print("commandLoop cmt mod: ");
      //Serial.print(mod);
      //Serial.println();

      String sStat = req.substring(ind+8,ind+9);
      int stat = sStat.toInt();
      Serial.print("commandLoop cmt stat: ");
      Serial.print(stat);
      Serial.println();
  
      String sVal = req.substring(ind + 10);
      int val = sVal.toInt();
      //Serial.print("commandLoop cmt val: ");
      //Serial.print(val);
      //Serial.println();

      if (comp.indexOf("Comp0") != -1) {
        //set the component value to val
        //Serial.print("commandLoop cmd sw[0].state: ");
        //Serial.print(sw[1].state);
        //Serial.println();
                      
      if (((sw[1].state == OFF) && (stat == 1)) || ((sw[1].state == ON) && (stat == 0)))
        sw[1].toggled = TRUE;
      if ((sw[1].state == ON) && (stat == 1)) {  
        sw[1].dim.lvl = val;
        sw[1].dim.s_lvl = val;
        sw[1].dim.s_lvl_chg = TRUE;
      }
                      
      }                      
      else if (comp.indexOf("Comp1") != -1) {
        //set the component value to val
    
        //Serial.print("commandLoop cmt sw[1].state: ");
        //Serial.print(sw[1].state);
        //Serial.println();
                  
        //if (val == 0)
        if (((sw[0].state == OFF) && (stat == 1)) || ((sw[0].state == ON) && (stat == 0)))
          sw[0].toggled = TRUE;
        if ((sw[0].state == ON) && (stat == 1)) {  
          sw[0].dim.lvl = val;
          sw[0].dim.s_lvl = val;
          sw[0].dim.s_lvl_chg = TRUE;
        }                
      }                      
      else if (comp.indexOf("Comp2") != -1) {
        //set the component value to val
        //sw[2].toggled = TRUE;
        Serial.println("sw[2].state: "+ String(sw[2].state));
        //commented by viru on 29-4-2017
        //if (((sw[2].state == OFF) && (stat == 1)) || ((sw[2].state == ON) && (stat == 0))) {
          sw[2].toggled = TRUE;
          Serial.println("sw[2].toggled true");
        //}
        
      }
      //String response = "rsp="+dev1;
      //sendFromClient("192.168.4.100",6000,response);
      //client.print(response);
      consumeRemainder(client);
      return;
    }
    /*
    else if (req.indexOf("res=") != -1) {
      socket.emit("response", "{\"devID\":devid, \"light\":master, \"response\":response}");
    }
    */
    else {
      Serial.println("invalid request");
      //client.stop();
      return;
    }

    // Set GPIO2 according to the request
    //digitalWrite(2, val);
    sendHTTPResponse(val,client);
    
    client.flush();
    consumeRemainder(client);
    /*
    // Prepare the response
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
    s += val;//(val)?"ON":"OFF";
    s += "</html>\n";
    */
    //sendHTTPResponse(val,client);
    //Serial.println("Client disonnected");

    // The client will actually be disconnected 
    // when the function returns and 'client' object is detroyed
  //}
}

void sendHTTPResponse(String val, WiFiClient client)
{
  String httpHeader;
  String httpResponse;
  Serial.print("sendHTTPResponse: ");
  Serial.print(val);
  Serial.println();

  // HTTP Header
  httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n"; 
  httpHeader += "Content-Length: ";
  httpHeader += val.length();
  httpHeader += "\r\n";
  httpHeader +="Connection: keep-alive\r\n\r\n";//close\r\n\r\n";
  httpResponse = httpHeader + val + " "; // There is a bug in this code: the last character of "content" is not sent, I cheated by adding this extra space

  // Send the response to the client
  //client.print(s);
  client.print(httpResponse);
  delay(1);
  
}

void socketLoop()
{
  if (isConnected) {
    socket.monitor();
    if (socket.connected()) {
      //test code need to be removed
      //socket.emit("toggle", "{\"state\":true}");
      if (!deviceRegistered) {
        delay(1000);
        String packet = "{\"name\":\"" + mac + "\", \"master\":\"" + mac +"\"}";
        //String dev = "mydevice";
        //String packet = "{\"name\":" + dev + ", \"master\":" + dev +"}";
        socket.emit("device", packet);
        //socket.emit("device", "{\"name\":mydevice}");
        //socket.emit("toggle", "{\"state\":true}");
        //socket.on("command", handleCommand);
        //socket.on("status", handleStatus);
        deviceRegistered = true;
      }
    }
  }

  //if (isServer)
    //webSocketServer.loop();
  
  //if (isClient)
    //webSocketClient.loop();
}

void consumeRemainder(WiFiClient client)
{
  String remainder = "";
  client.flush();
  while (client.available()) {
    char c = client.read();
    //remainder += c;
  }
  //Serial.print("remainder: ");
  //Serial.print(remainder);
  //Serial.println();
  Serial.flush();
  while (Serial.available()) 
    char ch = Serial.read();
  
}

///////////////// Vadiraj Code Begin //////////////////////////////
/*
#include <Wire.h>
#include "mcp4728.h"
#include "Adafruit_MCP23017.h"

#define M_A0 0
#define M_A1 1
#define M_A2 2
#define M_A3 3
#define M_A4 4
#define M_A5 5
#define M_A6 6
#define M_A7 7

#define M_B0 8
#define M_B1 9
#define M_B2 10
#define M_B3 11
#define M_B4 12
#define M_B5 13
#define M_B6 14
#define M_B7 15

#define NUM_SW 3
#define FALSE 0
#define TRUE 1


static uint8_t SW0_CTRL_PIN=M_A6;
static uint8_t SW1_CTRL_PIN=M_A7;
static uint8_t SW2_CTRL_PIN=M_A0;

static uint8_t LED1_CTRL_PIN=M_A3;
static uint8_t LED2_CTRL_PIN=M_A2;
static uint8_t LED3_CTRL_PIN=M_A1;


static uint8_t SW0_IP_PIN=M_B0;
static uint8_t SW1_IP_PIN=M_B1;
static uint8_t SW2_IP_PIN=M_B2;

typedef enum  states{ON,OFF}state_t;

typedef struct {
  boolean dimmable;
  boolean lvl_chg;
  uint16_t lvl;  
}dim_t;

typedef struct {
  uint8_t ip_pin;
  uint8_t ctrl_pin;
  boolean pos;
  state_t state;
  boolean toggled;
  dim_t dim;
}switch_t;

switch_t sw[NUM_SW];

//instantiate mcp4728 object, Device ID = 0
mcp4728 dac = mcp4728(0); 
Adafruit_MCP23017 mcp;


void setup()
{
  Serial.begin(115200);  // initialize serial interface for print() 
  initialize_mcp23017();
  initialize_mcp4728_dac();
  initialize_switches();  
  //printStatus(); // Print all internal value and setting of input register and EEPROM.   
  delay(1000);
}
*/
void  initialize_switches()
{
  int lvl, vtg;  
  for(int i=0; i<NUM_SW;i++)
  {  
    switch(i)
    {
      case 0: //Dimmable Lamp
              sw[i].ip_pin=SW0_IP_PIN;
              sw[i].ctrl_pin=SW0_CTRL_PIN;
              sw[i].dim.dimmable=TRUE;
              lvl = analogRead(A0);                            
              sw[i].dim.lvl=lvl;              
              sw[i].dim.s_lvl_chg=FALSE;
              sw[i].dim.h_lvl_chg=FALSE;
              sw[i].dim.s_lvl=lvl;  
              sw[i].dim.h_lvl=lvl;    
              break;
      
      case 1: //FAN
              sw[i].ip_pin=SW1_IP_PIN;
              sw[i].ctrl_pin=SW1_CTRL_PIN;
              sw[i].dim.dimmable=TRUE;
              lvl = analogRead(A0);                            
              sw[i].dim.lvl=lvl;  
              sw[i].dim.s_lvl_chg=FALSE;
              sw[i].dim.h_lvl_chg=FALSE;
              sw[i].dim.s_lvl=lvl;  
              sw[i].dim.h_lvl=lvl;                        
              break;
      
      case 2: //ON/OFF switch
              sw[i].ip_pin=SW2_IP_PIN;
              sw[i].ctrl_pin=SW2_CTRL_PIN;
              sw[i].dim.dimmable=FALSE;              
              break;            
    }    
    sw[i].pos=mcp.digitalRead(sw[i].ip_pin);    
    if(sw[i].pos==1 ||sw[i].pos==HIGH)
      sw[i].state=OFF;
    else
      sw[i].state=ON;      
      //commented by viru on 29-4-2017
    //sw[i].toggled=TRUE;    
  }
  for(int i=0; i<NUM_SW;i++)
  {  
      Serial.print("sw[i].state: ");
      Serial.print(sw[i].state);
      Serial.println();
  }
}

void initialize_mcp23017()
{
   //MCP23017 Initializations
  mcp.begin();      // use default address 0
  mcp.pinMode(M_B0, INPUT); //B0
  mcp.pullUp(8, HIGH);  
  mcp.pinMode(M_B1, INPUT); //B1
  mcp.pullUp(9, HIGH);  
  mcp.pinMode(M_B2, INPUT); //B2
  mcp.pullUp(10, HIGH);

  mcp.pinMode(M_A0,OUTPUT);  //A0
  mcp.pinMode(M_A1,OUTPUT); //LED1
  mcp.pinMode(M_A2,OUTPUT);//LED2
  mcp.pinMode(M_A3,OUTPUT);//LED3

  //Used to Control Dimmer Lights ON/OFF
  mcp.pinMode(M_A6,OUTPUT); //A6
  mcp.pinMode(M_A7,OUTPUT); //A7  

  mcp.digitalWrite(M_A0,LOW);
  mcp.digitalWrite(M_A1,HIGH);
  mcp.digitalWrite(M_A2,HIGH);
  mcp.digitalWrite(M_A3,HIGH);  
  mcp.digitalWrite(M_A6,LOW); //A6
  mcp.digitalWrite(M_A7,LOW); //A7    

  delay(500);  
  
  mcp.digitalWrite(M_A1,LOW);
  mcp.digitalWrite(M_A2,LOW);
  mcp.digitalWrite(M_A3,LOW);
}

void initialize_mcp4728_dac()
{
  dac.begin();  // initialize i2c interface
  dac.vdd(3300); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout

  /* 
  Basic analog writing to DAC
  */

  /* VADI
  dac.analogWrite(500,500,500,500); // write to input register of DAC four channel (channel 0-3) together. Value 0-4095
  dac.analogWrite(0,700); // write to input register of a DAC. Channel 0-3, Value 0-4095
  int value = dac.getValue(0); // get current input register value of channel 0
  Serial.print("input register value of channel 0 = ");
  Serial.println(value, DEC); // serial print of value
  

  dac.voutWrite(1800, 1800, 1800, 1800); // write to input register of DAC. Value(mV) (V < VDD)
  dac.voutWrite(2, 1600); // write to input register of DAC. Channel 0 - 3, Value(mV) (V < VDD)
  int vout = dac.getVout(1); // get current voltage out of channel 1
  Serial.print("Voltage out of channel 1 = "); 
  Serial.println(vout, DEC); // serial print of value
*/

/* 
  Voltage reference settings
    Vref setting = 1 (internal), Gain = 0 (x1)   ==> Vref = 2.048V
    Vref setting = 1 (internal), Gain = 1 (x2)   ==> Vref = 4.096V
    Vref setting = 0 (external), Gain = ignored  ==> Vref = VDD
*/

  //dac.setVref(1,1,1,1); // set to use internal voltage reference (2.048V)
  dac.setVref(0, 0); // set to use external voltage reference (=VDD, 2.7 - 5.5V)
  dac.setVref(1, 0); // set to use external voltage reference (=VDD, 2.7 - 5.5V)
  int vref1 = dac.getVref(0); // get current voltage reference setting of channel 1
  int vref2 = dac.getVref(1); // get current voltage reference setting of channel 1
  
  Serial.print("Voltage reference setting of channel 1 = "); // serial print of value
  Serial.println(vref1, DEC); // serial print of value
  Serial.print("Voltage reference setting of channel 2 = "); // serial print of value
  Serial.println(vref2, DEC); // serial print of value

  dac.setGain(0, 0, 0, 0); // set the gain of internal voltage reference ( 0 = gain x1, 1 = gain x2 )
  //dac.setGain(1, 1); // set the gain of internal voltage reference ( 2.048V x 2 = 4.096V )
  int gain = dac.getGain(1); // get current gain setting of channel 2
  Serial.print("Gain setting of channel 1 = "); // serial print of value
  Serial.println(gain, DEC); // serial print of value


/*
  Power-Down settings
    0 = Normal , 1-3 = shut down most channel circuit, no voltage out and saving some power.
    1 = 1K ohms to GND, 2 = 100K ohms to GND, 3 = 500K ohms to GND
*/

  dac.setPowerDown(0,0,0,0); // set Power-Down ( 0 = Normal , 1-3 = shut down most channel circuit, no voltage out) (1 = 1K ohms to GND, 2 = 100K ohms to GND, 3 = 500K ohms to GND)
  dac.setPowerDown(3, 1); // Power down channel 3 ( no voltage out from channel 3 )
  dac.setPowerDown(4, 1); // Power down channel 3 ( no voltage out from channel 3 )
  
  int powerDown3 = dac.getPowerDown(3); // get current Power-Down setting of channel 3
  int powerDown4 = dac.getPowerDown(4); // get current Power-Down setting of channel 3
  
  Serial.print("PowerDown setting of channel 3 = "); // serial print of value
  Serial.println(powerDown3, DEC); // serial print of value
  Serial.print("PowerDown setting of channel 4 = "); // serial print of value
  Serial.println(powerDown4, DEC); // serial print of value


/*
  Write values to EEPROM
    Writing value to EEPROM always update input register as well.
    Writing to EEPROM normally take about 50ms.
    To write Vref, Gain, Power-Down settings to EEPROM, just use eepromWrite() after set them in input registers.
*/
#if 0
  dac.eepromWrite(1500,1500,1500,1500); // write to EEPROM of DAC four channel together. Value 0-4095
  delay(100);//writing to EEPROM takes about 50ms
  dac.eepromWrite(1, 1000); // write to EEPROM of DAC. Channel 0-3, Value 0-4095
  delay(100);//writing to EEPROM takes about 50ms
  dac.eepromWrite(); // write all input register values and settings to EEPROM
  delay(100);//
#endif

/*
  Get Device ID (up to 8 devices can be used in a I2C bus, Device ID = 0-7)
*/

  int id = dac.getId(); // return devideID of object
  Serial.print("Device ID  = "); // serial print of value
  Serial.println(id, DEC); // serial print of value

  //dac.analogWrite(0,600); // write to input register of a DAC. Channel 0-3, Value 0-4095
  //dac.analogWrite(1,1000); // write to input register of a DAC. Channel 0-3, Value 0-4095
  //dac.voutWrite(0, 100);
  //dac.voutWrite(1, 1800);
}

void read_switches()
{  
  for(int i=0;i<NUM_SW;i++)
  {
    int new_pos=mcp.digitalRead(sw[i].ip_pin);
    int new_lvl=analogRead(A0);
        
    if((new_pos != sw[i].pos))
    {    
      sw[i].toggled=TRUE;
      sw[i].pos=new_pos;              
    }    
    if(sw[i].dim.dimmable==TRUE)    
    {       
      if(!(new_lvl >= sw[i].dim.h_lvl-4 && new_lvl <= sw[i].dim.h_lvl+4))
      //if(!(new_lvl >= sw[i].dim.lvl-3 && new_lvl <= sw[i].dim.lvl+3))
      { 
        sw[i].dim.lvl=new_lvl;
        sw[i].dim.h_lvl=new_lvl;
        sw[i].dim.h_lvl_chg=TRUE;
        Serial.print("sw[i].dim.h_lvl: ");
        Serial.print(sw[i].dim.h_lvl);   
        Serial.println();
        Serial.print("sw[i].dim.s_lvl: ");
        Serial.print(sw[i].dim.s_lvl);   
        Serial.println();

      }
    }
  }  
}

/*
void handle_switches()
{
  uint16_t vtg=0;  
  for(int i=0;i<NUM_SW;i++)
  {    
    switch(i)
    {
      case 0: //Dimmable Lamp
        if(sw[i].toggled==TRUE)
        {           
          //Serial.println("SW0 Toggled...");       
          sw[i].toggled=FALSE;                  
          if(sw[i].state==OFF && sw[i].dim.lvl <= 1024)
          {
            sw[i].state=ON;                        
            if(sw[i].dim.lvl >1000)
              sw[i].dim.lvl=1000;
            //vtg = map(sw[i].dim.lvl, 1, 1024, 1, 3300);            
            vtg = map(sw[i].dim.lvl, 1, 1000, 10, 3050);            
            //vtg = 3050;
            dac.voutWrite(0, vtg);            
            delay(5);
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);            
            mcp.digitalWrite(LED1_CTRL_PIN,HIGH);
            Serial.println("handle_switches case dimlight toggle true, state off, dim.lvl <= 1024");
            sendManualResponse("Comp1",1,sw[i].dim.lvl);
          }
          else if(sw[i].state==ON)         
          {
            sw[i].state=OFF;            
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED1_CTRL_PIN,LOW);
            dac.voutWrite(0, 0);
            Serial.println("handle_switches dimlight toggle true, state on");
            sendManualResponse("Comp1",0,sw[i].dim.lvl);                                                      
          }
        }
        if(sw[i].state==ON && sw[i].dim.lvl_chg==TRUE)         
        {           
          Serial.println("SW0 LVL Changed..."); 
          sw[i].dim.lvl_chg=FALSE;
          if(sw[i].dim.lvl <=1000){          
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);
            mcp.digitalWrite(LED1_CTRL_PIN,HIGH);            
            //vtg = map(sw[i].dim.lvl, 1, 1024, 1, 3300);            
            vtg = map(sw[i].dim.lvl, 1, 1000, 10, 3050);            
            
            dac.voutWrite(0, vtg);
          }
          
          //else{         
            //mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            //mcp.digitalWrite(LED1_CTRL_PIN,LOW);
            //dac.voutWrite(0, 0);     
          //}
          
          Serial.println("handle_switches dimlight toggle false, state on, dim.lvl_chg true");
          sendManualResponse("Comp1",1,sw[i].dim.lvl);
          
        }
        
        //Serial.print("\n");   
        //Serial.print(sw[i].dim.lvl);
        //Serial.print("\t");   
        //Serial.println(vtg);
                
        break;

      case 1: //  FAN
        if(sw[i].toggled==TRUE)
        {           
          //Serial.println("SW1 Toggled...");       
          sw[i].toggled=FALSE;                  
          if(sw[i].state==OFF && sw[i].dim.lvl <= 1024)
          {
            sw[i].state=ON;           
            if(sw[i].dim.lvl >1000)
              sw[i].dim.lvl=1000;
            vtg = map(sw[i].dim.lvl, 1, 1000, 50, 2500);            
            //vtg = map(sw[i].dim.lvl, 1, 1024, 1, 3300);            
            dac.voutWrite(1, vtg);            
            delay(5);
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);            
            mcp.digitalWrite(LED2_CTRL_PIN,HIGH);
            Serial.println("handle_switches case fan toggle true, state off, dim.lvl <= 1024");
            sendManualResponse("Comp0",1,sw[i].dim.lvl);

          }
          else if(sw[i].state==ON)         
          {
            sw[i].state=OFF;            
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED2_CTRL_PIN,LOW);
            dac.voutWrite(1, 0);
            Serial.println("handle_switches fan toggle true, state on");         
            sendManualResponse("Comp0",0,sw[i].dim.lvl);                                             
          }
        }
        if(sw[i].state==ON && sw[i].dim.lvl_chg==TRUE)         
        {           
          Serial.println("SW1 LVL Changed..."); 
          sw[i].dim.lvl_chg=FALSE;
          if(sw[i].dim.lvl <=1000){          
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);
            mcp.digitalWrite(LED2_CTRL_PIN,HIGH);
            vtg = map(sw[i].dim.lvl, 1, 1000, 50, 2500);            
            dac.voutWrite(1, vtg);
          }
          
          //else{         
            //mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            //mcp.digitalWrite(LED2_CTRL_PIN,LOW);
            //dac.voutWrite(1, 0);     
          //}
          
          Serial.println("handle_switches fan toggle false, state on, dim.lvl_chg true");
          sendManualResponse("Comp0",1,sw[i].dim.lvl);
          //vtg = map(sw[i].dim.lvl, 1, 1024, 1, 3300);          
        }
        
        //Serial.print("\n");   
        //Serial.print(sw[i].dim.lvl);
        //Serial.print("\t");   
        //Serial.println(vtg);                    
             
          break;
      case 2: //LIGHT ON/OFF                    
        //Serial.println("SW3 Toggled...");
        if(sw[i].toggled==TRUE)
        { 
          sw[i].toggled=FALSE;        
          if(sw[i].state==OFF)
          { 
            sw[i].state=ON;                              
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);
            mcp.digitalWrite(LED3_CTRL_PIN,HIGH);
            Serial.println("handle_switches on/off light toggle true, state off");
            sendManualResponse("Comp2",1,sw[i].dim.lvl);
          }
          else
          {
            sw[i].state=OFF;                    
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED3_CTRL_PIN,LOW);
            Serial.println("handle_switches on/off light toggle true, state on");
            sendManualResponse("Comp2",0,sw[i].dim.lvl);
          }
        }
        break;  
      }//switch            
  }//for
}
*/

void handle_switches()
{
  uint16_t vtg=0;  
  for(int i=0;i<NUM_SW;i++)
  {    
    switch(i)
    {
      case 0: //Dimmable Lamp
        if(sw[i].toggled==TRUE)
        {           
          //Serial.println("SW0 Toggled...");       
          sw[i].toggled=FALSE;                            
          if(sw[i].state==OFF && sw[i].dim.lvl <= 1024)
          {
            sw[i].state=ON;        
            if(sw[i].dim.lvl > 950)                            
              sw[i].dim.lvl=950;
            vtg = map(sw[i].dim.lvl, 1, 950, 10, 3050);            
            dac.voutWrite(0, vtg);            
            delay(5);
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);            
            mcp.digitalWrite(LED1_CTRL_PIN,HIGH);        

            Serial.print("\n");   
        Serial.print(sw[i].dim.lvl);
        Serial.print("\t");   
        Serial.println(vtg);        
            //Serial.println("handle_switches case dimlight toggle true, state off, dim.lvl <= 1024");
            //prevMilli = millis();
            sendManualResponse("Comp1",1,sw[i].dim.lvl);
            //Serial.print("Time taken for sendManualResponse: ");
            //Serial.print(millis() - prevMilli);
            //Serial.println();

          }
          else if(sw[i].state==ON)         
          {
            sw[i].state=OFF;            
            
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED1_CTRL_PIN,LOW);
            dac.voutWrite(0, 0);                                                      
            //Serial.println("handle_switches dimlight toggle true, state on");
            //prevMilli = millis();
            sendManualResponse("Comp1",0,sw[i].dim.lvl);                                                      
            //Serial.print("Time taken for sendManualResponse: ");
            //Serial.print(millis() - prevMilli);
            //Serial.println();

          }
        }
        if(sw[i].state==ON && (sw[i].dim.h_lvl_chg==TRUE || sw[i].dim.s_lvl_chg==TRUE) )         
        {           
          Serial.println("SW0 LVL Changed..."); 
          /*
          if(sw[i].dim.s_lvl_chg==TRUE)
            sw[i].dim.s_lvl_chg=FALSE;
          else
            sw[i].dim.h_lvl_chg=FALSE;
           */ 
          if(sw[i].dim.h_lvl_chg==TRUE)
            sw[i].dim.h_lvl_chg=FALSE;
          if(sw[i].dim.s_lvl_chg==TRUE)
            sw[i].dim.s_lvl_chg=FALSE;
            
          if(sw[i].dim.lvl <=1024){          
            if(sw[i].dim.lvl > 950)                            
              sw[i].dim.lvl=950;
            
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);
            mcp.digitalWrite(LED1_CTRL_PIN,HIGH);                        
            vtg = map(sw[i].dim.lvl, 1, 950, 10, 3050);            
            dac.voutWrite(0, vtg);                        
          }
         /* else{         
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED1_CTRL_PIN,LOW);
            dac.voutWrite(0, 0);     
          }*/
          //Serial.println("handle_switches dimlight toggle false, state on, dim.lvl_chg true");
          //prevMilli = millis();
          sendManualResponse("Comp1",1,sw[i].dim.lvl);
          //Serial.print("Time taken for sendManualResponse: ");
          //Serial.print(millis() - prevMilli);
          //Serial.println();
          
        }       
        /*
        Serial.print("\n");   
        Serial.print(sw[i].dim.lvl);
        Serial.print("\t");   
        Serial.println(vtg);        
        */
        break;

      case 1: //  FAN
        if(sw[i].toggled==TRUE)
        {           
          //Serial.println("SW1 Toggled...");       
          sw[i].toggled=FALSE;                  
          if(sw[i].state==OFF && sw[i].dim.lvl <= 1024)
          {
            sw[i].state=ON;                        
            if(sw[i].dim.lvl > 950)                            
              sw[i].dim.lvl=950;
            vtg = map(sw[i].dim.lvl, 1, 950, 50, 2500);            
            dac.voutWrite(1, vtg);            
            delay(5);
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);            
            mcp.digitalWrite(LED2_CTRL_PIN,HIGH);                           
            //Serial.println("handle_switches case fan toggle true, state off, dim.lvl <= 1024");
            //prevMilli = millis();
            sendManualResponse("Comp0",1,sw[i].dim.lvl);
            //Serial.print("Time taken for sendManualResponse: ");
            //Serial.print(millis() - prevMilli);
            //Serial.println();
          }
          else if(sw[i].state==ON)         
          {
            sw[i].state=OFF;            
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED2_CTRL_PIN,LOW);
            dac.voutWrite(1, 0);                                                      
            //Serial.println("handle_switches fan toggle true, state on");         
            //prevMilli = millis();
            sendManualResponse("Comp0",0,sw[i].dim.lvl);                                             
            //Serial.print("Time taken for sendManualResponse: ");
            //Serial.print(millis() - prevMilli);
            //Serial.println();
          }
        }
        if(sw[i].state==ON && (sw[i].dim.h_lvl_chg==TRUE || sw[i].dim.s_lvl_chg==TRUE) )         
        {           
          Serial.println("SW1 LVL Changed..."); 
          if(sw[i].dim.s_lvl_chg==TRUE)
            sw[i].dim.s_lvl_chg=FALSE;
          else
            sw[i].dim.h_lvl_chg=FALSE;
            
          if(sw[i].dim.lvl <=1024){          
            if(sw[i].dim.lvl > 950)                            
              sw[i].dim.lvl=950;
            
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);
            mcp.digitalWrite(LED2_CTRL_PIN,HIGH);
            vtg = map(sw[i].dim.lvl, 1, 950, 50, 2500);            
            dac.voutWrite(1, vtg);
          }
          /*else{         
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED2_CTRL_PIN,LOW);
            dac.voutWrite(1, 0);     
          }*/          
          //Serial.println("handle_switches fan toggle false, state on, dim.lvl_chg true");
          //prevMilli = millis();
          sendManualResponse("Comp0",1,sw[i].dim.lvl);
          //Serial.print("Time taken for sendManualResponse: ");
          //Serial.print(millis() - prevMilli);
          //Serial.println();
        }
        /*
        Serial.print("\n");   
        Serial.print(sw[i].dim.lvl);
        Serial.print("\t");   
        Serial.println(vtg); 
        */     
          break;
      case 2: //LIGHT ON/OFF                    
        //Serial.println("SW3 Toggled...");
        if(sw[i].toggled==TRUE)
        { 
          sw[i].toggled=FALSE;        
          if(sw[i].state==OFF)
          { 
            sw[i].state=ON;                              
            mcp.digitalWrite(sw[i].ctrl_pin,HIGH);
            mcp.digitalWrite(LED3_CTRL_PIN,HIGH);
            Serial.println("handle_switches on/off light toggle true, state off");
            //prevMilli = millis();
            sendManualResponse("Comp2",1,sw[i].dim.lvl);
            //Serial.print("Time taken for sendManualResponse: ");
            //Serial.print(millis() - prevMilli);
            //Serial.println();
          }
          else
          {
            sw[i].state=OFF;                    
            mcp.digitalWrite(sw[i].ctrl_pin,LOW);
            mcp.digitalWrite(LED3_CTRL_PIN,LOW);
            Serial.println("handle_switches on/off light toggle true, state on");
            //prevMilli = millis();
            sendManualResponse("Comp2",0,sw[i].dim.lvl);
            //Serial.print("Time taken for sendManualResponse: ");
            //Serial.print(millis() - prevMilli);
            //Serial.println();
          }
        }
        break;  
      }//switch            
  }//for
}
//test code begin need to be removed
void light(String state) {
  Serial.println("[light] " + state);
  if (state == "\"state\":true") {
    Serial.println("[light] ON");
    digitalWrite(2, HIGH);
  }
  else {
    Serial.println("[light] off");
    digitalWrite(2, LOW);
  }
}
//test code end need to be removed

void loop()
{
  //prevMilli = millis();
  read_switches();
  //Serial.print("Time taken for read_switches: ");
  //Serial.print(millis() - prevMilli);
  //Serial.println();
  //prevMilli = millis();
  CommandLoop();
  //Serial.print("Time taken for CommandLoop: ");
  //Serial.print(millis() - prevMilli);
  //Serial.println();
  //if (isClient)
    //deviceServerLoop();
  //prevMilli = millis();    
  //socketLoop();
  handle_switches();
  //Serial.print("Time taken for handle_switches: ");
  //Serial.print(millis() - prevMilli);
  //Serial.println();
  //if (isClient)
    //pingServer();
  delay(10);
}

void printStatus()
{
  Serial.println("NAME     Vref  Gain  PowerDown  Value");
  for (int channel=0; channel <= 3; channel++)
  { 
    Serial.print("DAC");
    Serial.print(channel,DEC);
    Serial.print("   ");
    Serial.print("    "); 
    Serial.print(dac.getVref(channel),BIN);
    Serial.print("     ");
    Serial.print(dac.getGain(channel),BIN);
    Serial.print("       ");
    Serial.print(dac.getPowerDown(channel),BIN);
    Serial.print("       ");
    Serial.println(dac.getValue(channel),DEC);

    Serial.print("EEPROM");
    Serial.print(channel,DEC);
    Serial.print("    "); 
    Serial.print(dac.getVrefEp(channel),BIN);
    Serial.print("     ");
    Serial.print(dac.getGainEp(channel),BIN);
    Serial.print("       ");
    Serial.print(dac.getPowerDownEp(channel),BIN);
    Serial.print("       ");
    Serial.println(dac.getValueEp(channel),DEC);
  }
  Serial.println(" ");
}
///////////// Vadiraj code end ///////////////////////////////////////

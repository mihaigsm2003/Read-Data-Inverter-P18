/*************************************************************************************
/* ALl credits for some bits from https://github.com/scottwday/InverterOfThings
/* And i have trashed alot of his code, rewritten some. So thanks to him and credits to him. 
/* Changes by softwarecrash
/*************************************************************************************/
#define SERIALDEBUG
//#include <Wire.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <NTPClient.h>

#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ESP8266HTTPClient.h> //for update
#include <ESP8266httpUpdate.h>  //for update


const int PIN_LED = 5;  // COM
const int PIN_LED2 = 0; // SRV
const int PIN_LED4 = 4; // NET

#include "inverter.h"
#include "Settings.h"

#include "webpages/HTMLcase.h"     //The HTML Konstructor
#include "webpages/main.h"         //landing page with menu
#include "webpages/settings.h"     //settings page
#include "webpages/settingsedit.h" //mqtt settings page
//#include "webservers.h"            //excluded web servers

WiFiClient client;

Settings _settings;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

PubSubClient mqttclient(client);

extern QpigsMessage _qpigsMessage;
extern QpigsnMessage _qpigsnMessage;
extern QmodMessage _qmodMessage;
extern QpiriMessage _qpiriMessage;
extern QpiwsMessage _qpiwsMessage; // STATUS INVERTOR , verifica lista in inverter, Bool types
extern QtMessage _qtMessage;
extern QdatMessage _qdatMessage;
String currentDate;

extern QRaw _qRaw;
//extern QAv _qAv;

#define ESP_getChipId()   (ESP.getChipId())
String topic = "ESP_" + String ESP_getChipId(); //Default first part of topic. We will add device ID in setup
const char* mqttmod = "Waiting for Connection";

unsigned long mqtttimer = 0;

AsyncWebServer server(80);

DNSServer dns;

//flag for saving data
bool shouldSaveConfig = false;
char mqtt_server[40];
bool restartNow = false;
bool askInverterOnce = true;
bool valChange = false;
String commandFromWeb;

#define FIRMWARE_VERSION "0.1"   // Version for update

//----------------------------------------------------------------------
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!index)
  {
    Serial.println("Update");
    Update.runAsync(true);
    if (!Update.begin(free_space))
    {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
  }

  if (final)
  {
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device is booting new Firmware");
      response->addHeader("Refresh", "10; url=/");
      response->addHeader("Connection", "close");
      request->send(response);

      restartNow = true; //Set flag so main loop can issue restart call
      Serial.println("Update complete");
    }
  }
}

void setup()
{
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED2, LOW);
  pinMode(PIN_LED4, OUTPUT);
  digitalWrite(PIN_LED4, LOW);
  
  _settings.load();
  delay(1000);
  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_LED2, HIGH);
  digitalWrite(PIN_LED4, HIGH);
  WiFi.persistent(true); //fix wifi save bug

  AsyncWiFiManager wm(&server, &dns);

  //muss dann wieder weg
  wm.setDebugOutput(false);

#ifdef SERIALDEBUG
  wm.setDebugOutput(false);
#endif
  wm.setSaveConfigCallback(saveConfigCallback);

//  Wire.begin(4, 5);
#ifdef SERIALDEBUG
  Serial1.begin(9600); // Debugging towards UART1
#endif
  Serial.begin(2400); // Using UART0 for comm with inverter. IE cant be connected during flashing
  //Serial.setTimeout(100);

#ifdef SERIALDEBUG
  Serial1.println();
  Serial1.printf("Device Name:\t");
  Serial1.println(_settings._deviceName);
  Serial1.printf("Mqtt Server:\t");
  Serial1.println(_settings._mqttServer);
  Serial1.printf("Mqtt Port:\t");
  Serial1.println(_settings._mqttPort);
  Serial1.printf("Mqtt User:\t");
  Serial1.println(_settings._mqttUser);
  Serial1.printf("Mqtt Passwort:\t");
  Serial1.println(_settings._mqttPassword);
  Serial1.printf("Mqtt Interval:\t");
  Serial1.println(_settings._mqttRefresh);
  Serial1.printf("Mqtt Topic:\t");
  Serial1.println(_settings._mqttTopic);
#endif
  //create custom wifimanager fields
/*
  AsyncWiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Password", NULL, 100);
  AsyncWiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", NULL, 30);
  AsyncWiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", NULL, 6);
  AsyncWiFiManagerParameter custom_mqtt_refresh("mqtt_refresh", "MQTT Send Interval", NULL, 4);
  AsyncWiFiManagerParameter custom_device_name("device_name", "Device Name", NULL, 40);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_refresh);
  wm.addParameter(&custom_device_name);
*/
  bool res = wm.autoConnect("Solar-AP");

  wm.setConnectTimeout(30);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds

  //save settings if wifi setup is fire up
  if (shouldSaveConfig)
  {
 /*   _settings._mqttServer = custom_mqtt_server.getValue();
    _settings._mqttUser = custom_mqtt_user.getValue();
    _settings._mqttPassword = custom_mqtt_pass.getValue();
    _settings._mqttPort = atoi(custom_mqtt_port.getValue());
    _settings._deviceName = custom_device_name.getValue();
    _settings._mqttTopic = custom_mqtt_topic.getValue();
    _settings._mqttRefresh = atoi(custom_mqtt_refresh.getValue());
*/
    _settings.save();
    delay(500);
    _settings.load();
    ESP.restart();
  }
// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  //GMT+2 = 7200
  timeClient.setTimeOffset(7200);
  
  mqttclient.setServer(_settings._mqttServer.c_str(), _settings._mqttPort);

  mqttclient.setCallback(callback);

  //check is WiFi connected
  if (!res)
  {
#ifdef SERIALDEBUG
    Serial1.println("Failed to connect or hit timeout");
#endif
digitalWrite(PIN_LED4, HIGH);
  }
  else
  {
    //set the device name
    MDNS.begin(_settings._deviceName);
    WiFi.hostname(_settings._deviceName);
    digitalWrite(PIN_LED4, LOW);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_MAIN);
                response->printf_P(HTML_FOOT);
                request->send(response);
              });
    server.on("/livejson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                DynamicJsonDocument liveJson(1024);
                liveJson["gridV"] = _qpigsMessage.gridV;
                liveJson["gridHz"] = _qpigsMessage.gridHz;
                liveJson["acOutV"] = _qpigsMessage.acOutV;
                liveJson["acOutHz"] = _qpigsMessage.acOutHz;
                liveJson["acOutVa"] = _qpigsMessage.acOutVa;
                liveJson["acOutW"] = _qpigsMessage.acOutW;
                liveJson["acOutPercent"] = _qpigsMessage.acOutPercent;
                liveJson["heatSinkDegC"] = _qpigsMessage.heatSinkDegC;
                liveJson["battV"] = _qpigsMessage.battV;
                liveJson["battPercent"] = _qpigsMessage.battPercent;
                liveJson["battChargeA"] = _qpigsMessage.battChargeA;
                liveJson["battDischargeA"] = _qpigsMessage.battDischargeA;
                liveJson["sccBattV"] = _qpigsMessage.battV_SCC;
                liveJson["solarV_PV1"] = _qpigsMessage.solarV_PV1;
                liveJson["solarW_PV1"] = _qpigsMessage.solarW_PV1;
                liveJson["solarV_PV2"] = _qpigsMessage.solarV_PV2;
                liveJson["solarW_PV2"] = _qpigsMessage.solarW_PV2;
                liveJson["cSOC"] = _qpigsMessage.battDirection;
                liveJson["iv_mode"] = _qmodMessage.operationMode;
                liveJson["mqttmode"] = mqttmod;
                liveJson["device_name"] = _settings._deviceName;
                serializeJson(liveJson, *response);
                request->send(response);
              });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device reboots...");
                response->addHeader("Refresh", "15; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                restartNow = true;
              });
    server.on("/confirmreset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_CONFIRM_RESET);
                response->printf_P(HTML_FOOT);
                request->send(response);
              });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Device is Erasing...");
                response->addHeader("Refresh", "15; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                delay(1000);
                _settings.reset();
                ESP.eraseConfig();
                ESP.restart();
              });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS);
                response->printf_P(HTML_FOOT);
                request->send(response);
              });

    server.on("/settingsedit", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS_EDIT);
                response->printf_P(HTML_FOOT);
                request->send(response);
              });

    server.on("/settingsjson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                DynamicJsonDocument SettingsJson(256);
                SettingsJson["device_name"] = _settings._deviceName;
                SettingsJson["mqtt_server"] = _settings._mqttServer;
                SettingsJson["mqtt_port"] = _settings._mqttPort;
                SettingsJson["mqtt_topic"] = _settings._mqttTopic;
                SettingsJson["mqtt_user"] = _settings._mqttUser;
                SettingsJson["mqtt_password"] = _settings._mqttPassword;
                SettingsJson["mqtt_refresh"] = _settings._mqttRefresh;
                serializeJson(SettingsJson, *response);
                request->send(response);
              });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                request->redirect("/settings");
                _settings._mqttServer = request->arg("post_mqttServer");
                _settings._mqttPort = request->arg("post_mqttPort").toInt();
                _settings._mqttUser = request->arg("post_mqttUser");
                _settings._mqttPassword = request->arg("post_mqttPassword");
                _settings._mqttTopic = request->arg("post_mqttTopic");
                _settings._mqttRefresh = request->arg("post_mqttRefresh").toInt();
                _settings._deviceName = request->arg("post_deviceName");
                Serial.print(_settings._mqttServer);
                _settings.save();
                delay(500);
                _settings.load();
              });
/*
    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebParameter *p = request->getParam(0);
                if (p->name() == "maxcharge")
                {
                  valChange = true;
                  _qpiriMessage.battMaxChrgA = p->value().toInt(); //const string zu int
                }
                if (p->name() == "maxaccharge")
                {
                  valChange = true;
                  _qpiriMessage.battMaxAcChrgA = p->value().toInt(); //const string zu int
                }
                request->send(200, "text/plain", "message received");
              });
*/
    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
          request->send(200);
          request->redirect("/");
        },
        handle_update_progress_cb);

    server.begin();
    MDNS.addService("http", "tcp", 80);
#ifdef SERIALDEBUG
    Serial1.println("Webserver Running...");
#endif
  }
topic.toUpperCase();  //face topic cu litere mari
  server.begin();
  if (!mqttclient.connected())
   mqttmod = "Not Connected";
   digitalWrite(PIN_LED2, HIGH);
   mqttclient.connect(topic.c_str(), _settings._mqttUser.c_str(), _settings._mqttPassword.c_str());
  if (mqttclient.connect(topic.c_str()))
  {
     mqttmod = "Connected to MQTT";
     digitalWrite(PIN_LED2, LOW);
    mqttclient.subscribe((String(topic) + String("/Device Data/Current max AC charging current")).c_str());
    mqttclient.subscribe((String(topic) + String("/Device Data/Current max charging current")).c_str());
  }
}
//end void setup

//----------------------------------------------------------------------
//-------------------------LOOP---------------------------------------------
//----------------------------------------------------------------------
void loop()
{
  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  { //No use going to next step unless WIFI is up and running.
    MDNS.update();
    mqttclient.loop(); // Check if we have something to read from MQTT
      
// Internet Time      
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

String Hours = timeClient.getHours() < 10 ? "0" + String(timeClient.getHours()) : String(timeClient.getHours());
String Minutes = timeClient.getMinutes() < 10 ? "0" + String(timeClient.getMinutes()) : String(timeClient.getMinutes());
String Seconds = timeClient.getSeconds() < 10 ? "0" + String(timeClient.getSeconds()) : String(timeClient.getSeconds());
String Days = ptm->tm_mday < 10 ? "0" + String(ptm->tm_mday) : String(ptm->tm_mday);
String Month = ptm->tm_mon+1 < 10 ? "0" + String(ptm->tm_mon+1) : String(ptm->tm_mon+1);


  currentDate = String(ptm->tm_year-100) + String(Month) + String(Days)+ String(Hours)+ String(Minutes)+ String(Seconds);
  //End Internet time

    
      digitalWrite(PIN_LED, LOW);
      requestInverter(QMOD);
      digitalWrite(PIN_LED, HIGH);
      requestInverter(QPIGS);
      digitalWrite(PIN_LED, LOW);
      requestInverter(QPIWS);
      digitalWrite(PIN_LED, HIGH);
      requestInverter(QPIRI); 
      digitalWrite(PIN_LED, LOW);
      requestInverter(QT); 
      digitalWrite(PIN_LED, HIGH);
      if (_qpigsMessage.Parallel_ID == 1)
      {
        requestInverter(QPIGSn); // run only if parallel connection
      digitalWrite(PIN_LED, LOW);
      }


    if (askInverterOnce) //ask the inverter once to get static data
    {
      requestInverter(QPI); //just send for clear out the serial puffer and resolve the first NAK
      requestInverter(QPI);
      //requestInverter(QPIRI);
      //requestInverter(QMCHGCR);
      //requestInverter(QMUCHGCR);
      requestInverter(DAT);

      ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.setLedPin(PIN_LED, LOW);

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://pro.arenax.ro/firmware/02/p18update.bin");
      
      askInverterOnce = false;
    }
    sendtoMQTT(); // Update data to MQTT server if we should
  }
  if (restartNow)
  {
    delay(1000);
    Serial.println("Restart");
    ESP.restart();
  }

  yield();
}
//End void loop

bool sendtoMQTT()
{

  if (millis() < (mqtttimer + (_settings._mqttRefresh * 1000)) || _settings._mqttRefresh == 0) //its save for rollover?
  {
    return false;
  }
  mqtttimer = millis();
  if (!mqttclient.connected())
 {
     mqttmod = "Not connected";
     digitalWrite(PIN_LED2, HIGH);
    if (mqttclient.connect(topic.c_str(), _settings._mqttUser.c_str(), _settings._mqttPassword.c_str()))
    {
#ifdef SERIALDEBUG
      Serial1.println(F("Reconnected to MQTT SERVER"));
        mqttmod = "Reconnected to MQTT SERVER";
        digitalWrite(PIN_LED2, LOW);
#endif
      mqttclient.publish((topic + String("/Device Data/IP")).c_str(), String(WiFi.localIP().toString()).c_str());
    }
    else
    {
#ifdef SERIALDEBUG
      Serial1.println(F("CANT CONNECT TO MQTT"));
      mqttmod = "CANT CONNECT TO MQTT";
#endif
      return false; // Exit if we couldnt connect to MQTT brooker
    }
  }
#ifdef SERIALDEBUG
  Serial1.println(F("Data sent to MQTT Server"));
#endif
  //mqttclient.subscribe((String(topic) + String("/Device Data/Current max AC charging current")).c_str());
  //mqttclient.subscribe((String(topic) + String("/Device Data/Current max charging current")).c_str());

  //qpigs
  mqttclient.publish((String(topic) + String("/Grid Voltage")).c_str(), String(_qpigsMessage.gridV).c_str());
  mqttclient.publish((String(topic) + String("/Grid Frequenz")).c_str(), String(_qpigsMessage.gridHz).c_str());
  mqttclient.publish((String(topic) + String("/AC out Voltage")).c_str(), String(_qpigsMessage.acOutV).c_str());
  mqttclient.publish((String(topic) + String("/AC out Frequenz")).c_str(), String(_qpigsMessage.acOutHz).c_str());
  mqttclient.publish((String(topic) + String("/AC out VA")).c_str(), String(_qpigsMessage.acOutVa).c_str());
  mqttclient.publish((String(topic) + String("/AC out Watt")).c_str(), String(_qpigsMessage.acOutW).c_str());
  mqttclient.publish((String(topic) + String("/AC out percent")).c_str(), String(_qpigsMessage.acOutPercent).c_str());
  mqttclient.publish((String(topic) + String("/Battery Voltage")).c_str(), String(_qpigsMessage.battV).c_str());
  mqttclient.publish((String(topic) + String("/Battery SCC1")).c_str(), String(_qpigsMessage.battV_SCC).c_str()); 
  mqttclient.publish((String(topic) + String("/Battery SCC2")).c_str(), String(_qpigsMessage.battV_SCC_2).c_str());
  mqttclient.publish((String(topic) + String("/Battery Charge A")).c_str(), String(_qpigsMessage.battChargeA).c_str());
  mqttclient.publish((String(topic) + String("/Battery Discharge A")).c_str(), String(_qpigsMessage.battDischargeA).c_str());
  mqttclient.publish((String(topic) + String("/Battery Percent")).c_str(), String(_qpigsMessage.battPercent).c_str()); 
  mqttclient.publish((String(topic) + String("/Bus Temp")).c_str(), String(_qpigsMessage.heatSinkDegC).c_str());
  mqttclient.publish((String(topic) + String("/PV Volt")).c_str(), String(_qpigsMessage.solarV_PV1).c_str());
  mqttclient.publish((String(topic) + String("/PV Watt")).c_str(), String(_qpigsMessage.solarW_PV1).c_str());
  mqttclient.publish((String(topic) + String("/PV Volt2")).c_str(), String(_qpigsMessage.solarV_PV2).c_str());
  mqttclient.publish((String(topic) + String("/PV Watt2")).c_str(), String(_qpigsMessage.solarW_PV2).c_str());
  mqttclient.publish((String(topic) + String("/Battery Direction")).c_str(), String(_qpigsMessage.battDirection).c_str());
  mqttclient.publish((String(topic) + String("/Parallel ID")).c_str(), String(_qpigsMessage.Parallel_ID).c_str());
  //qmod
  mqttclient.publish((String(topic) + String("/Inverter Operation Mode")).c_str(), String(_qmodMessage.operationMode).c_str());
  mqttclient.publish((String(topic) + String("/Inverter Operation Mode RAW")).c_str(), String(_qmodMessage.operationMode_RAW).c_str());
  //piri
  mqttclient.publish((String(topic) + String("/Device Data/Grid rating voltage")).c_str(), String(_qpiriMessage.gridRatingV).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Grid rating current")).c_str(), String(_qpiriMessage.gridRatingA).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/AC output rating voltage")).c_str(), String(_qpiriMessage.acOutRatingV).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/AC output rating frequency")).c_str(), String(_qpiriMessage.acOutRatingHz).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/AC output rating current")).c_str(), String(_qpiriMessage.acOutRatingA).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/AC output rating apparent power")).c_str(), String(_qpiriMessage.acOutRatungVA).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/AC output rating active power")).c_str(), String(_qpiriMessage.acOutRatingW).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Battery rating voltage")).c_str(), String(_qpiriMessage.battRatingV).c_str());

  mqttclient.publish((String(topic) + String("/Device Data/Battery re-charge voltage")).c_str(), String(_qpiriMessage.battreChargeV).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Battery under voltage")).c_str(), String(_qpiriMessage.battUnderV).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Battery bulk voltage")).c_str(), String(_qpiriMessage.battBulkV).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Battery float voltage")).c_str(), String(_qpiriMessage.battFloatV).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Battery type")).c_str(), String(_qpiriMessage.battType).c_str());

  mqttclient.publish((String(topic) + String("/Device Data/Current max AC charging current")).c_str(), String(_qpiriMessage.battMaxAcChrgA).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Current max charging current")).c_str(), String(_qpiriMessage.battMaxChrgA).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/IP")).c_str(), String(WiFi.localIP().toString()).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Time String")).c_str(), String(currentDate).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Time Invert")).c_str(), String(_qtMessage.qt).c_str());
  mqttclient.publish((String(topic) + String("/Device Data/Dat Response")).c_str(), String(_qdatMessage.dt).c_str());
  


  //piws /*Adaugat de SILVIU  linia de mai jos*/ 
  mqttclient.publish((String(topic) + String("/Device Data/Inverter fault")).c_str(), String(_qpiwsMessage.faultCode).c_str());
  
//RAW Messages from Invberter
#ifdef MQTTDEBUG
  mqttclient.publish((String(topic) + String("/RAW/QPIGS")).c_str(), String(_qRaw.QPIGS).c_str());
  mqttclient.publish((String(topic) + String("/RAW/QPIGSn")).c_str(), String(_qRaw.QPIGSn).c_str());
  mqttclient.publish((String(topic) + String("/RAW/QPIRI")).c_str(), String(_qRaw.QPIRI).c_str());
  mqttclient.publish((String(topic) + String("/RAW/QMOD")).c_str(), String(_qRaw.QMOD).c_str());
  mqttclient.publish((String(topic) + String("/RAW/QPIWS")).c_str(), String(_qRaw.QPIWS).c_str());
  mqttclient.publish((String(topic) + String("/RAW/QFLAG")).c_str(), String(_qRaw.QFLAG).c_str());
  mqttclient.publish((String(topic) + String("/RAW/QID")).c_str(), String(_qRaw.QID).c_str());
  mqttclient.publish((String(topic) + String("/RAW/QPI")).c_str(), String(_qRaw.QPI).c_str());

#endif

  return true;
}

void callback(char *top, byte *payload, unsigned int length)
{
  String messageTemp;
  for (unsigned int i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }
  //modify the max charging current
  if (strcmp(top, (topic + "/Device Data/Current max charging current").c_str()) == 0)
  {
    Serial1.println("message recived");
    if (messageTemp.toInt() != _qpiriMessage.battMaxChrgA)
    {
      _qpiriMessage.battMaxChrgA = messageTemp.toInt();
      valChange = true;
    }
  }
  //modify the max ac charging current
  if (strcmp(top, (topic + "/Device Data/Current max AC charging current").c_str()) == 0)
  {
    Serial1.println("message recived");
    if (messageTemp.toInt() != _qpiriMessage.battMaxAcChrgA)
    {
      _qpiriMessage.battMaxAcChrgA = messageTemp.toInt();
      valChange = true;
    }
  }
}

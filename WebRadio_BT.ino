                  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                 //   Code by Tim, for use with ESP32 Wroom32 Dev Kit V1 (38pin)                                                              //
                //    Update / hack of 80's Radio Tape player 'Cascade T-26' for use as web radio and Bluetooth speaker.                     //
               //     Connects ESP32 to MAX 98357A module from Adafruit via I2S.                                                            //
              //      Makes use of 'Cascade T-26' slide switch for selection of BT speaker (FM), web radio (AM) or tape,                   // 
             //           potentiometer 'tuning dial' for web radio selection and potentiometer for volume in software for web radio.     //
            //        Code almagamated from various sources.                                                                             //
           //         BT Library https://github.com/pschatzmann/ESP32-A2DP                                                              //
          //          ESP32-audioI2S Library - https://github.com/schreibfaul1/ESP32-audioI2S                                          //
         //           Use board ESP Dev Module and choose tools > partitions > Huge APP                                               //
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
         
/////////////////////////////////////////////////////////// Web Radio stuff ////////

// Include required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include "Audio.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "BluetoothA2DPSink.h"

// Little FS initialiation
#define FORMAT_LITTLEFS_IF_FAILED true
// Define I2S connections
#define I2S_DOUT  25
#define I2S_BCLK  26
#define I2S_LRC   27

// for the radio loop counter and wifi connect timer
const long rdinterval = 2000;
unsigned long rdpreviousMillis = 0;
const long wifiinterval = 10000; // interval to wait for Wi-Fi connection (milliseconds)
unsigned long wifipreviousMillis = 0;

// control variables
int radioStation;
int previousRadioStation;
int vol;
int previousVolume;

// pot pins
const int radioPin = 34;
const int volumePin = 39;           // alias as VN or SN on ESP32 dev board

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "station1name";
const char *PARAM_INPUT_4 = "station1url";
const char *PARAM_INPUT_5 = "station2name";
const char *PARAM_INPUT_6 = "station2url";
const char *PARAM_INPUT_7 = "station3name";
const char *PARAM_INPUT_8 = "station3url";
const char *PARAM_INPUT_9 = "station4name";
const char *PARAM_INPUT_10 = "station4url";
const char *PARAM_INPUT_11 = "station5name";
const char *PARAM_INPUT_12 = "station5url";
const char *PARAM_INPUT_13 = "station6name";
const char *PARAM_INPUT_14 = "station6url";
const char *PARAM_INPUT_15 = "station7name";
const char *PARAM_INPUT_16 = "station7url";
const char *PARAM_INPUT_17 = "station8name";
const char *PARAM_INPUT_18 = "station8url";
const char *PARAM_INPUT_19 = "station9name";
const char *PARAM_INPUT_20 = "station9url";
const char *PARAM_INPUT_21 = "station10name";
const char *PARAM_INPUT_22 = "station10url";
const char *PARAM_INPUT_23 = "deleteWifiCredentials";

//Variables to save values from HTML form
String ssid;
String pass;

// These are written over on the first read of the files which fails, the read function returns an empty string
String stationName[10] = {"Station1","Station2","Station3","Station4","Station5","Station6","Station7","Station8","Station9","Station10"};
String stationList[10] = {"URL","URL","URL","URL","URL","URL","URL","URL","URL","URL"};

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";

const char *stationNamePath[10] = {"/station1name.txt","/station2name.txt","/station3name.txt","/station4name.txt","/station5name.txt",
                                  "/station6name.txt","/station7name.txt","/station8name.txt","/station9name.txt","/station10name.txt"};
                                  
const char *stationURLPath[10] = {"/station1URL.txt","/station2URL.txt","/station3URL.txt","/station4URL.txt","/station5URL.txt",
                                  "/station6URL.txt","/station7URL.txt","/station8URL.txt","/station9URL.txt","/station10URL.txt"};

int numberofStations = 10;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create audio object
Audio audio;

// Create BT object
BluetoothA2DPSink a2dp_sink;
int btPin = 36;  // SP

//////////////////// FUNCTIONS ///////////////////
void initLittleFS() {
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
      Serial.println("LittleFS Mount Failed");
      return;
   }
   else{
       Serial.println("Little FS Mounted Successfully");
   }
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
    
  Serial.print("- file content: ");
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces html placeholder with names and urls from saved stations -  it doesn't display anything after a space in html form, so I remove all spaces in html form submit!
String processor(const String &var) {
  if(var == "SSID") {
    return ssid;
  }
  if(var == "PASS") {
    return pass;
  }
  if(var == "STAT1NAME") {
    return (stationName[0]);
  }
  if(var == "STAT1URL") {
    return (stationList[0]);
  }
  if(var == "STAT2NAME") {
    return (stationName[1]);
  }
  if(var == "STAT2URL") {
    return (stationList[1]);
  }
  if(var == "STAT3NAME") {
    return (stationName[2]);
  }
  if(var == "STAT3URL") {
    return (stationList[2]);
  }
  if(var == "STAT4NAME") {
    return (stationName[3]);
  }
  if(var == "STAT4URL") {
    return (stationList[3]);
  }
  if(var == "STAT5NAME") {
    return (stationName[4]);
  }
  if(var == "STAT5URL") {
    return (stationList[4]);
  }
  if(var == "STAT6NAME") {
    return (stationName[5]);
  }
  if(var == "STAT6URL") {
    return (stationList[5]);
  }
  if(var == "STAT7NAME") {
    return (stationName[6]);
  }
  if(var == "STAT7URL") {
    return (stationList[6]);
  }
  if(var == "STAT8NAME") {
    return (stationName[7]);
  }
  if(var == "STAT8URL") {
    return (stationList[7]);
  }
  if(var == "STAT9NAME") {
    return (stationName[8]);
  }
  if(var == "STAT9URL") {
    return (stationList[8]);
  }
  if(var == "STAT10NAME") {
    return (stationName[9]);
  }
  if(var == "STAT10URL") {
    return (stationList[9]);
  }
  return String();
}

// Access Point for adding WiFi credentials
void startWifiManager()  {
  // Start access point to configure SSID & pass
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("RADIO-MANAGER", "cascadeT26");  //this is the access point SSID and password

    //Always gives 192.168.4.1 - not worth using mDNS due to compatibility issues
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    //for (int i=0;i<10;i++)  {
    //  Serial.printf("Station name is: %s\n", stationName[i]);
      //Serial.printf("Station url is: %s\n", stationList[i]);
      //Serial.printf("Address of name is: %p\n", stationName[i]);      
    //}
    
    // AP Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html", false, processor);
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/html", "<!DOCTYPE html><head></head><body><h1>Submitted, the radio will restart.<br>Reconnect to the Access Point and return here if there are any problems.</h1></body>");
      delay(3000);
      ESP.restart();
    });
    server.begin(); /// help
}

void startStationManager()  {
  // Start access point to configure SSID & pass
    Serial.println("Starting station manager...");
    //for (int i=0;i<10;i++)  {
    //  Serial.printf("Station name is: %s\n", stationName[i]);
      //Serial.printf("Station url is: %s\n", stationList[i]);
      //Serial.printf("Address of name is: %p\n", stationName[i]);      
    //}
    
    // web server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          String statname;
          String statURL;
          bool deleteWifi;
          // HTTP POST station 1 name value
          if (p->name() == PARAM_INPUT_3) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[0], statname.c_str());  
          }
          // HTTP POST station 1 url value
          if (p->name() == PARAM_INPUT_4) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000"); 
            Serial.print("Station URL set to: ");            
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[0], statURL.c_str());
          }
          // HTTP POST station 2 name value
          if (p->name() == PARAM_INPUT_5) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[1], statname.c_str());  
            }
          // HTTP POST station 2 url value
          if (p->name() == PARAM_INPUT_6) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000"); 
            Serial.print("Station URL set to: ");            
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[1], statURL.c_str());
          }
          // HTTP POST station 3 name value
          if (p->name() == PARAM_INPUT_7) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[2], statname.c_str());  
            }
          // HTTP POST station 3 url value
          if (p->name() == PARAM_INPUT_8) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000"); 
            Serial.print("Station URL set to: ");           
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[2], statURL.c_str());
          }
          // HTTP POST station 4 name value
          if (p->name() == PARAM_INPUT_9) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[3], statname.c_str());  
            }
          // HTTP POST station 4 url value
          if (p->name() == PARAM_INPUT_10) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000");
            Serial.print("Station URL set to: ");           
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[3], statURL.c_str());
          }
          // HTTP POST station 5 name value
          if (p->name() == PARAM_INPUT_11) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[4], statname.c_str());  
            }
          // HTTP POST station 5 url value
          if (p->name() == PARAM_INPUT_12) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000");
            Serial.print("Station URL set to: ");         
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[4], statURL.c_str());
          }
          // HTTP POST station 6 name value
          if (p->name() == PARAM_INPUT_13) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[5], statname.c_str());  
            }
          // HTTP POST station 6 url value
          if (p->name() == PARAM_INPUT_14) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000");
            Serial.print("Station URL set to: ");
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[5], statURL.c_str());
          }
          // HTTP POST station 7 name value
          if (p->name() == PARAM_INPUT_15) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[6], statname.c_str());  
            }
          // HTTP POST station 7 url value
          if (p->name() == PARAM_INPUT_16) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000");
            Serial.print("Station URL set to: ");        
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[6], statURL.c_str());
          }
          // HTTP POST station 8 name value
          if (p->name() == PARAM_INPUT_17) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[7], statname.c_str());  
            }
          // HTTP POST station 8 url value
          if (p->name() == PARAM_INPUT_18) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000");
            Serial.print("Station URL set to: ");         
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[7], statURL.c_str());
          }
          // HTTP POST station 9 name value
          if (p->name() == PARAM_INPUT_19) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[8], statname.c_str());  
            }
          // HTTP POST station 9 url value
          if (p->name() == PARAM_INPUT_20) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000");
            Serial.print("Station URL set to: ");
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[8], statURL.c_str());
          }
          // HTTP POST station 10 name value
          if (p->name() == PARAM_INPUT_21) {
            statname = p->value().c_str();
            Serial.print("Station name set to: ");
            Serial.println(statname);
            // Write file with statname
            writeFile(LittleFS, stationNamePath[9], statname.c_str());  
            }
          // HTTP POST station 10 url value
          if (p->name() == PARAM_INPUT_22) {
            statURL = p->value().c_str();
            statURL.replace("320000","96000");
            statURL.replace("128000","96000");
            Serial.print("Station URL set to: ");
            Serial.println(statURL);
            // Write file to save value
            writeFile(LittleFS, stationURLPath[9], statURL.c_str());
          }  
          // HTTP POST delete saved wifi credentials
          if (p->name() == PARAM_INPUT_23)  {
            deleteWifi = p->value();
            Serial.println(deleteWifi);
            if (deleteWifi == true) {
              Serial.println("Deleting saved Wifi credentials");
              writeFile(LittleFS, ssidPath, "");
              writeFile(LittleFS, passPath, "");
            }
          }       
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/html", "<!DOCTYPE html><head></head><body><h1>Submitted, the radio will restart.</h1></body>");
      delay(3000);
      ESP.restart();
    });
    server.begin(); /// help
}

// BT speaker loop - does not use void loop() has it's own loop and works with I2S
void blueToothPlay() {
  a2dp_sink.start("CascadeT26");
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || pass==""){
    Serial.println("Undefined SSID or password.");
    return false;
  }

  String hostname = "radiomanager";
  WiFi.setHostname(hostname.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  wifipreviousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - wifipreviousMillis >= wifiinterval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

// init MAX module - only used by web radio, and set volume and station
void initMAX()  {
  // Connect MAX98357 I2S Amplifier Module
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Set the volume (0-21)
  vol = map(analogRead(volumePin), 0, 4095, 0, 21);
  audio.setVolume(vol);
  previousVolume = vol;

  // Set the radio station
  radioStation = map(analogRead(radioPin), 0, 4095, 0, numberofStations);  // number of stations in the list.
  audio.connecttohost(stationList[radioStation].c_str());
}

// Start the web radio
void webRadioLoop() {
  while (true)  {
    unsigned long currentMillis = millis();
    
    if(currentMillis - rdpreviousMillis > rdinterval) {
      vol = map(analogRead(volumePin), 0, 4095, 0, 21);
      if (vol != previousVolume) {
        audio.setVolume(vol);
        previousVolume = vol;
      }
      radioStation = map(analogRead(radioPin), 0, 4095, 0, numberofStations);  // also here number of stations in the list.
      if(radioStation!= previousRadioStation)  {
        audio.connecttohost(stationList[radioStation].c_str());
        previousRadioStation = radioStation;
      }
      rdpreviousMillis = currentMillis;
    }
  
    // Run audio player
    audio.loop();
  }
}


// for serial data from webradio
void audio_info(const char *info) {
  Serial.print("info        "); Serial.println(info);
}

////////////////////////////////////////////////// Set Up ////////////////////

void setup() {
  // CPU frequecy, reduce for power saving. Default is 240, does not work below 160
  setCpuFrequencyMhz(240);

  // Serial port for debugging purposes
  Serial.begin(115200);

  initLittleFS();

  // Load values saved in LittleFS
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Password: %s\n", pass);
  // and load the stations from LittleFS
  for (int i=0;i<10;i++) {
    stationName[i] = readFile(LittleFS, stationNamePath[i]);
    Serial.println("stationName: " + stationName[i]);
    stationList[i] = readFile(LittleFS, stationURLPath[i]);
    Serial.println("stationList: " + stationList[i]);
  }

  // BT speaker set up - NB cannot change to 32 bits in i2s config as it fails  
  i2s_pin_config_t my_pin_config = {
      .bck_io_num = 26,
      .ws_io_num = 27,
      .data_out_num = 25,
      .data_in_num = I2S_PIN_NO_CHANGE
  };
  a2dp_sink.set_pin_config(my_pin_config);

  // Switch to BT speaker if btPin is high
  pinMode(btPin, INPUT); // for switching to BT speaker mode
  if (digitalRead(btPin)) {
    blueToothPlay();
  }

// Running station manager in station mode while radio is working
  if (initWiFi()) {
    startStationManager();
    serial.println("Starting web radio");
    webRadioPlay();
  } else  {
    Serial.println("AP started as WIFI details not configured");
    startWifiManager();
  }
}

void loop() {
}

void webRadioPlay() {
  initMAX();
  webRadioLoop();
}

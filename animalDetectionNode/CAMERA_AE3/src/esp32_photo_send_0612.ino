/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-post-image-photo-server/
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "OneM2MClient.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"

//add Tas
#include "TasMOTOR.h"

TasMOTOR tasMotor;

// Wifi
const char* ssid = "Laon_R&D";//"*********";
const char* password = "development";//"*********";

uint8_t USE_WIFI = 1;
WiFiClient espClient; 
PubSubClient mqtt(espClient);
//

// Upload server
String serverName    = "";//photo server
String serverPath    = "/upload";     // The default serverPath should be upload.php
const int serverPort = 3000;          // Server port
// photo config / timelapse
const int timeIntervalInMinutes = 5;
const int timerInterval         = timeIntervalInMinutes*60000; // time between each HTTP POST image
unsigned long previousMillis    = 0;                           // last time image was sent
framesize_t picSize             = FRAMESIZE_SVGA;              // Image size

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED       4

#define WIFI_INIT 1
#define WIFI_CONNECT 2
#define WIFI_CONNECTED 3
#define WIFI_RECONNECT 4
uint8_t WIFI_State = WIFI_INIT;

// for MQTT
#define _MQTT_INIT 1
#define _MQTT_CONNECT 2
#define _MQTT_CONNECTED 3
#define _MQTT_RECONNECT 4
#define _MQTT_READY 5
#define _MQTT_IDLE 6

// User Define
// Period of Sensor Data, can make more
const unsigned long base_generate_interval = 3*1000;    //20 * 1000;
unsigned long cam_generate_previousMillis = 0;
unsigned long cam_generate_interval = 1*1000;//base_generate_interval;

unsigned long wifi_previousMillis = 0;
const long wifi_interval = 30; // count
const long wifi_led_interval = 100; // ms
uint16_t wifi_wait_count = 0;

unsigned long mqtt_previousMillis = 0;
unsigned long mqtt_interval = 8; // count
const unsigned long mqtt_base_led_interval = 250; // ms
unsigned long mqtt_led_interval = mqtt_base_led_interval; // ms
uint16_t mqtt_wait_count = 0;
unsigned long mqtt_watchdog_count = 0;

char mqtt_id[]= "ESP32_CAM";
uint8_t MQTT_State = _MQTT_INIT;

char in_message[MQTT_MAX_PACKET_SIZE];
StaticJsonBuffer<MQTT_MAX_PACKET_SIZE> jsonBuffer;
unsigned long req_previousMillis = 0;
const long req_interval = 15000; // ms

unsigned long chk_previousMillis = 0;
const long chk_interval = 100; // ms
unsigned long chk_count = 0;

#define UPLOAD_UPLOADING 2
#define UPLOAD_UPLOADED 3
unsigned long uploading_previousMillis = 0;
const long uploading_interval = 10000; // ms
uint8_t UPLOAD_State = UPLOAD_UPLOADING;
uint8_t upload_retry_count = 0;

#define NCUBE_REQUESTED 1
#define NCUBE_REQUESTING 2

char req_id[10];
String state = "create_ae";
uint8_t nCube_State = NCUBE_REQUESTED;
uint8_t sequence = 0;

String strRef[8];
int strRef_length = 0;

#define QUEUE_SIZE 8
typedef struct _queue_t {
    uint8_t pop_idx;
    uint8_t push_idx;
    String ref[QUEUE_SIZE];
    String con[QUEUE_SIZE];
    String rqi[QUEUE_SIZE];
    unsigned long* previousMillis[QUEUE_SIZE];
} queue_t;

queue_t noti_q;
queue_t upload_q;

// Information of CSE as Mobius with MQTT
const String FIRMWARE_VERSION = "1.0.0.0";
String AE_NAME = "CAMERA_AE3"; 
String AE_ID = "S" + AE_NAME;
const String CSE_ID = "/Mobius2";
const String CB_NAME = "Mobius";
const char* MOBIUS_MQTT_BROKER_IP = "192.168.1.11";//my pc ip(wifi 라우터로  네트웍 연결됨)
const uint16_t MOBIUS_MQTT_BROKER_PORT = 1883;

String body_str = "";

char resp_topic[48];
char noti_topic[48];

unsigned long system_watchdog = 0;

OneM2MClient nCube;

void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        //size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        //*dest++ = charset[index];

        size_t index = random(62);
        *dest++ = charset[index];
    }
    *dest = '\0';
}
// build tree of resource of oneM2M
void buildResource() {
    nCube.configResource(2, "/"+CB_NAME, AE_NAME);                       // AE resource

    nCube.configResource(3, "/"+CB_NAME+"/"+AE_NAME, "update");          // Container resource   
    nCube.configResource(3, "/"+CB_NAME+"/"+AE_NAME, "cam");             // Container resource
    nCube.configResource(3, "/"+CB_NAME+"/"+AE_NAME, "motor");             // Container resource
    nCube.configResource(23, "/"+CB_NAME+"/"+AE_NAME+"/update", "sub");  // Subscription resource
    nCube.configResource(23, "/"+CB_NAME+"/"+AE_NAME+"/cam", "sub");     // Subscription resource
    nCube.configResource(23, "/"+CB_NAME+"/"+AE_NAME+"/motor", "sub");     // Subscription resource
}    
 
//
int send_ok = 0;
int send_req = 0;
int sendPhoto() {
  String getAll;
  String getBody;  
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  Serial.println("Connecting to server: " + serverName);
  if (espClient.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"profile_pic\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";
    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
    espClient.println("POST " + serverPath + " HTTP/1.1");
    espClient.println("Host: " + serverName);
    espClient.println("Content-Length: " + String(totalLen));
    espClient.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    espClient.println();
    espClient.print(head);
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        espClient.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        espClient.write(fbBuf, remainder);
      }
    }
    espClient.print(tail);
    esp_camera_fb_return(fb);
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);
      while (espClient.available()) {
        char c = espClient.read();
        if (c == '\n') {
          if (getAll.length()==0) { state=true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state==true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length()>0) { break; }
    }
    Serial.println();
    espClient.stop();
    Serial.println(getBody);
    send_ok = 1;
  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    send_ok = 0;
    Serial.println(getBody);
  } 
  serverPath = "/upload"; 
  return send_ok;
}
void setup() {
  //added
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  pinMode(FLASH_LED, OUTPUT);
  digitalWrite(FLASH_LED, HIGH); //flash led on
  
  tasMotor.init();
  tasMotor.setSPEED(5);
  
  setup_wifi();
  String topic = "/oneM2M/resp/" + AE_ID + CSE_ID + "/json";
  topic.toCharArray(resp_topic, 64);

  topic = "/oneM2M/req" + CSE_ID + "/" + AE_ID + "/json";
  topic.toCharArray(noti_topic, 64);

  nCube.Init(CSE_ID, MOBIUS_MQTT_BROKER_IP, AE_ID);
  //for MQTT
  mqtt.setServer(MOBIUS_MQTT_BROKER_IP, MOBIUS_MQTT_BROKER_PORT);   
  mqtt.setCallback(mqtt_message_handler); 
  MQTT_State = _MQTT_INIT;
  Serial.println("waiting for mqtt connection...");
  while (!mqtt.connect(mqtt_id)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("mqtt connected"); 
  
  buildResource();
  if (mqtt.subscribe(resp_topic)) {
        Serial.println(String(resp_topic) + " ---------------------------------------------------Successfully subscribed");
  }

  if (mqtt.subscribe(noti_topic)) {
      Serial.println(String(noti_topic) + " ---------------------------------------------------Successfully subscribed");
  }
  digitalWrite(FLASH_LED, LOW); //flash led on
  rand_str(req_id, 8);
  nCube_State = NCUBE_REQUESTED;
  //init OTA client
  //OTAClient.begin(AE_NAME, FIRMWARE_VERSION);
 
  //for http post
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = picSize;  // RESOLUTION
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
  //sendPhoto();
}
void setup_wifi() {

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  WIFI_State = WIFI_CONNECT;
  //digitalWrite(FLASH_LED, LOW);//flash led off
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
}
void WiFi_chkconnect() {
    if(USE_WIFI) {
        if(WIFI_State == WIFI_INIT) {
            //digitalWrite(ledPin, HIGH);

            //Serial.println("beginProvision - WIFI_INIT");
            //WiFi.beginProvision();
            //WiFi.begin("FILab", "badacafe00");
            WiFi.begin("Laon_R&D", "development");
            //WiFi.beginAP("FeatherM0", "adafruit");

            WIFI_State = WIFI_CONNECT;
            wifi_previousMillis = 0;
            wifi_wait_count = 0;
            noti_q.pop_idx = 0;
            noti_q.push_idx = 0;
            upload_q.pop_idx = 0;
            upload_q.push_idx = 0;
        }
        else if(WIFI_State == WIFI_CONNECTED) {
            if (WiFi.status() == WL_CONNECTED) {
                return;
            }

            wifi_wait_count = 0;
            if(WIFI_State == WIFI_CONNECTED) {
                WIFI_State = WIFI_RECONNECT;
                wifi_previousMillis = 0;
                wifi_wait_count = 0;
                noti_q.pop_idx = 0;
                noti_q.push_idx = 0;
                upload_q.pop_idx = 0;
                upload_q.push_idx = 0;
            }
            else {
                WIFI_State = WIFI_CONNECT;
                wifi_previousMillis = 0;
                wifi_wait_count = 0;
                noti_q.pop_idx = 0;
                noti_q.push_idx = 0;
                upload_q.pop_idx = 0;
                upload_q.push_idx = 0;
            }
            //nCube.MQTT_init(AE_ID);
        }
        else if(WIFI_State == WIFI_CONNECT) {
            unsigned long currentMillis = millis();
            if (currentMillis - wifi_previousMillis >= wifi_led_interval) {
                wifi_previousMillis = currentMillis;
                if(wifi_wait_count++ >= wifi_interval) {
                    wifi_wait_count = 0;
                    if (WiFi.status() != WL_CONNECTED) {
                        //Serial.println("Provisioning......");
                        Serial.println("wait for .....");
                    }
                }
                else {
//                    if(wifi_wait_count % 2) {
//                        digitalWrite(ledPin, HIGH);
//                    }
//                    else {
//                        digitalWrite(ledPin, LOW);
//                    }
                      Serial.println("wait connected.....");
                }
            }
            else {
                if (WiFi.status() == WL_CONNECTED) {
                    // you're connected now, so print out the status:
                    printWiFiStatus();

                    //digitalWrite(ledPin, LOW);

                    WIFI_State = WIFI_CONNECTED;
                    wifi_previousMillis = 0;
                    wifi_wait_count = 0;
                    noti_q.pop_idx = 0;
                    noti_q.push_idx = 0;
                    upload_q.pop_idx = 0;
                    upload_q.push_idx = 0;
                }
            }
        }
        else if(WIFI_State == WIFI_RECONNECT) {
            //digitalWrite(ledPin, HIGH);

            unsigned long currentMillis = millis();
            if (currentMillis - wifi_previousMillis >= wifi_led_interval) {
                wifi_previousMillis = currentMillis;
                if(wifi_wait_count++ >= wifi_interval) {
                    wifi_wait_count = 0;
                    if (WiFi.status() != WL_CONNECTED) {
                        Serial.print("Attempting to connect to SSID: ");
                        Serial.println("previous SSID");
                        WiFi.begin("Laon_R&D", "development");
                        //WiFi.begin();
                    }
                }
                else {
//                    if(wifi_wait_count % 2) {
//                        digitalWrite(ledPin, HIGH);
//                    }
//                    else {
//                        digitalWrite(ledPin, LOW);
//                    }
                }
            }
            else {
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("Connected to wifi");
                    printWiFiStatus();

                    //digitalWrite(ledPin, LOW);

                    WIFI_State = WIFI_CONNECTED;
                    wifi_previousMillis = 0;
                    wifi_wait_count = 0;
                }
            }
        }
    }
}

void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}
void wifi_reconnect(){  
   while (WiFi.status() != WL_CONNECTED) {
    espClient.connect(serverName.c_str(), serverPort);
    Serial.print(".");
    delay(500);
  }
}
void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(mqtt_id)) {
      Serial.println("connected");
      mqtt_watchdog_count = 0;
      Serial.println("connected");      
      if (mqtt.subscribe(resp_topic)) {
          Serial.println(String(resp_topic) + " ---------------------------------------------------Successfully subscribed");
      }

      if (mqtt.subscribe(noti_topic)) {
          Serial.println(String(noti_topic) + " ---------------------------------------------------Successfully subscribed");
      }

      MQTT_State = _MQTT_CONNECTED;
      //nCube.reset_heartbeat();   //by joon
            
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void Split(String sData, char cSeparator)
{
    int nCount = 0;
    int nGetIndex = 0 ;
    strRef_length = 0;

    String sTemp = "";
    String sCopy = sData;

    while(true) {
        nGetIndex = sCopy.indexOf(cSeparator);

        if(-1 != nGetIndex) {
            sTemp = sCopy.substring(0, nGetIndex);
            strRef[strRef_length++] = sTemp;
            sCopy = sCopy.substring(nGetIndex + 1);
        }
        else {
            strRef[strRef_length++] = sCopy;
            break;
        }
        ++nCount;
    }
}
void notiProcess() {     

    if(noti_q.pop_idx != noti_q.push_idx) {
        Split(noti_q.ref[noti_q.pop_idx], '/');
        Serial.print(" sub:");        
        if(strRef[strRef_length-1] == "cam") {
            Serial.print(" CAM Value:");
            Serial.println(noti_q.con[noti_q.pop_idx]);
            Split(noti_q.con[noti_q.pop_idx], '/');
            serverName = strRef[strRef_length-2];
            serverPath = serverPath + '/'+ strRef[strRef_length-1];                                
            if(strRef[strRef_length-3] == "cap"){
              send_req = 1; //capture request
              String resp_body = "";
              resp_body += "{\"rsc\":\"2000\",\"to\":\"\",\"fr\":\"" + nCube.getAeid() + "\",\"pc\":\"\",\"rqi\":\"" + noti_q.rqi[noti_q.pop_idx] + "\"}";
              nCube.response(mqtt, resp_body);
              Serial.println("2000 ---->");                  
            }  
            else if(strRef[strRef_length-3] =="on"){
               digitalWrite(FLASH_LED, HIGH);   //Turn the LED on       
            } 
            else if(strRef[strRef_length-3] == "off"){
               digitalWrite(FLASH_LED, LOW);   //Turn the LED on                
            }            
            
        }
        else if(strRef[strRef_length-1] == "motor") {
            
            if(noti_q.con[noti_q.pop_idx] =="1"){
               tasMotor.setMOTOR(1);  //clockwise 45 degree rotate
            }
            else if(noti_q.con[noti_q.pop_idx] =="2"){
               tasMotor.setMOTOR(2);  //counter clockwise 45 degree rotate              
            }
            else if(noti_q.con[noti_q.pop_idx] =="3"){
               tasMotor.setMOTOR(3);  //clockwise 360 degree rotate           
            }
            else if(noti_q.con[noti_q.pop_idx] =="4"){
               tasMotor.setMOTOR(4);  //counter clokcwise 360 degree rotae            
            } 
            String resp_body = "";
            resp_body += "{\"rsc\":\"2000\",\"to\":\"\",\"fr\":\"" + nCube.getAeid() + "\",\"pc\":\"\",\"rqi\":\"" + noti_q.rqi[noti_q.pop_idx] + "\"}";
            nCube.response(mqtt, resp_body);

            Serial.println("2001 ---->");
        }      
        else if(strRef[strRef_length-1] == "update") {
          if (noti_q.con[noti_q.pop_idx] == "active") {
              //OTAClient.start();   // active OTAClient upgrad process
  
              String resp_body = "";
              resp_body += "{\"rsc\":\"2000\",\"to\":\"\",\"fr\":\"" + nCube.getAeid() + "\",\"pc\":\"\",\"rqi\":\"" + noti_q.rqi[noti_q.pop_idx] + "\"}";
              nCube.response(mqtt, resp_body);
          }
        }
        noti_q.pop_idx++;
        if(noti_q.pop_idx >= QUEUE_SIZE) {
            noti_q.pop_idx = 0;
        }
    }
}

void publisher() {   
    unsigned long currentMillis = millis();
    if (currentMillis - req_previousMillis >= req_interval) {
        req_previousMillis = currentMillis;

        rand_str(req_id, 8);
        nCube_State = NCUBE_REQUESTED;
    }

    if(nCube_State == NCUBE_REQUESTED && WIFI_State == WIFI_CONNECTED && MQTT_State == _MQTT_CONNECTED) {
        if (state == "create_ae") {
            Serial.print(state + " - ");
            Serial.print(String(sequence));
            Serial.print(" - ");
            Serial.println(String(req_id));
            nCube_State = NCUBE_REQUESTING;
            body_str = nCube.createAE(mqtt, req_id, 0, "3.14");

            if (body_str == "0") {
              Serial.println(F("REQUEST Failed"));
              delay(500);
            
            }
            else {
              Serial.print("Request [");
              Serial.print(nCube.getReqTopic());
              Serial.print("] ----> ");
              Serial.println(body_str.length()+1);
              Serial.println(body_str);            
            }
                //digitalWrite(ledPin, HIGH);
        }
        else if (state == "create_cnt") {
            Serial.print(state + " - ");
            Serial.print(String(sequence));
            Serial.print(" - ");
            Serial.println(String(req_id));            
                       
            nCube_State = NCUBE_REQUESTING;           
            body_str = nCube.createCnt(mqtt, req_id, sequence);
            if (body_str == "0") {
              Serial.println(F("REQUEST Failed"));
            }
            else {
              
              Serial.print("Request [");
              Serial.print(nCube.getReqTopic());
              Serial.print("] ----> ");
              Serial.println(body_str.length()+1);
              Serial.println(body_str);
              delay(500);              
               
            }
            //digitalWrite(ledPin, HIGH);
        }
        else if (state == "delete_sub") {
            Serial.print(state + " - ");
            Serial.print(String(sequence));
            Serial.print(" - ");
            Serial.println(String(req_id));
            nCube_State = NCUBE_REQUESTING;
            body_str = nCube.deleteSub(mqtt, req_id, sequence);
            if (body_str == "0") {
            Serial.println(F("REQUEST Failed"));
          }
          else {
            Serial.print("Request [");
            Serial.print(nCube.getReqTopic());
            Serial.print("] ----> ");
            Serial.println(body_str.length()+1);
            Serial.println(body_str);
          }
            //digitalWrite(ledPin, HIGH);
        }
        else if (state == "create_sub") {
            Serial.print(state + " - ");
            Serial.print(String(sequence));
            Serial.print(" - ");
            Serial.println(String(req_id));
            nCube_State = NCUBE_REQUESTING;
            Serial.print("sequence:");
            Serial.println(sequence);
            body_str = nCube.createSub(mqtt, req_id, sequence);
            if (body_str == "0") {
            Serial.println(F("REQUEST Failed"));
            }
            else {
            Serial.print("Request [");
            Serial.print(nCube.getReqTopic());
            Serial.print("] ----> ");
            Serial.println(body_str.length()+1);
            Serial.println(body_str);

              
        }
        //digitalWrite(ledPin, HIGH);
        }
        else if (state == "create_cin") {
        }
    }
}
void mqtt_message_handler(char* topic_in, byte* payload, unsigned int length) {
    
    String topic = String(topic_in);

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] <---- ");
    Serial.println(length);

    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    //noInterrupts();

    if (topic.substring(8,12) == "resp") {
        memset((char*)in_message, '\0', length+1);
        memcpy((char*)in_message, payload, length);
        JsonObject& resp_root = jsonBuffer.parseObject(in_message);

        if (!resp_root.success()) {
            Serial.println(F("parseObject() failed"));
            jsonBuffer.clear();
            return;
        }

        espClient.flush();

        resp_handler(resp_root["rsc"], resp_root["rqi"]);

        jsonBuffer.clear();
    }
    else if(topic.substring(8,11) == "req") {
        memset((char*)in_message, '\0', length+1);
        memcpy((char*)in_message, payload, length);
        JsonObject& req_root = jsonBuffer.parseObject(in_message);

        if (!req_root.success()) {
            Serial.println(F("parseObject() failed"));
            return;
        }

        espClient.flush();

        noti_handler(req_root["pc"]["m2m:sgn"]["sur"], req_root["rqi"], req_root["pc"]["m2m:sgn"]["nev"]["rep"]["m2m:cin"]["con"]);

        jsonBuffer.clear();
    }
    //interrupts();
    system_watchdog = 0;
}
void noti_handler(String sur, String rqi, String con) {
    if (state == "create_cin") {
        Serial.print("<---- ");
        if(sur.charAt(0) != '/') {
            sur = '/' + sur;
            Serial.println(sur);
        }
        else {
            Serial.println(sur);
        }

        String valid_sur = nCube.validSur(sur);
        if (valid_sur != "empty") {
            noti_q.ref[noti_q.push_idx] = valid_sur;
            noti_q.con[noti_q.push_idx] = con;
            noti_q.rqi[noti_q.push_idx] = rqi;
            noti_q.push_idx++;
            if(noti_q.push_idx >= QUEUE_SIZE) {
                noti_q.push_idx = 0;
            }
            if(noti_q.push_idx == noti_q.pop_idx) {
                noti_q.pop_idx++;
                if(noti_q.pop_idx >= QUEUE_SIZE) {
                    noti_q.pop_idx = 0;
                }
            }
        }
    }
}
void resp_handler(int response_code, String response_id) {
    String request_id = String(req_id);     
    
    if (request_id == response_id) {
        if (response_code == 2000 || response_code == 2001 || response_code == 2002 || response_code == 4105 || response_code == 4004) {
            Serial.print("<---- ");
            Serial.println(response_code);
            if (state == "create_ae") {
                sequence++;
                if(sequence >= nCube.getAeCount()) {
                    state = "create_cnt";                    
                    sequence = 0;
                }
                rand_str(req_id, 8);
                nCube_State = NCUBE_REQUESTED;
            }
            else if (state == "create_cnt") {
                sequence++;                
                if(sequence >= nCube.getCntCount()) {
                    state = "delete_sub";                   
                    sequence = 0;
                }
                rand_str(req_id, 8);
                nCube_State = NCUBE_REQUESTED;
            }
            else if(state == "delete_sub") {
                sequence++;
                if(sequence >= nCube.getSubCount()) {
                    state = "create_sub";                   
                    sequence = 0;
                }
                rand_str(req_id, 8);
                nCube_State = NCUBE_REQUESTED;
            }
            else if (state == "create_sub") {
                sequence++;
                if(sequence >= nCube.getSubCount()) {
                    state = "create_cin";                   
                    sequence = 0;
                }
                rand_str(req_id, 8);
                nCube_State = NCUBE_REQUESTED;
            }
            else if (state == "create_cin") {                                
                upload_retry_count = 0;
                if(upload_q.pop_idx == upload_q.push_idx) {

                }
                else {
                    *upload_q.previousMillis[upload_q.pop_idx] = millis();

                    upload_q.pop_idx++;
                    if(upload_q.pop_idx >= QUEUE_SIZE) {
                        upload_q.pop_idx = 0;
                    }
                }
            }
            //digitalWrite(ledPin, LOW);
            UPLOAD_State = UPLOAD_UPLOADED;
        }
    }
}
unsigned long mqtt_sequence = 0;
void chkState() {
    unsigned long currentMillis = millis();
    if (currentMillis - chk_previousMillis >= chk_interval) {
        chk_previousMillis = currentMillis;

        system_watchdog++;
        if(system_watchdog > 9000) {
          Serial.println("Togle led");
//            if(system_watchdog % 2) {
//                digitalWrite(ledPin, HIGH);
//            }
//            else {
//                digitalWrite(ledPin, LOW);
//            }
        }
        else if(system_watchdog > 18000) {
            //NVIC_SystemReset();
            Serial.println("Need to SystemReset~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        }

        if(WIFI_State == WIFI_CONNECT) {
            Serial.println("WIFI_CONNECT");
            MQTT_State = _MQTT_INIT;
        }
        else if(WIFI_State == WIFI_RECONNECT) {
            Serial.println("WIFI_RECONNECT");
            MQTT_State = _MQTT_INIT;
        }
        else if(WIFI_State == WIFI_CONNECTED && MQTT_State == _MQTT_INIT) {
            MQTT_State = _MQTT_CONNECT;
        }

        if(MQTT_State == _MQTT_CONNECT) {
            Serial.println("_MQTT_CONNECT");
        }

        /*if(WIFI_State == WIFI_CONNECTED && MQTT_State == _MQTT_CONNECTED) {
            chk_count++;
            if(chk_count >= 100) {
                chk_count = 0;

                //noInterrupts();
                body_str = nCube.heartbeat(mqtt);
                // char seq[10];
                // sprintf(seq, "%ld", ++mqtt_sequence);
                // mqtt.publish("/nCube/count/test", seq, strlen(seq));
                // Serial.println(String(mqtt_sequence));
                //interrupts();

                if (body_str == "Failed") {
                    Serial.println(F("Heartbeat Failed"));
                }
                else {
                    Serial.print("Send heartbeat [");
                    Serial.print(nCube.getHeartbeatTopic());
                    Serial.print("] ----> ");
                    Serial.println(body_str.length()+1);
                    Serial.println(body_str);
                    system_watchdog = 0;
                }
            }
        }*/
    }
}
void camProcess() {
    unsigned long currentMillis = millis();
    if (currentMillis - cam_generate_previousMillis >= cam_generate_interval) {
        cam_generate_previousMillis = currentMillis;
        cam_generate_interval = base_generate_interval + (random(1000));
        if (state == "create_cin") {
            String cnt = "cam";
            String con = "\"?\"";            
            Serial.println("cam state");
            if(sendPhoto()) {
                digitalWrite(FLASH_LED, HIGH);   //Turn the LED on
                delay(500);
                digitalWrite(FLASH_LED, LOW);   //Turn the LED on
                Serial.println("photo send ok");          
            }
            else {
                Serial.println("photo send fail"); 
            }                 

           
        }
    }
}
void camState(int val){
  unsigned long currentMillis = millis();  
  if (currentMillis - cam_generate_previousMillis >= cam_generate_interval) {
        cam_generate_previousMillis = currentMillis;
        cam_generate_interval = base_generate_interval + (random(1000));
            String cnt = "cam";            
            String con = "\"?\"";   
            con = String(val);
            con = "\"" + con + "\"";

            char rqi[10];
            rand_str(rqi, 8);
            //if (state){
                //tasCam.setFlash("1"); 
                //tasCam.setCam("1");              
            //} else {
                //tasCam.setFlash("0");  
                //tasCam.setCam("0");                 
            //}     
            upload_q.ref[upload_q.push_idx] = "/"+CB_NAME+"/"+AE_NAME+"/"+cnt;
            upload_q.con[upload_q.push_idx] = con;
            upload_q.rqi[upload_q.push_idx] = String(rqi);
            upload_q.push_idx++;
            if(upload_q.push_idx >= QUEUE_SIZE) {
                upload_q.push_idx = 0;
            }
            if(upload_q.push_idx == upload_q.pop_idx) {
                upload_q.pop_idx++;
                if(upload_q.pop_idx >= QUEUE_SIZE) {
                    upload_q.pop_idx = 0;
                }
            }
           
            
  } 
}
void uploadProcess() {
    if(WIFI_State == WIFI_CONNECTED && MQTT_State == _MQTT_CONNECTED) {
        unsigned long currentMillis = millis();
        if (currentMillis - uploading_previousMillis >= uploading_interval) {
            uploading_previousMillis = currentMillis;

            if (state == "create_cin") {
                if(UPLOAD_State == UPLOAD_UPLOADING) {
                    Serial.println("upload timeout");
                }

                UPLOAD_State = UPLOAD_UPLOADED;
                upload_retry_count++;
                if(upload_retry_count > 2) {
                    upload_retry_count = 0;
                    if(upload_q.pop_idx == upload_q.push_idx) {

                    }
                    else {
                        upload_q.pop_idx++;
                        if(upload_q.pop_idx >= QUEUE_SIZE) {
                            upload_q.pop_idx = 0;
                        }
                    }
                }
            }
        }

        if((UPLOAD_State == UPLOAD_UPLOADED) && (upload_q.pop_idx != upload_q.push_idx)) {
            if (espClient.available()) {
                return;
            }

            uploading_previousMillis = millis();
            UPLOAD_State = UPLOAD_UPLOADING;

            upload_q.rqi[upload_q.pop_idx].toCharArray(req_id, 10);

            Serial.println("pop : " + String(upload_q.pop_idx));
            Serial.println("push : " + String(upload_q.push_idx));
            //noInterrupts();
            body_str = nCube.createCin(mqtt, upload_q.rqi[upload_q.pop_idx], upload_q.ref[upload_q.pop_idx], upload_q.con[upload_q.pop_idx]);
            espClient.flush();
            //interrupts();
            if (body_str == "0") {
            Serial.println(F("REQUEST Failed"));
          }
          else {
                system_watchdog = 0;
            Serial.print("Request [");
            Serial.print(nCube.getReqTopic());
            Serial.print("] ----> ");
            Serial.println(body_str.length()+1);
            Serial.println(body_str);
          }
            //digitalWrite(ledPin, HIGH);
        }
    }
}
void loop() {    
   // nCube loop
   nCube_loop();
   // user defined loop
   if(!send_req) {
      notiProcess();
      //camState(send_req);
   }
   else {
     
      camProcess();
      //camState(1);//mqtt notification
      uploadProcess();
      send_req = 0;
   }
   
}
void nCube_loop() {
    WiFi_chkconnect();
    if (!mqtt.loop()) {
        MQTT_State = _MQTT_CONNECT;
        //digitalWrite(13, HIGH);
        mqtt_reconnect();
        //digitalWrite(13, LOW);
    }
    else {
        MQTT_State = _MQTT_CONNECTED;
    }

    chkState();
    publisher();
    //otaProcess();
    uploadProcess();
}

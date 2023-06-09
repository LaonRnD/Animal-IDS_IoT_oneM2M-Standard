#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
//#include <Adafruit_BusIO.h>
#include <Adafruit_MLX90614.h>

/**
Copyright (c) 2018, OCEAN
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <WiFi101.h>
#include <WiFiMDNSResponder.h>
#include "OneM2MClient.h"
#include "m0_ota.h"

const int ledPin = 13; // LED pin for connectivity status indicator

uint8_t USE_WIFI = 1;

//for X4 Sensor
#define SENSOR_PIN 12



Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2

#define WIFI_INIT 1
#define WIFI_CONNECT 2
#define WIFI_CONNECTED 3
#define WIFI_RECONNECT 4
uint8_t WIFI_State = WIFI_INIT;

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

// for MQTT
#define _MQTT_INIT 1
#define _MQTT_CONNECT 2
#define _MQTT_CONNECTED 3
#define _MQTT_RECONNECT 4
#define _MQTT_READY 5
#define _MQTT_IDLE 6

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char mqtt_id[16];
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

String body_str = "";

char resp_topic[48];
char noti_topic[48];

unsigned long system_watchdog = 0;

bool buzz_flag = false;
bool led_flag = false;

// -----------------------------------------------------------------------------
// User Define
// Period of Sensor Data, can make more
const unsigned long base_generate_interval = 3*1000;    //20 * 1000;
unsigned long temp_generate_previousMillis = 0;
unsigned long temp_generate_interval = base_generate_interval;
unsigned long uwb_generate_previousMillis = 0;
unsigned long uwb_generate_interval = base_generate_interval;

// Information of CSE as Mobius with MQTT
const String FIRMWARE_VERSION = "1.0.0.0";
String AE_NAME = "AE1FM0"; 
String AE_ID = "S" + AE_NAME;
const String CSE_ID = "/Mobius2";
const String CB_NAME = "Mobius";
const char* MOBIUS_MQTT_BROKER_IP = "192.168.1.11";
const uint16_t MOBIUS_MQTT_BROKER_PORT = 1883;

OneM2MClient nCube;



// add TAS(Thing Adaptation Layer) for Sensor
#include "TasLED.h"
TasLED tasLed;

#include "TasCCS811.h"
TasCCS811 TasCCSSensor;

//#include "Adafruit_MLX90614.h"
//Adafruit_MLX90614 TasGY906Sensor;

#include "TasBUZZ.h"
TasBUZZ tasBuzz(A1);    //A0

#include "TasUWB.h"
TasUWB tasUwb(12);



unsigned long previousNotificationTime = 0;
bool ledState = LOW;
const unsigned long blinkDuration = 500;

void blinkLed() {
    unsigned long currentTime = millis();
    static unsigned long previousBlinkTime = currentTime;
    if (currentTime - previousBlinkTime >= blinkDuration) {
        previousBlinkTime = currentTime;
        ledState = !ledState;
        tasLed.setLED(String(random(ledState, 8)));
  }  
}



// build tree of resource of oneM2M
void buildResource() {
    nCube.configResource(2, "/"+CB_NAME, AE_NAME);                       // AE resource

    nCube.configResource(3, "/"+CB_NAME+"/"+AE_NAME, "update");          // Container resource    
    nCube.configResource(3, "/"+CB_NAME+"/"+AE_NAME, "led");             // Container resource    
    nCube.configResource(3, "/"+CB_NAME+"/"+AE_NAME, "temp");            // Container resource
    nCube.configResource(3,"/"+CB_NAME+"/"+AE_NAME, "buzzer");           // Container resource
    nCube.configResource(3,"/"+CB_NAME+"/"+AE_NAME, "uwb");              // Container resource

    nCube.configResource(23, "/"+CB_NAME+"/"+AE_NAME+"/update", "sub");  // Subscription resource
    nCube.configResource(23, "/"+CB_NAME+"/"+AE_NAME+"/led", "sub");     // Subscription resource    
    nCube.configResource(23, "/"+CB_NAME+"/"+AE_NAME+"/buzzer", "sub");  // Subscription resource
    nCube.configResource(23, "/"+CB_NAME+"/"+AE_NAME+"/uwb", "sub");     // Subscription resource
}

// Period of generating sensor data

void tempGenProcess() {
    //bool buzz_temp_flag = false;
    unsigned long currentMillis = millis();
    if (currentMillis - temp_generate_previousMillis >= temp_generate_interval) {
        temp_generate_previousMillis = currentMillis;
        temp_generate_interval = base_generate_interval + (random(1000));

        //int sensorValue = digitalRead(SENSOR_PIN);

        if (state == "create_cin") {
            String cnt = "temp";
            String con = "\"?\"";
            String con1 = "\"?\"";
            String con2 = "\"?\"";
            String con3 = "\"?\"";

            //if(TasGY906Sensor.available()) {            
            float tempO = mlx.readObjectTempC();
            float tempA = mlx.readAmbientTempC();
            float tempD = abs(tempO-tempA);

            //Serial.println(tempO);
            //Serial.println(tempA);
            //Serial.println(tempD);            


            con1 = String(tempA);
            con2 = String(tempO);
            con3 = String(tempD);

            con = "\"" + con1 + ":" + con2 + ":" + con3 +"\"";
            //con = "\"ATemp:" + con1 + " / OTemp:" + con2 + " / DTemp:" + con3 + "\"";
            //con2 = "\"" + con2 + "\"";
            //con3 = "\"" + con3 + "\"";

            char rqi[10];
            rand_str(rqi, 8);
            
            
            if(buzz_flag == true) {
                //tasBuzz.on();
            } else if(tempD > 5.00) {
                //tasBuzz.on();
                unsigned long currentTime = millis();
                if (currentTime - previousNotificationTime >= temp_generate_interval) {
                    // Send sensor data to MQTT server
                    notiProcess();                
                    previousNotificationTime = currentTime;
                }
                //notiProcess();                
            } else {
                //tasBuzz.off();
            }
                        

        /*
            if (tempD < 5.00) {
                uint8_t buzz_state = tasBuzz.getBUZZ();
                if(buzz_state == 0) {
                    tasBuzz.off();
                } else {
                    tasBuzz.off();
                }
            }  
            //else if ((tempD > 5.00) && (buzz_flag == true)){                 
            //    tasBuzz.on();
            //    buzz_flag = false;
            //    //unsigned long currentTime = millis();
            //    //if (currentTime - previousNotificationTime >= temp_generate_interval) {
            //        // Send sensor data to MQTT server
            //    notiProcess();                
            //    //    previousNotificationTime = currentTime;
            //    //}
            //}
            else if (tempD > 5.00) {
                uint8_t buzz_state = tasBuzz.getBUZZ();
                if(buzz_state == 1) {
                    tasBuzz.on();
                } else {
                    tasBuzz.on();
                }
                notiProcess();
            }
        */

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

            //Serial.println("pop : " + String(upload_q.pop_idx));
            //Serial.println("push : " + String(upload_q.push_idx));
            //}
        }
    }
}

void uwbGenProcess() {
    unsigned long currentMillis = millis();
    if (currentMillis - uwb_generate_previousMillis >= uwb_generate_interval) {
        uwb_generate_previousMillis = currentMillis;
        uwb_generate_interval = base_generate_interval + (random(1000));        

        if (state == "create_cin") {
            String cnt = "uwb";
            String con = "\"?\"";            

            //if(TasGY906Sensor.available()) {            
            int sensorValue = tasUwb.read(); 

            bool objectDetected = (sensorValue == LOW);
            con = String(sensorValue);
            con = "\"" + con + "\"";

            char rqi[10];
            rand_str(rqi, 8);


        // led control based on API priority
        /*
            if(led_flag == true) {
                tasLed.setLED("b");                
            } else if(objectDetected == true) {
                tasLed.setLED("b");
                unsigned long currentTime = millis();
                if (currentTime - previousNotificationTime >= temp_generate_interval) {
                    // Send sensor data to MQTT server
                    notiProcess();                
                    previousNotificationTime = currentTime;
                }
                //notiProcess();                
            } else {
                tasLed.setLED("0");
            }     
        */


        // notification based on detection
            
            if (objectDetected){
                //blinkLed();
                //tasLed.setLED("b");
                //tasBuzz.on();                
                //sensorValue = tasUwb.read();
                //objectDetected = (sensorValue == LOW);
                //tasLed.setLED(String(random(1, 8))); 
                //notiProcess();
                unsigned long currentTime = millis();
                if (currentTime - previousNotificationTime >= uwb_generate_interval) {
                    // Send sensor data to MQTT server
                    notiProcess();
                    previousNotificationTime = currentTime;
                }
            } else {
                //tasLed.setLED("0");
                //tasBuzz.off();
                //unsigned long currentTime = millis();
                //if (currentTime - previousNotificationTime >= uwb_generate_interval) {
                    // Send sensor data to MQTT server
                //    notiProcess();
                //    previousNotificationTime = currentTime;
                //}
            }          
            


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

            //Serial.println("pop : " + String(upload_q.pop_idx));
            //Serial.println("push : " + String(upload_q.push_idx));
            //}
        }
    }
}


// Handle API command
void handleAPICommand(const String& command) {
  if (command == "on") {
    //tasLAMP.setLAMP("1");  // Turn on the LED
  } else if (command == "off") {
    //tasLAMP.setLAMP("0");  // Turn off the LED
  }
}


// Process notification of Mobius for control
void notiProcess() {   
    
    if(noti_q.pop_idx != noti_q.push_idx) {
        Split(noti_q.ref[noti_q.pop_idx], '/');
        if(strRef[strRef_length-1] == "led"){
            //Serial.print("LED Value: ");
            //Serial.println(noti_q.con[noti_q.pop_idx]);
            tasLed.setLED(noti_q.con[noti_q.pop_idx]);            
            if(((noti_q.con[noti_q.pop_idx]) == "on") && (led_flag == false)) {
                tasLed.setLED("b");
                led_flag = true;                
            }
            else if (((noti_q.con[noti_q.pop_idx]) == "off") && (led_flag == true)) {
                tasLed.setLED("0");
                led_flag = false;
            }           

            String resp_body = "";
            resp_body += "{\"rsc\":\"2000\",\"to\":\"\",\"fr\":\"" + nCube.getAeid() + "\",\"pc\":\"\",\"rqi\":\"" + noti_q.rqi[noti_q.pop_idx] + "\"}";
            nCube.response(mqtt, resp_body);

            Serial.println("2000 ---->");
        }
        else if(strRef[strRef_length-1] == "buzzer"){
            //Serial.print("BUZZ Value: ");
            //Serial.println(noti_q.con[noti_q.pop_idx]);                        
            tasBuzz.setBUZZ(noti_q.con[noti_q.pop_idx]);
            if(((noti_q.con[noti_q.pop_idx]) == "on") && (buzz_flag == false)) {
                tasBuzz.setBUZZ("on");
                buzz_flag = true;                
            }
            else if (((noti_q.con[noti_q.pop_idx]) == "off") && (buzz_flag == true)) {
                tasBuzz.setBUZZ("0");
                buzz_flag = false;
            }                    

            String resp_body = "";
            resp_body += "{\"rsc\":\"2000\",\"to\":\"\",\"fr\":\"" + nCube.getAeid() + "\",\"pc\":\"\",\"rqi\":\"" + noti_q.rqi[noti_q.pop_idx] + "\"}";
            nCube.response(mqtt, resp_body);

            Serial.println("2000 ---->");
        }
        else if(strRef[strRef_length-1] == "update") {
            if (noti_q.con[noti_q.pop_idx] == "active") {
                OTAClient.start();   // active OTAClient upgrad process

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
//------------------------------------------------------------------------------


void setup() {
    //int sensorValue;
    // configure the LED pin for output mode
    pinMode(ledPin, OUTPUT);
    pinMode(SENSOR_PIN, INPUT);    
    //Serial1.begin(115200);      //uwb init    
    //tasLAMP.init();

    // Initialize serial:
    Serial.begin(9600);
    

    //Init Thermal temperature sensor
    Wire.begin();
    mlx.begin();
    //while(!Serial);

    noti_q.pop_idx = 0;
    noti_q.push_idx = 0;
    upload_q.pop_idx = 0;
    upload_q.push_idx = 0;

    WiFi_init();

    delay(1000);

    byte mac[6];
    WiFi.macAddress(mac);
    //sprintf(mqtt_id, "nCube-%.2X%.2X", mac[1], mac[0]);
    sprintf(mqtt_id, "nCube-%.2X%.2X", mac[5], mac[4]);
    unsigned long seed = mac[4] + mac[5];
    randomSeed(seed);

    delay(500);

    // User Defined setup -------------------------------------------------------
    tasLed.init();
    tasBuzz.init();              //buzzer init
    tasUwb.init();

    delay(500);
    tasLed.setLED("0");
    tasBuzz.setBUZZ("0");
    //--------------------------------------------------------------------------

    delay(500);

    String topic = "/oneM2M/resp/" + AE_ID + CSE_ID + "/json";
    topic.toCharArray(resp_topic, 64);

	topic = "/oneM2M/req" + CSE_ID + "/" + AE_ID + "/json";
    topic.toCharArray(noti_topic, 64);

    nCube.Init(CSE_ID, MOBIUS_MQTT_BROKER_IP, AE_ID);
    mqtt.setServer(MOBIUS_MQTT_BROKER_IP, MOBIUS_MQTT_BROKER_PORT);
    mqtt.setCallback(mqtt_message_handler);
    MQTT_State = _MQTT_INIT;

    buildResource();
    rand_str(req_id, 8);
    nCube_State = NCUBE_REQUESTED;

    //init OTA client
    OTAClient.begin(AE_NAME, FIRMWARE_VERSION);
}


void loop() {
    // nCube loop
    nCube_loop();

    // user defined loop
    notiProcess();

    // Sensor processes    
    tempGenProcess();
    uwbGenProcess();    
}

//------------------------------------------------------------------------------
// nCube core functions
//------------------------------------------------------------------------------

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
    otaProcess();
    uploadProcess();
}

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

void WiFi_init() {
    if(USE_WIFI) {
        // begin WiFi
        digitalWrite(ledPin, HIGH);
        WiFi.setPins(WINC_CS, WINC_IRQ, WINC_RST, WINC_EN);
        if (WiFi.status() == WL_NO_SHIELD) { // check for the presence of the shield:
            Serial.println("WiFi shield not present");
            // don't continue:
            while (true) {
                digitalWrite(ledPin, HIGH);
                delay(100);
                digitalWrite(ledPin, LOW);
                delay(100);
            }
        }
        digitalWrite(ledPin, LOW);
    }

    WIFI_State = WIFI_INIT;
}

void WiFi_chkconnect() {
    if(USE_WIFI) {
        if(WIFI_State == WIFI_INIT) {
            digitalWrite(ledPin, HIGH);

            Serial.println("beginProvision - WIFI_INIT");
            WiFi.beginProvision();
            //WiFi.begin("FILab", "badacafe00");
            //WiFi.begin("Laon_R&D", "development");
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
                        Serial.println("Provisioning......");
                    }
                }
                else {
                    if(wifi_wait_count % 2) {
                        digitalWrite(ledPin, HIGH);
                    }
                    else {
                        digitalWrite(ledPin, LOW);
                    }
                }
            }
            else {
                if (WiFi.status() == WL_CONNECTED) {
                    // you're connected now, so print out the status:
                    printWiFiStatus();

                    digitalWrite(ledPin, LOW);

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
            digitalWrite(ledPin, HIGH);

            unsigned long currentMillis = millis();
            if (currentMillis - wifi_previousMillis >= wifi_led_interval) {
                wifi_previousMillis = currentMillis;
                if(wifi_wait_count++ >= wifi_interval) {
                    wifi_wait_count = 0;
                    if (WiFi.status() != WL_CONNECTED) {
                        Serial.print("Attempting to connect to SSID: ");
                        Serial.println("previous SSID");

                        WiFi.begin();
                    }
                }
                else {
                    if(wifi_wait_count % 2) {
                        digitalWrite(ledPin, HIGH);
                    }
                    else {
                        digitalWrite(ledPin, LOW);
                    }
                }
            }
            else {
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("Connected to wifi");
                    printWiFiStatus();

                    digitalWrite(ledPin, LOW);

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

void otaProcess() {
    if(WIFI_State == WIFI_CONNECTED && MQTT_State == _MQTT_CONNECTED) {
        if (OTAClient.finished()) {
        }
        else {
            OTAClient.poll();
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

unsigned long mqtt_sequence = 0;
void chkState() {
    unsigned long currentMillis = millis();
    if (currentMillis - chk_previousMillis >= chk_interval) {
        chk_previousMillis = currentMillis;

        system_watchdog++;
        if(system_watchdog > 9000) {
            if(system_watchdog % 2) {
                digitalWrite(ledPin, HIGH);
            }
            else {
                digitalWrite(ledPin, LOW);
            }
        }
        else if(system_watchdog > 18000) {
            NVIC_SystemReset();
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

void mqtt_reconnect() {
    if(WIFI_State == WIFI_CONNECTED && MQTT_State == _MQTT_CONNECT) {
        unsigned long currentMillis = millis();
        if (currentMillis - mqtt_previousMillis >= mqtt_led_interval) {
            mqtt_led_interval = mqtt_base_led_interval + random(mqtt_base_led_interval);
            mqtt_previousMillis = currentMillis;
            if(mqtt_wait_count++ >= (mqtt_interval)) {
                mqtt_wait_count = 0;

                Serial.print("Attempting MQTT connection...");
                // Attempt to connect
                //rand_str(mqtt_id, 8);
                if (mqtt.connect(mqtt_id)) {
                    mqtt_watchdog_count = 0;
                    Serial.println("connected");

                    if (mqtt.subscribe(resp_topic)) {
                        Serial.println(String(resp_topic) + " Successfully subscribed");
                    }

                    if (mqtt.subscribe(noti_topic)) {
                        Serial.println(String(noti_topic) + " Successfully subscribed");
                    }

                    MQTT_State = _MQTT_CONNECTED;
                    nCube.reset_heartbeat();
                }
                else {
                    Serial.print("failed, rc=");
                    Serial.print(mqtt.state());
                    Serial.println(" try again in 2 seconds");
                    mqtt_watchdog_count++;
                    if(mqtt_watchdog_count > 10) {
                        NVIC_SystemReset();
                    }
                }
            }
            else {
                if(mqtt_wait_count % 2) {
                    digitalWrite(ledPin, HIGH);
                }
                else {
                    digitalWrite(ledPin, LOW);
                }
            }
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

        wifiClient.flush();

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

        wifiClient.flush();

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
            if (wifiClient.available()) {
                return;
            }

            uploading_previousMillis = millis();
            UPLOAD_State = UPLOAD_UPLOADING;

            upload_q.rqi[upload_q.pop_idx].toCharArray(req_id, 10);

            //Serial.println("pop : " + String(upload_q.pop_idx));
            //Serial.println("push : " + String(upload_q.push_idx));
            //noInterrupts();
            body_str = nCube.createCin(mqtt, upload_q.rqi[upload_q.pop_idx], upload_q.ref[upload_q.pop_idx], upload_q.con[upload_q.pop_idx]);
            wifiClient.flush();
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
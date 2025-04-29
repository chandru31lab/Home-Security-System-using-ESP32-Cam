#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>  

// Replace with your network credentials
const char* ssid = "iotdata"; //WiFi Name
const char* password = "123456789"; //WiFi Password

// Use @myidbot to find out the chat ID of an individual or a group
// You need to click "start" on a bot before it can message you 
// Initialize Telegram BOT
String chatId = "xxxxxxxxxx";
String BOTtoken = "7796789559:AAH_S3e1SmNef7XIbaNU1QxxxxxxxHObM";

bool sendPhoto = false;
bool withFlash = false;
bool motionSensorFlag = false;

WiFiClientSecure clientTCP;

UniversalTelegramBot bot(BOTtoken, clientTCP);

// Define GPIOs
#define PIR 13
#define BUZZER_PIN 12 // Buzzer pin
//#define RELAY 12
//#define DHTPIN 2
#define FLASH_LED 4

//#define DHTTYPE DHT11     // DHT 11

//DHT dht(DHTPIN, DHTTYPE);

//CAMERA_MODEL_AI_THINKER
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


String r_msg = "";
 
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
unsigned long bot_lasttime; // last time messages' scan has been done

void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();

/*String getWeather(){
  
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return "Failed to read from DHT sensor! /weather";
  }
  else {
    return "Temperature: " + String(t) + " Humidity: " + String(h) + ". /weather";
  }  
}

String lightOn(){  
 if (digitalRead(RELAY)) {
  return "Light is already ON. /lightOff";
 }
 else{
  digitalWrite(RELAY, HIGH);
  delay(100);
  return "Light is ON. /lightOff";
 }  
}

String lightOff(){  
 if (digitalRead(RELAY)) {
  digitalWrite(RELAY, LOW);
  delay(100);
  return "Light is OFF. /lightOn";
 }
 else{
  return "Light is already OFF. /lightOn";
 }  
}*/

String sendPhotoTelegram(){

  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  if (withFlash){
  digitalWrite(FLASH_LED, HIGH);
  //digitalWrite(RELAY, HIGH);
  delay(200);
  }

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
   Serial.println("Connected to " + String(myDomain));
    
    String head = "--IotCircuitHub\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--IotCircuitHub\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--IotCircuitHub--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=IotCircuitHub");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()){
          char c = clientTCP.read();
          if (c == '\n'){
            if (getAll.length()==0) state=true; 
            getAll = "";
          } 
          else if (c != '\r'){
            getAll += String(c);
          }
          if (state==true){
            getBody += String(c);
          }
          startTimer = millis();
       }
       if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  if (withFlash){
    digitalWrite(FLASH_LED, LOW);
    //digitalWrite(RELAY, LOW);
    withFlash = false;
  }
  return getBody;
}

void handleNewMessages(int numNewMessages){
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++){
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String fromName = bot.messages[i].from_name;
    if (text == "/photo") {
      sendPhoto = true;
      Serial.println("New photo request");
    }
    if (text == "/photoWithFlash") {
      sendPhoto = true;
      withFlash = true;
      Serial.println("New photo request");
    }
    /*if (text == "/lightOn"){
      String r_msg = lightOn();
      bot.sendMessage(chatId, r_msg, "");
    }
    if (text == "/lightOff"){
      String r_msg = lightOff();
      bot.sendMessage(chatId, r_msg, "");
    }*/
    if (text == "/motionOn"){
      motionSensorFlag = true;
      String r_msg = "Motion Sensor is ON. /motionOff";
      bot.sendMessage(chatId, r_msg, "");
    }
    if (text == "/motionOff"){
      motionSensorFlag = false;
      String r_msg = "Motion Sensor is OFF. /motionOn";
      bot.sendMessage(chatId, r_msg, "");
    }
    /*if (text == "/weather"){
      String r_msg = getWeather();
      bot.sendMessage(chatId, r_msg, "");
    }*/
    if (text == "/start"){
      String welcome = "Welcome to the ESP32-CAM Telegram Motion Camera.\n";
      welcome += "/photo : Takes a new photo\n";
      welcome += "/photoWithFlash : Takes photo with Flash\n";
      //welcome += "/lightOn : Turn ON the Light\n";
      //welcome += "/lightOff : Turn OFF the Light\n";
      welcome += "/motionOn : Turn ON Motion Sensor\n";
      welcome += "/motionOff : Turn OFF Motion Sensor\n";
      //welcome += "/weather : Send Live Weather.\n";
      bot.sendMessage(chatId, welcome, "Markdown");
    }
  }
}

void configInitCamera(){
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

  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
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

  /*
   * UXGA   = 1600 x 1200 pixels
   * SXGA   = 1280 x 1024 pixels
   * XGA    = 1024 x 768  pixels
   * SVGA   = 800 x 600   pixels
   * VGA    = 640 x 480   pixels
   * CIF    = 352 x 288   pixels
   * QVGA   = 320 x 240   pixels
   * HQVGA  = 240 x 160   pixels
   * QQVGA  = 160 x 120   pixels
   */

//  // Drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
 s->set_framesize(s, FRAMESIZE_UXGA);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  //sensor_t *s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);       //flip it back
    s->set_brightness(s, 1);  //up the blightness just a bit
    s->set_saturation(s, -2); //lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_CIF);
}

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);
  delay(1000);
  
  //pinMode(RELAY,OUTPUT);
  pinMode(FLASH_LED,OUTPUT);
  pinMode(PIR,INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
   
  //digitalWrite(RELAY, LOW);
  //digitalWrite(FLASH_LED, LOW);
  
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); 
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

//  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
//  connecting_process_timed_out = connecting_process_timed_out * 2;
//  while (WiFi.status() != WL_CONNECTED) {
//    Serial.print(".");
//    digitalWrite(FLASH_LED, HIGH);
//    delay(250);
//    digitalWrite(FLASH_LED, LOW);
//    delay(250);
//    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
//    if(connecting_process_timed_out == 0) {
//      delay(1000);
//      ESP.restart();
//    }
//  }
//  digitalWrite(FLASH_LED, LOW);
  
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP()); 

  //dht.begin();    // Enabling DHT sensor

  Serial.println("Start configuring and initializing the camera...");
  configInitCamera();
  Serial.println("Successfully configure and initialize the camera.");

  delay(1000);
}

void loop(){
    
  if (sendPhoto){
    Serial.println("Preparing photo");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }
  if(motionSensorFlag){    
    if(digitalRead(PIR) == LOW){
      Serial.println("Motion Detected");
      Serial.println("Preparing photo");
      digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
      sendPhotoTelegram(); 
      sendPhoto = false; 
      digitalWrite(BUZZER_PIN, LOW);
      delay(1000);
    }
  }

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}

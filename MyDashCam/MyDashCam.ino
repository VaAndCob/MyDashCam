/*My Dash Cam App
 * board - AI Thinker ESP32 CAM
 * URL
 * streaming video - http://192.168.4.1:stream
 * set WiFi password - http://192.168.4.1/config?password=newpass
 * get firmware version - http://192.168.4.1/config?version=
 * upload firmware page - http://192.168.4.1:3232/uploadIndex
 * OTA firmware upload - http://192.168.4.1:3232/update
 * 
 EEPROM Address
 0x00 - picture counter 2 bytes
 0x02 - wifi password 16 byte
 */
//Intermal temperature sensor function declaration
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();  //declare intermal temperature sensor functio

#include <esp_camera.h>
#include <WiFi.h>
#include <FS.h>                // SD Card ESP32
#include <SD_MMC.h>            // SD Card ESP32
#include <soc/soc.h>           // Disable brownour problems
#include <soc/rtc_cntl_reg.h>  // Disable brownour problems
#include <driver/rtc_io.h>
#include <EEPROM.h>            // read and write from flash memory
//Add on
#include "camera_server.h"    //response http
#include "OTAWebUpdate.h"

//text overlay library
//#include "fd_forward.h"
//#include "fr_forward.h"
#include "img_converters.h"  
#include "fb_gfx.h"

// Pins for ESP32-CAM
#define flashChannel      0
#define FLASH_PIN         4
#define LED_PIN           33 //internal led
//use RX pin for hard reset

// Camera pins
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
/*
//flash pin
const int pwmfreq = 50000;     // 50K pwm frequency
const int pwmresolution = 9;   // duty cycle bit range
ledcSetup(config.ledc_channel, pwmfreq, pwmresolution);  // configure LED PWM channel
ledcAttachPin(4, config.ledc_channel);            // attach the GPIO pin to the channel
*/
//#define overlaytext; //enable text overlay


//Variable
String res[25] = {"SQUARE-96X96","QQVGA-160x120","QCIF-176x144","HQVGA-240x176","SQUARE-240x240",
"QVGA-320x420","CIF-400x296","HVGA-480x320","VGA-640x480","SVGA-800x600","XGA-1024x768","HD-1280x720",
"SXGA-1280x1024","UXGA-1600x1200","FHD-1920x1080","P_HD-720x1280","P_3MP-867x1536","QXGA-2048x1536",
"QHD-2560x1440","WQXGA-2560x1600","P_FHD-1080x1920","QSXGA-2560x1920","INVALID"};
//WiFi
const char* ssid = "My Dash Cam";
const char* password = "12345678";//default password
IPAddress local_IP(192, 168, 4,1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

//char* overlayText = "Ford Ranger Next Gen Wildtrak [ 3kr-145 bkk ]";
//camera state
uint8_t overtemp = 60;
bool camera_error = false;
bool sdcard_error = false;
bool flash = false;//flash status
//loop timer variable
int64_t startTime = 0;
int64_t startResetTime = 0;
int64_t intervalTime = 1000000;//1 sec save 1 image
u_short pictureNumber = 0;
bool picNoSave = false;
uint32_t entry;
byte waitUploadCounter = 0;//counter to exist from update firmware mode

  
/*------------------*/
void memoryCheck() {
  psramInit();
  Serial.println("\n---------------------------------------\n");
  Serial.printf("Free Internal Heap %d/%d\n",ESP.getFreeHeap(),ESP.getHeapSize());
  Serial.printf("Free SPIRam Heap %d/%d\n",ESP.getFreePsram(),ESP.getPsramSize());
  Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.println("\n---------------------------------------\n");
}
/*------------------
static void rgb_print(dl_matrix3du_t *image_matrix, uint32_t color, const char * str){
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    //fb_gfx_print(&fb, (fb.width - (strlen(str) * 14)) / 2, 10, color, str);
    fb_gfx_print(&fb, (fb.width - (strlen(str) * 14)) / 2, fb.height-20, color, str);
}
/*------------------*/
void startCamera() {
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
  
  /* if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  for larger pre-allocated frame buffer. */
 
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 4;
  } else {  
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  } else {
   Serial.print(res[config.frame_size]);    
   Serial.println("  Camera..OK");
  }

  sensor_t * s = esp_camera_sensor_get();
    s->set_vflip(s, 1); // flip it back
    s->set_hmirror(s, 1);
   // s->set_brightness(s, 1); // up the brightness just a bit
   // s->set_saturation(s, -2); // lower the saturation
    //s->set_framesize(s, FRAMESIZE_SXGA);

}
/*------------------*/
bool startMicroSD() {
 

  // Pin 13 needs to be pulled-up
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html#pull-up-conflicts-on-gpio13
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  if(SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card..OK");
    
    return true;
  } else {
    Serial.println("SD Card not mounted!");
    return false;
  }
}
/*------------------*/
void captureImage(String filename) { // Take a photo and get the data

  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("ERROR: Unable to take a photo");
    camera_error = true;
   return;
  } else {
    camera_error = false;

  }
  //text overlay
#ifdef overlaytext
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
  rgb_print(image_matrix, 0x000000FF, overlayText);
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  bool jpeg_converted = fmt2jpg(image_matrix->item, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len);  
  dl_matrix3du_free(image_matrix); 
#endif
  
  File file = SD_MMC.open(filename.c_str(), "w");
  if(file) {
  #ifdef overlaytext
    file.write(_jpg_buf,_jpg_buf_len);
  #else
    file.write(fb->buf,fb->len); 
  #endif      
    file.close();    
    Serial.printf("%d..",startTime);
    Serial.println(filename);
    sdcard_error = false;
 
  } else {
    Serial.println("ERROR: Unable to write " + filename);
    sdcard_error = true;
  }
  // Return the picture data
  esp_camera_fb_return(fb);
 }
/*------------------*/

void setup() {
  gpio_install_isr_service(0);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable detector
  Serial.begin(115200, SERIAL_8N1, -1, TX);//use only tx pin leave rx free for button
  Serial.setDebugOutput(true);
  memoryCheck();  
  
  //pin setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);//red led on
  pinMode(FLASH_PIN, OUTPUT);
  ledcSetup(flashChannel, 10000, 8);//backlight 8 bit
  ledcAttachPin(FLASH_PIN, flashChannel); 
  //digitalWrite(FLASH_PIN, LOW);//flash off
  ledcWrite(flashChannel, 0);
  pinMode(RX,INPUT);//reset password button  
 //rtc_gpio_hold_en(GPIO_NUM_4);

  Serial.println("My Dash Cam v1.0\n==============");

//initialize the camera
  startCamera();
  startMicroSD(); 

//start wifi station on esp32cam
  EEPROM.begin(128);
  String pwd =  EEPROM.readString(0x02);
  if (pwd != "")
      password = pwd.c_str();//read at address 2
  Serial.printf("WiFi password -> %s\n",password);
  //Serial.println(pwd);
  
  WiFi.softAP(ssid, password);//start ACCESS Point
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  Serial.print("Camera IP Address ->http://");
  Serial.println(WiFi.softAPIP()); //start wifi station

  StartCameraServer();//live stream server
  StartOTAserver();//server for update firmware

//read last picture number
 
  pictureNumber = EEPROM.read(0x00);
  Serial.print("Last image save 'image");
  Serial.print(pictureNumber);
  Serial.println(".jpg'");

 // ftpSrv.begin("RearCam","RearCam"); //login, password for ftp
  digitalWrite(LED_PIN,LOW);//ready
}
/*------------------*/
void loop() {
  // put your main code here, to run repeatedly:
 
  server.handleClient();//update server handle
  
  int64_t currentTime = esp_timer_get_time();
  if (currentTime - startTime > intervalTime) {
     startTime = currentTime;    
    if (!camera_error && !sdcard_error && !updatefirmware) {
     
     pictureNumber++;
     String filename = "/image";
     if(pictureNumber < 10000) filename += "0";
     if(pictureNumber < 1000) filename += "0";
     if(pictureNumber < 100)  filename += "0";
     if(pictureNumber < 10)   filename += "0";
     filename += pictureNumber;
     filename += ".jpg";
  
     captureImage(filename);
  
     if (pictureNumber > 25000) pictureNumber = 0;
     //digitalWrite(FLASH_PIN, LOW);
     ledcWrite(flashChannel, 0);
     flash = false;
     digitalWrite(LED_PIN, !digitalRead(LED_PIN)); //red led blink so everything ok running
    } else {
       //digitalWrite(FLASH_PIN, !digitalRead(FLASH_PIN)); //error blink flash light      
       flash = !flash;
       ledcWrite(flashChannel, 8*flash);
             
       if (updatefirmware) {//updating firmware now
          waitUploadCounter++;
          if (waitUploadCounter > 30) { //30 sec firmware not update
              updatefirmware = false;//stop update
              waitUploadCounter = 0;
             // digitalWrite(FLASH_PIN,LOW);//turn off light
              ledcWrite(flashChannel, 0);
              flash = false;
           }
      }//if updatefirmware
    }//else
    float temp = (temprature_sens_read() - 32) / 1.8;
    Serial.print(temp,2);   //print Celsius temperature and tab
    Serial.println("°C");
    if (temp >= overtemp) {
      ledcWrite(flashChannel, 8);
      Serial.printf("Over temperature shutdown at %d°C\n",overtemp);
      esp_deep_sleep_start();  //sleep shutdown
    } 
  }//1 sec loop  

  if(digitalRead(RX) == LOW) {//RX pin low (push reset button)
     if (currentTime - startResetTime > 5000000) {//5 sec hold
     startResetTime = currentTime;
     Serial.println("Reset WiFi Password");
     setPassword("12345678");//reset default wifi password
     }
     if(!picNoSave) {//press  
    //save picture counter to eepromp when power down reset  
      EEPROM.writeUShort(0x00,pictureNumber);
      EEPROM.commit();
      Serial.println("Last picture counter saved!");
      picNoSave = true; //power down 
      
     }//if !picnosave
  } else {//high
    startResetTime = currentTime; 
    picNoSave = false;

  }
  
}//loop

 
/*------------------*/

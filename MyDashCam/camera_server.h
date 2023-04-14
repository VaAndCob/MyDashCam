#include "http_parser.h"
#include <type_traits>
#include "HardwareSerial.h"
/*------------------*/
#include "esp_http_server.h" 
#include <EEPROM.h> 

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
httpd_handle_t stream_httpd = NULL;

static const char* version = "1.01 Jan 23rd, 2023";//change number everytime update new firmeware

static const char successResponse[] PROGMEM =
"<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success!<br>Rear Cam restarting...";
/*-----------------------*/

/* Camera Server
- streaming video
- change wifi password
- update firmware
/*-----------------------*/
// interup timer to delay httpd response fisnih update password and restart
hw_timer_t *onesec_timer = NULL;
void IRAM_ATTR onTimer(){//timer delay to restart
    ESP.restart();
}
/*--------------------*/
static void setPassword(String pwd) {
  Serial.print("Set a new WiFi password -> ");
  Serial.println(pwd);//new password 
  EEPROM.writeString(0x02,pwd);//write to eeprom at addres 2
  EEPROM.commit(); 
  Serial.println("Password saved. -> Restaring...");
   //start interrupt timer to reset               
  onesec_timer= timerBegin(0, 10, true);//set timer 1 second
  timerAttachInterrupt(onesec_timer, &onTimer, true);
  timerAlarmWrite(onesec_timer, 1000000, true);//1us multiplier
  timerAlarmEnable(onesec_timer);//start imer
}
/*-----------------------*/
//http GET streaming handler
static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];


    int64_t fr_start = 0;
    int64_t fr_ready = 0;


    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while(true){
     
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            fr_start = esp_timer_get_time();
            fr_ready = fr_start;        
                   
            if(fb->width > 400){
                if(fb->format != PIXFORMAT_JPEG){
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                                     
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted){
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            } 
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t ready_time = (fr_ready - fr_start)/1000;
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;

        Serial.printf("MJPG: %uB %ums (%.1ffps)\n",
            (uint32_t)(_jpg_buf_len),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time
            
        );
    }

    last_frame = 0;
    return res;
}

//http GET control handler
static esp_err_t config_handler(httpd_req_t *req){
    char*  buf;
    size_t buf_len;
    char value[15] = "";
    bool pwdchanged = false;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        //key value (password=xxxxxxxx) 
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
          //Update new WiFi password  (http://192.168.4.1/config?password=12345678)
            if (httpd_query_key_value(buf, "password", value, sizeof(value)) == ESP_OK) {
                setPassword(value);//new wifi password set
               
                httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
                return httpd_resp_send(req, NULL, 0);
           //ask firmware version (http://192.168.4.1/config?version=)
        
            }  else if (httpd_query_key_value(buf,"version", value, sizeof(value)) == ESP_OK) {
                Serial.print("Firmware version -> ");
                Serial.println(version);
                httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
                return httpd_resp_send(req,version,strlen(version));//response version

            //add more http get response here           
            } else {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL; 
            }  
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }        
    } else {
       free(buf);
       httpd_resp_send_404(req);
       return ESP_FAIL;
    }//buf lenght

}

// http POST handler
/*
static esp_err_t update_handler(httpd_req_t *req){

   Serial.println("Uploading...");
 
  uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  Serial.print("Sketch space = ");
  Serial.println(maxSketchSpace);
  Serial.print("SPIFFS totabl byte");
  Serial.println(SPIFFS.totalBytes());

  char buf[1000000];
   size_t buf_len = min(req->content_len, sizeof(buf)); 
   Serial.print("req Content-length = ");
   Serial.println(req->content_len);
  // int ret = httpd_req_recv(req, buf, buf_len);//return data length

 /*  
  if (ret <= 0) {  // 0 return value indicates connection closed 
        // Check if timeout occurred 
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            //In case of timeout one can choose to retry calling
             // httpd_req_recv(), but to keep it simple, here we
             //respond with an HTTP 408 (Request Timeout) error 
            free(buf);
            httpd_resp_send_408(req);
            return ESP_FAIL;
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
    
    } else {
 
   Serial.println(buf);



   httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
   httpd_resp_send(req, successResponse, sizeof(successResponse));
   return ESP_OK; 
  
}
*/
//start cmamera server
void StartCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  //config.stack_size=1000000;
  config.server_port = 80;//to hide from another one

  httpd_uri_t stream_uri = {//streaming video
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t config_uri = {//change wifi password , update firmware trigger
    .uri       = "/config",
    .method    = HTTP_GET,
    .handler   = config_handler,
    .user_ctx  = NULL
  };
/*
  httpd_uri_t update_uri = {//change wifi password , update firmware trigger
    .uri       = "/upload",
    .method    = HTTP_POST,
    .handler   = update_handler,
    .user_ctx  = NULL
  };
*/
  
  //Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    httpd_register_uri_handler(stream_httpd, &config_uri);
    //httpd_register_uri_handler(stream_httpd, &update_uri);
 
  }
}

// Function for stopping the webserver
void stop_webserver(httpd_handle_t server)
{
     // Ensure handle is non NULL
     if (server != NULL) {
         // Stop the httpd server
         httpd_stop(server);
     }
}
/*-----------------------*/

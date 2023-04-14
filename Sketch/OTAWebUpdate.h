#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>

WebServer server(3232);

bool updatefirmware = false;
/*
 * Server Index Page
 */
static const char uploadIndex[] PROGMEM =
R"(
<!DOCTYPE html>
<head>
<meta name='viewport' content='width=device-width'/>
</head>
<body>

<h3 style='color:white;background:green'>Firmware Update:</h3>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
<input type='file' name='update'  id='file1' onchange='uploadFile()'>
</form>
<p style='align:center'>
<progress id='progressBar' value='0' max='100' style='width:100%'></progress>
<div id='status' style='text-align:center';>0%</div>
</p>
<script>
function _(el) {
  return document.getElementById(el);
}

function uploadFile() {
  
  let file = _('file1').files[0];
  // alert(file.name+' | '+file.size+' | '+file.type);
  let req = new XMLHttpRequest();
  let formdata = new FormData();
  formdata.append('file1', file);
 
  req.upload.addEventListener('progress', progressHandler, false);
  req.addEventListener('load', completeHandler, false);
  req.addEventListener('error', errorHandler, false);
  req.addEventListener('abort', abortHandler, false);
  req.open('POST', '/update'); 
  req.send(formdata);
}

function progressHandler(event) {
  var percent = (event.loaded / event.total) * 100;
  _('progressBar').value = Math.round(percent);
  _('status').innerHTML = Math.round(percent) + '%';
}

function completeHandler(event) {
  _('status').innerHTML = 'Restarting...';

}

function errorHandler(event) {
  _('status').innerHTML = 'Upload Failed';
}

function abortHandler(event) {
  _('status').innerHTML = 'Upload Aborted';
}
</script>
</body>
</html>
)";
/*
<script>
async function uploadFile() {
    let formData = new FormData();           
    formData.append("file", update.files[0]);
    console.log(formData);
    await fetch('/update', {
      method: "POST", 
      body: formData
    });    
    alert('The file has been uploaded successfully.');
}
</script>
 */
void  StartOTAserver() {

  //open upload page  http://192.168.4.1:3232/uploadIndex
  server.on("/uploadIndex", HTTP_GET, []() {
    Serial.println("Request upload firmware...");
    updatefirmware = true;
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", uploadIndex);
  });

   //handling uploading firmware file http://192.168.4.1:3232/update
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Firmware: %s\n", upload.filename.c_str());
      updatefirmware = true;
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
}

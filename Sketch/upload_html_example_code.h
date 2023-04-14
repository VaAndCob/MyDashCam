/*
<!DOCTYPE html>
<html lang='en'>
<head>
<meta name='viewport' content='width=device-width'/>
</head>
<body>

<h3 style='color:white;background:green'>Firmware Update:</h3>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
<input type='file' name='update'  id='update' onchange='uploadFile()'>
</form>
<p style='align:center'>
<progress id='progressBar' value='0' max='100' style='width:100%'></progress>
<div id='status' style='text-align:center';>0%</div>
</p>

<script>
async function uploadFile() {
 var url = '/update' ;
 var selectedFile = document.getElementById('file1');
 var file = selectedFile.files[0];
 var fd = new FormData(); 
 fd.append("file",file);
 var xhr = new XMLHttpRequest(); 
xhr.upload.addEventListener('progress',progressHandler,false);
xhr.addEventListener('load',completeHandler,false);
xhr.addEventListener('error',errorHandler,false);
xhr.addEventListener('abort',abortHandler,false);
xhr.open('POST',url,true);
xhr.setRequestHeader("X-Requested-With","XMLHttpRequest");
xhr.setRequestHeader("X-File-Name",encodeURIComponent(name));
xhr.setRequestHeader("Content-Type","application/octet-stream");
xhr.send(fd) ;
}

function progressHandler(event) {
  _('loaded_n_total').innerHTML = 'Uploaded ' + event.loaded + ' bytes of ' + event.total;
  var percent = (event.loaded / event.total) * 100;
  _('progressBar').value = Math.round(percent);
  _('status').innerHTML = Math.round(percent) + '%';
}

function completeHandler(event) {
  _('status').innerHTML = event.target.responseText;
  _('progressBar').value = 0; //wil clear progress bar after successful upload
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
*/


/*
static const char uploadHtml[] PROGMEM =
R"(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta name='viewport' content='width=device-width'/>
</head>
<body>
<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>

<h3 style='color:white;background:green'>Firmware Update:</h3>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
<input type='file' name='update'  id='update'>
</form>
<p style='align:center'>
<progress id='progressBar' value='0' max='100' style='width:100%'></progress>
<div id='prg' style='text-align:center';>0%</div>
</p>

<script>  
  $('#update').on('change',function(e){
  e.preventDefault();
  var form = $('#upload_form')[0];
  var data = new FormData(form);
  $.ajax({
  url: '/update',
  type: 'POST',
  data: data,
  contentType: false,
  processData:false,
  xhr: function() {
  var xhr = new window.XMLHttpRequest();
  xhr.upload.addEventListener('progress', function(evt) {
  if (evt.lengthComputable) {
  var per = evt.loaded / evt.total;
  $('#prg').html('progress: ' + Math.round(per*100) + '%');
  $('#progressBar').val(Math.round(per*100));
  }
  }, false);
  return xhr;
  },
  success:function(d, s) {
  console.log('success!')
 },
 error: function (a, b, c) {
 }
 });
 });
 </script>
 
</body>
</html>
)";
*/

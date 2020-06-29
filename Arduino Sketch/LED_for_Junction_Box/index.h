String rgbpage = ""
"<!DOCTYPE html><html><head><title>RGB control</title><meta name='mobile-web-app-capable' content='yes' />"
"<meta name='viewport' content='width=device-width' /></head><body style='margin: 0px; padding: 0px;'>"
"<canvas id='colorspace'></canvas></body>"
"<script type='text/javascript'>"
"(function () {"
" var canvas = document.getElementById('colorspace');"
" var ctx = canvas.getContext('2d');"
" function drawCanvas() {"
" var colours = ctx.createLinearGradient(0, 0, window.innerWidth, 0);"
" for(var i=0; i <= 360; i+=10) {"
" colours.addColorStop(i/360, 'hsl(' + i + ', 100%, 50%)');"
" }"
" ctx.fillStyle = colours;"
" ctx.fillRect(0, 0, window.innerWidth, window.innerHeight);"
" var luminance = ctx.createLinearGradient(0, 0, 0, ctx.canvas.height);"
" luminance.addColorStop(0, '#ffffff');"
" luminance.addColorStop(0.05, '#ffffff');"
" luminance.addColorStop(0.5, 'rgba(0,0,0,0)');"
" luminance.addColorStop(0.95, '#000000');"
" luminance.addColorStop(1, '#000000');"
" ctx.fillStyle = luminance;"
" ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);"
" }"
" var eventLocked = false;"
" function handleEvent(clientX, clientY) {"
" if(eventLocked) {"
" return;"
" }"
" function colourCorrect(v) {"
" return Math.round(1023-(v*v)/64);"
" }"
" var data = ctx.getImageData(clientX, clientY, 1, 1).data;"
" var params = ["
" 'r=' + colourCorrect(data[0]),"
" 'g=' + colourCorrect(data[1]),"
" 'b=' + colourCorrect(data[2])"
" ].join('&');"
" var req = new XMLHttpRequest();"
" req.open('POST', '?' + params, true);"
" req.send();"
" eventLocked = true;"
" req.onreadystatechange = function() {"
" if(req.readyState == 4) {"
" eventLocked = false;"
" }"
" }"
" }"
" canvas.addEventListener('click', function(event) {"
" handleEvent(event.clientX, event.clientY, true);"
" }, false);"
" canvas.addEventListener('touchmove', function(event){"
" handleEvent(event.touches[0].clientX, event.touches[0].clientY);"
"}, false);"
" function resizeCanvas() {"
" canvas.width = window.innerWidth;"
" canvas.height = window.innerHeight;"
" drawCanvas();"
" }"
" window.addEventListener('resize', resizeCanvas, false);"
" resizeCanvas();"
" drawCanvas();"
" document.ontouchmove = function(e) {e.preventDefault()};"
" })();"
"</script></html>";

const char MAIN_page[] PROGMEM = R"=====(
<!doctype html>
<html>

<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      margin: 0;
      font-family: -apple-system,
        system-ui,
        BlinkMacSystemFont,
        "Segoe UI", "Roboto", "Ubuntu",
        "Helvetica Neue", sans-serif;
    }

    .upload-btn-wrapper {
      position: relative;
      overflow: hidden;
      display: inline-block;
    }

    .btn {
      border: 1px solid;
      padding: 8px 20px;
      border-radius: 4px;
    }

    .upload-btn-wrapper input[type=file] {
      font-size: 100px;
      position: absolute;
      left: 0;
      top: 0;
      opacity: 0;
    }
  </style>
</head>

<body>
  <div class="upload-btn-wrapper">
    <button id="uploadBtn" class="btn">Firmware Update</button>
    <input type="file" name="firmware" id="firmware" accept=".bin,.bin.gz" />
  </div>
  <div id="uploadLog"></div>

  <script>
    var countdown = 15 // seconds
    function docReady(fn) {
      // see if DOM is already available
      if (document.readyState === "complete" || document.readyState === "interactive") {
        // call on next available tick
        setTimeout(fn, 1);
      } else {
        document.addEventListener("DOMContentLoaded", fn);
      }
    }
    docReady(function() {
      document.querySelector('#firmware').addEventListener('change', function(e) {
        document.getElementById("uploadBtn").disabled = true;
        var file = this.files[0];
        var fd = new FormData();
        fd.append("firmware", file);
        // These extra params aren't necessary but show that you can include other data.
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '/update', true);
        xhr.upload.onprogress = function(e) {
          if (e.lengthComputable) {
            var percentComplete = parseInt((e.loaded / e.total) * 100);
            console.log(percentComplete + '% uploaded');
            document.getElementById('uploadBtn').innerHTML = 'Uploading: ' + percentComplete + '%';
          }
        };
        xhr.onload = function() {
          if (this.status == 200) {
            console.log('Server got:', this.response);
            setInterval(function() {
              countdown = (countdown - 1);
              document.getElementById('uploadBtn').innerHTML = 'Upload Complete';
              document.getElementById('uploadLog').innerHTML = 'Rebooting in ' + countdown + ' seconds';
              if (countdown == 0) {
                window.location.href = "/";
              }
            }, 1000);
          };
        };
        xhr.send(fd);
      }, false);
    });
  </script>
</body>

</html>
<html>
)=====";
const char MAIN_page[] PROGMEM = R"=====(
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

    .red {
      background-color: #880000;
      color: #fff;
    }

    .tab {
      padding: 10px;
    }
  </style>
</head>

<body>

  <div>
    <button class="btn tablink red" onclick="openTab(event,'tools')">Tools</button>
    <button class="btn tablink" onclick="openTab(event,'update')">Firmware</button>
  </div>

  <div id="tools" class="w3-container w3-border tab">
    <a href="/compileTime">CompileTime</a><br>
    <a href="/readADC">ReadADC</a><br>
    <a href="/testMessage">Send Test Message</a><br>
    <a href="/testCamera">Send Camera Test Message</a><br>
    <a href="/getTime">Get Time</a><br>
    <a href="/reboot">Reboot</a><br> 
  </div>
  
  <div id="update" class="w3-container w3-border tab" style="display:none">
  <div class="upload-btn-wrapper">
    Select a new firmware .bin file to update the firmware on this device.
    <hr>
    <button id="uploadBtn" class="btn">Firmware Update</button>
    <input type="file" name="firmware" id="firmware" accept=".bin,.bin.gz" />
  </div>
  <div id="uploadLog"></div>
</div>

<script>
  var countdown = 15 // seconds
  function openTab(evt, tabName) {
    var i, x, tablinks;
    x = document.getElementsByClassName("tab");
    for (i = 0; i < x.length; i++) {
      x[i].style.display = "none";
    }
    tablinks = document.getElementsByClassName("tablink");
    for (i = 0; i < x.length; i++) {
      tablinks[i].className = tablinks[i].className.replace(" red", "");
    }
    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " red";
  }

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
      console.log(this, e)
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
          alert(this.response)
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

)=====";

// Disable console for production or comment this line out to enable it for debugging.
// console.log = function() {};

var msg;
var deviceIDs = ["0"];
var deviceNum;
var doorTimer;
var settings;
var default_settings = { "login":"", "token":"" };

Pebble.addEventListener('ready',function(e) {
  try { 
    settings = JSON.parse(localStorage.settings);
    if (typeof settings.login != "string") settings = default_settings;
   } catch(err) { 
    settings = default_settings; 
  }
  localStorage.settings = JSON.stringify(settings);
  console.log("Settings are: " + localStorage.settings);
  getDeviceList();
});

Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received from Pebble: ' + JSON.stringify(e.payload));
  clearTimeout(doorTimer);
	var command = e.payload.command || 'query';
	deviceNum = e.payload.device;
  console.log(command, deviceNum, deviceIDs[deviceNum]);
  var xhr = new XMLHttpRequest();
  var URL;
  if (command != "clear") {
    if (command != "query") {
      URL = "https://api.particle.io/v1/devices/" + deviceIDs[deviceNum] + "/setState";
      xhr.open("POST", URL , false);
      var data = "access_token="+settings.token+"&arg="+command;
      xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");  
      console.log("Calling " + URL);
      console.log("Data: " + data);
      xhr.send(data);
      console.log(xhr.responseText);
    }
    updateDoorStatus();
  }
});

Pebble.addEventListener("showConfiguration", function() {
  var uri = "https://www.garadget.com/pebble/config/#" + encodeURIComponent('{"login":"' + settings.login + '"}');
  console.log("Configuration url: " + uri);
  Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    console.log("WebViewClosed's response: " + e.response);
    if(e.response != "CANCELLED" && e.response != "{}") {
      try { 
        settings = JSON.parse(e.response); 
      } catch(err) { 
        settings = default_settings;
        console.log("Parsing error: " + err.message);
        console.log(e.response);
      }
    console.log("Configuration window returned: " + JSON.stringify(settings));
    localStorage.settings = JSON.stringify(settings);
    }
    getDeviceList();
  });

function getDeviceList() {
  if (settings.token === "") 
     msg = {"command" : "no config"};
  else {
    try {
      var URL = "https://api.particle.io/v1/devices?access_token=" + settings.token;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", URL , false);
      xhr.send();
      console.log("Calling " + URL);
      console.log("Device list response = " + xhr.responseText);
      var deviceList = JSON.parse(xhr.responseText);
      var deviceNames = ["0"];
      deviceNum=0;
      for (var count=0; count<10; count++) {
        console.log(typeof deviceList[count]);
        if (typeof deviceList[count] == "object") {
          if ((typeof deviceList[count].product_id == "number") && (deviceList[count].product_id == 355))
          console.log(typeof deviceList[count].name);
          console.log(deviceList[count].name);
          console.log(typeof deviceList[count].id);
          console.log(deviceList[count].id);
          console.log(typeof deviceList[count].product_id);
          console.log(deviceList[count].product_id);
          console.log(typeof deviceList[count].connected);
          console.log(deviceList[count].connected);
          deviceIDs[deviceNum] = deviceList[count].id; 
          deviceNames[deviceNum++] = deviceList[count].connected ? deviceList[count].name : "("+deviceList[count].name+")"; 
        } else
          deviceNames[deviceNum++] = "";
      }
      msg = { "command" : "list",
        "device0": deviceNames[0], "device1": deviceNames[1], "device2": deviceNames[2], "device3": deviceNames[3], "device4": deviceNames[4],
        "device5": deviceNames[5], "device6": deviceNames[6], "device7": deviceNames[7], "device8": deviceNames[8], "device9": deviceNames[9] };
      }
    catch(err) {
      msg = {"command" : "no token"};
    }  
  }
  console.log(JSON.stringify(msg));
  sendMessage(msg);
}

function updateDoorStatus() {
  var msg, status = "";
  try {
    var xhr = new XMLHttpRequest();
    var URL = "https://api.particle.io/v1/devices/" + deviceIDs[deviceNum] + "/doorStatus?access_token=" + settings.token;
    xhr.open("GET", URL , false);
    xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");  
    console.log("Calling: " + URL);
    xhr.send();
    console.log("Response: " + xhr.responseText);
    var result = JSON.parse(xhr.responseText).result.split("|");
    console.log(result[0]);
    console.log(result[1]);
    console.log(result[2]);
    console.log(result[3]);
    status = result[0].split("=");
    var time = result[1].split("=");
    var sensor = result[2].split("=");
    var signal = result[3].split("=");
    msg = {"command" : "status", "device0": status[1], "device1": time[1], "device2": sensor[1], "device3": signal[1]};
  } catch(err) {
    msg = {"command" : "status", "device0": "???", "device1": "", "device2": "", "device3": ""};
  }
  console.log(JSON.stringify(msg));
  sendMessage(msg);
  console.log("Status ends: "+ status[1].substr(-3,3)); 
  if (status[1].substr(-3,3) == "ing") 
    doorTimer = setTimeout(updateDoorStatus, 10000);
}
    
function sendMessage(dict) {
  Pebble.sendAppMessage(dict, appMessageAck, appMessageNack);
  console.log("Sent message to Pebble! " + JSON.stringify(dict));
}

function appMessageAck(e) {
  console.log("Message accepted by Pebble!");
}

function appMessageNack(e) {
  console.log("Message rejected by Pebble! " + e.data.error.message);
}


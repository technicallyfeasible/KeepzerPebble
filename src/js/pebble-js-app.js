var configUri = "http://10.100.81.4:61722/other/pebble";
var sensorUri = "http://10.100.81.4:61722/sensors/v1";

var initialised = false;
var connecting = false;
var options = {
	"items": []			// items available for the user
};
var keytoken = "";		// token to use for pushing items into the store

var messages = [];
var isSending = false;

function appMessageAck(e) {
	isSending = false;
	sendMessages();
}
function appMessageNack(e) {
	isSending = false;
    console.log("Error sending message: " + e.error.message);
}
function sendMessages() {
	if(isSending || messages.length == 0) return;
	isSending = true;
	var message = messages[0];
	messages.splice(0, 1);
	Pebble.sendAppMessage(message, appMessageAck, appMessageNack);
}
function addMessage(message) {
	messages.push(message);
	sendMessages();
}

function sendItem(index, name, length) {
	var message = {
		"type": "item",
		"item": index,
		"itemName": name,
		"itemCount": length
	};
	addMessage(message);
}
function sendItems() {
	if(!options || !options.items)
		return;
	
	for(var i = 0; i < options.items.length; i++)
		sendItem(i, options.items[i].name, options.items.length);
}

function sendAccountToken(token) {
	var message = {
		"type": "account_token",
		"accountToken": token
	};
	addMessage(message);
}

function sendSensorId(sensorId) {
	var message = {
		"type": "sensorid",
		"sensorId": sensorId
	};
	addMessage(message);
}
function sendKeyToken(token) {
	var message = {
		"type": "keytoken",
		"keyToken": token
	};
	addMessage(message);
}

function logItem(index) {
}

function hex2byte(c)
{
	var charCode = c - 48;
	if (charCode > 15)
		charCode -= 7;
	if (charCode > 15)
		charCode -= 32;
	if (charCode < 0 || charCode > 15)
		return 0;
	return charCode;
}

function hex2base64 (data) {
  var b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
  var o1, o2, o3, h1, h2, i = 0, ac = 0, enc = "", tmp_arr = [];

  if (!data) {
    return data;
  }

  do { // pack three quartets into two hexets
    o1 = hex2byte(i < data.length ? data.charCodeAt(i++) : 48);
    o2 = hex2byte(i < data.length ? data.charCodeAt(i++) : 48);
    o3 = hex2byte(i < data.length ? data.charCodeAt(i++) : 48);

    h1 = (o1 << 2) | (o2 >> 2);
    h2 = ((o2 & 0x03) << 4) | o3;

    // use hexets to index into b64, and append result to encoded string
    tmp_arr[ac++] = b64.charAt(h1) + b64.charAt(h2);
  } while (i < data.length);

  enc = tmp_arr.join('');

  var r = (data.length % 3);

  return (r == 1 ? enc.slice(0, -1) : enc) + '==='.slice(0, r);
}

function connectKeepzer(sensorId) {
	if (connecting || !sensorId)
		return;
	connecting = true;

	console.log("Connecting sensor " + sensorId);

	var req = new XMLHttpRequest();
	req.open('GET', sensorUri + '/discover/connect?maker=a1b962c3-ce24-45be-9ee4-093692cbef79&id=' + encodeURIComponent(sensorId), true);
	req.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
	req.onload = function(e) {
		if (req.readyState == 4 && req.status == 200) {
			if (req.status == 200) {
				var response = JSON.parse(req.responseText);
				if (!response.isError && response.key) {
					keyValue = response.key;
					console.log("Connected. Key: " + keyValue);
					sendKeyToken(keyValue);
				} else {
					if (!connecting)
						return;
					// still waiting
					connecting = false;
					connectKeepzer(sensorId);
				}
			} else {
				console.log("Connect error: " + response.errorCode + ", " + response.errorMessage);
			}
		}
	};
	req.send(null);
}

function cancelConnect() {
	connecting = false;
}

function storeItemKeepzer(index) {
	console.log("Storing item " + index);
	if (!keytoken || options.items.length <= index || index < 0) {
		console.log("Invalid item or not connected.");
		return;
	}

	var item = options.items[index];

	var req = new XMLHttpRequest();
	req.open('POST', sensorUri + '/data/store', true);
	req.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
	req.setRequestHeader("sensorkey", keytoken);
	req.onload = function(e) {
		if (req.readyState == 4 && req.status == 200) {
			if (req.status == 200) {
				console.log("Item logged: " + req.responseText);
				var response = JSON.parse(req.responseText);
			} else {
				console.log("Error logging item: " + response.errorCode + ", " + response.errorMessage);
			}
		}
	};
	var data = JSON.stringify(item);
	console.log("Sending: " + data);
	req.send(data);
}


Pebble.addEventListener("ready", function() {
    initialised = true;
    options = JSON.parse(window.localStorage.getItem('options'));
});

Pebble.addEventListener("showConfiguration", function() {
	var stringOptions = JSON.stringify(options);
    console.log("Showing config with options: " + stringOptions);
	var uri = configUri + '#' + encodeURIComponent(stringOptions);
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response != '') {
		console.log("Options received: " + e.response);
		var stringOptions = decodeURIComponent(e.response).replace(/[+]/g, ' ');
		options = JSON.parse(stringOptions);
		console.log("Storing options: " + stringOptions);
		window.localStorage.setItem('options', stringOptions);
		sendItems();
    } else {
		console.log("No options received");
    }
});

Pebble.addEventListener("appmessage", function(e) {
	console.log("Received message (type: " + e.payload.type + ", Item: " + e.payload.item + ")");
	switch(e.payload.type) {
		case "keytoken":
			// the watch sends the current keytoken for item logging
			console.log("Received token: " + e.payload.keyToken);
			keytoken = e.payload.keyToken;
			break;
		case "connect":
			var token = Pebble.getAccountToken();
			console.log("Account token: " + token);
			var sensorId = hex2base64(token);
			sendSensorId(sensorId);
			connectKeepzer(sensorId);
			break;
		case "cancel_connect":
			cancelConnect();
			break;
		case "log":
			storeItemKeepzer(e.payload.item);
			break;
	}
  }
);
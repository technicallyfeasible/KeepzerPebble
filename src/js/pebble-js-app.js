var configUri = "http://192.168.1.164:61722/other/pebble";
var sensorUri = "http://192.168.1.164:61722/sensors/v1";
var appId = "a1b962c3-ce24-45be-9ee4-093692cbef79";

var initialised = false;
var connecting = false;
var options = {
	"items": []			// items available for the user
};
var keytoken = "";		// token to use for pushing items into the store

var messages = [];
var isSending = false;

	 
function log(text) {
	//console.log(text);
}

function appMessageAck(e) {
	isSending = false;
	sendMessages();
}
function appMessageNack(e) {
	isSending = false;
    log("Error sending message: " + e.error.message);
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

function sendItem(index, item, length) {
	var message = {
		"type": "item",
		"item": index,
		"itemName": item.name,
		"itemCount": length,
		"dataType": item.dataType,
		"json": item.jsonData
	};
	addMessage(message);
}
function sendItems() {
	if(!options || !options.items)
		return;
	
	for(var i = 0; i < options.items.length; i++)
		sendItem(i, options.items[i], options.items.length);
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

function sendLogResult(result) {
	var message = {
		"type": "log_result",
		"result": result
	};
	addMessage(message);
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

	log("Connecting sensor " + sensorId);

	var req = new XMLHttpRequest();
	req.open('GET', sensorUri + '/discover/connect?appId=' + appId + '&id=' + encodeURIComponent(sensorId), true);
	req.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
	req.onload = function(e) {
		if (req.readyState != 4) return;
		
		if (req.status == 200) {
			var response = JSON.parse(req.responseText);
			if (!response.isError && response.key) {
				keyValue = response.key;
				log("Connected. Key: " + keyValue);
				keytoken = keyValue;
				window.localStorage.setItem('keytoken', keytoken);
				sendKeyToken(keytoken);
			} else {
				if (!connecting)
					return;
				// still waiting
				connecting = false;
				connectKeepzer(sensorId);
			}
		} else {
			log("Connect error: " + req.status);
		}
	};
	req.send(null);
}

function cancelConnect() {
	connecting = false;
}

function storeItemKeepzer(itemDate, itemType, itemJson, done) {
	log("Storing item");
	if (!keytoken) {
		log("No keytoken available, cannot send.");
		if(done) done(0);
		return;
	}

	var req = new XMLHttpRequest();
	req.open('POST', sensorUri + '/data/store', true);
	req.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
	req.setRequestHeader("sensorkey", keytoken);
	req.onload = function(e) {
		if (req.readyState != 4) return; 

		var responseType = req.getResponseHeader('Content-Type');
		var result = 2;
		if (req.status == 200) {
			log("Item logged: " + req.responseText);
			result = 1;
		}
		else {
			log("Error logging item: " + req.status + ", " + req.responseText);
			if (responseType && responseType.indexOf('application/json') != -1) {
				var response = JSON.parse(req.responseText);
				if (response.isError)
					result = (req.status == 401 ? 0 : 3);	// bad item (3) or unauthorized (0)
			}
		}
		if(done) done(result);
	};
	var item = {
		"created": itemDate,
		"dataType": itemType,
		"jsonData": itemJson
	};

	var data = JSON.stringify(item);
	log("Sending: " + data);
	req.send(data);
}


Pebble.addEventListener("ready", function() {
    initialised = true;
    options = JSON.parse(window.localStorage.getItem('options'));
});

Pebble.addEventListener("showConfiguration", function() {
	var stringOptions = JSON.stringify(options);
    log("Showing config with options: " + stringOptions);
	var uri = configUri + '#' + encodeURIComponent(stringOptions);
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    log("configuration closed");
    if (e.response != '') {
		log("Options received: " + e.response);
		var stringOptions = decodeURIComponent(e.response).replace(/[+]/g, ' ');
		options = JSON.parse(stringOptions);
		log("Storing options: " + stringOptions);
		window.localStorage.setItem('options', stringOptions);
	    keytoken = window.localStorage.getItem('keytoken');
		if (keytoken)
			sendKeyToken(keytoken);
		sendItems();
    } else {
		log("No options received");
    }
});

Pebble.addEventListener("appmessage", function(e) {
	log("Received message (type: " + e.payload.type + ")");
	switch(e.payload.type) {
		case "keytoken":
			// the watch sends the current keytoken for item logging
			log("Received token: " + e.payload.keyToken);
			keytoken = e.payload.keyToken;
			window.localStorage.setItem('keytoken', keytoken);
			break;
		case "connect":
			var token = Pebble.getAccountToken();
			log("Account token: " + token);
			var sensorId = hex2base64(token);
			sendSensorId(sensorId);
			connectKeepzer(sensorId);
			break;
		case "cancel_connect":
			cancelConnect();
			break;
		case "log":
			var itemDate = e.payload.date;
			var itemType = e.payload.dataType;
			var itemJson = e.payload.json;
			storeItemKeepzer(itemDate, itemType, itemJson, function(result) {
				log("Logged item: " + result);
				sendLogResult(result);
			});
			break;
	}
  }
);
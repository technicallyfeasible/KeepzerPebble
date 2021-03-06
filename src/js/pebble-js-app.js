var configUri = "https://www.keepzer.com/other/pebble";
var sensorUri = "https://www.keepzer.com/sensors/v1";
var appId = "a1b962c3-ce24-45be-9ee4-093692cbef79";
var batteryType = "keepzer.device.battery";

var initialised = false;
var connecting = false;
var options = {
	"items": []			// items available for the user
};
var keytoken = "";		// token to use for pushing items into the store
var sensorId = "";		// sensor id for this pebble watch

var messages = [];
var isSending = false;

	 
function log(text) {
	//console.log(text);
}
function setItem(key, val) {
	localStorage.setItem('keepzer_' + key, val);
}
function getItem(key) {
	return localStorage.getItem('keepzer_' + key);
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
	if(isSending || messages.length === 0) return;
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
	// send fake item if all were deleted
	if (options.items.length === 0) {
		sendItem(0, {"name":"", "dataType":"", "json":""}, 0);
		return;
	}
	for(var i = 0; i < options.items.length; i++)
		sendItem(i, options.items[i], options.items.length);
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

function sendTimezone() {
	var tz = -(new Date().getTimezoneOffset());
	log('tz: ' + tz);
	var message = {
		"type": "tz",
		"timezoneOffset": tz
	};
	addMessage(message);
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
				var keyValue = response.key;
				log("Connected. Key: " + keyValue);
				keytoken = keyValue;
				setItem('keytoken', keytoken);
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
		keytoken = getItem('keytoken');
		if(!keytoken) {
			log("No keytoken available, cannot send.");
			if(done) done(0);
			return;
		}
		log("Send warning: Keytoken restored");
	}
	if (!itemJson)
	{
		log("Cannot send empty json for item type " + itemType);
		if(done) done(3);
		return;
	}
	// special treatment for double encoded json (can come from Pebble, may be a bug with Pebble)
	else if (itemJson.indexOf("{\\\"") === 0)
		itemJson = itemJson.replace(/\\\"/g, "\"").replace(/\\\\/g, "\\");

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

function getSensorIdKeepzer(done) {
	if (connecting)
		return;

	log("Getting sensor id");

	var req = new XMLHttpRequest();
	req.open('GET', sensorUri + '/discover/uniqueid?appId=' + appId + '&length=6&exclude=lI0O', true);
	req.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
	req.onload = function(e) {
		if (req.readyState != 4) return;
		
		if (req.status == 200) {
			var response = JSON.parse(req.responseText);
			if (!response.isError && response.sensorId) {
				sensorId = response.sensorId;
				setItem('sensorid', keytoken);
			}
		} else {
			log("UniqueId error: " + req.status);
		}
		log("UniqueId: " + sensorId);
		if (done) done(sensorId);
	};
	req.send(null);
}


Pebble.addEventListener("ready", function() {
    initialised = true;
	var stringOptions = getItem('options');
	options = JSON.parse(stringOptions ? stringOptions : '{}');
	log('Loaded options: ' + stringOptions);
	keytoken = getItem('keytoken');
	log('Fetched keytoken: ' + keytoken);
	sendTimezone();
});

Pebble.addEventListener("showConfiguration", function() {
	var stringOptions = getItem('options');
	//var stringOptions = JSON.stringify(options);
    log("Showing config with options: " + stringOptions);
	var uri = configUri + '#' + encodeURIComponent(stringOptions);
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    log("configuration closed");
    if (e.response) {
		log("Options received: " + e.response);
		var stringOptions = decodeURIComponent(e.response).replace(/[+]/g, ' ');
		options = JSON.parse(stringOptions);
		// make sure all string sizes are safe
		if (!options.items) options.items = [];
		for (var i = 0; i < options.items.length; i++) {
			var item = options.items[i];
			if (item.name && item.name.length > 31) item.name = item.name.substring(0, 31);
			if (item.dataType && item.dataType.length > 63) item.dataType = item.dataType.substring(0, 63);
			if (item.jsonData && item.jsonData.length > 127) item.jsonData = item.jsonData.substring(0, 127);
		}
		stringOptions = JSON.stringify(options);
		// store options
		log("Storing options: " + stringOptions);
		setItem('options', stringOptions);
		// get keytoken and send to watch if it exists
		keytoken = getItem('keytoken');
		if (keytoken)
			sendKeyToken(keytoken);
		// get sensor id and send to watch if it exists
		sensorId = getItem('sensorid');
		if (sensorId)
			sendSensorId(sensorId);
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
			setItem('keytoken', keytoken);
			break;
		case "sensorid":
			// the watch sends the current sensorid for item logging
			log("Received sensorId: " + e.payload.sensorId);
			sensorId = e.payload.sensorId;
			setItem('sensorid', sensorId);
			break;
		case "connect":
			if(!sensorId) {
				getSensorIdKeepzer(function(sensorId) {
					sendSensorId(sensorId);
					connectKeepzer(sensorId);
				});
			}
			else
				connectKeepzer(sensorId);
			break;
		case "cancel_connect":
			cancelConnect();
			break;
		case "log":
			var itemDate = e.payload.date;
			var itemType = e.payload.dataType;
			var itemJson = e.payload.json;
			log("Logging date: " + itemDate);
			// battery status unchanged so just send the item
			if (typeof e.payload.battery === "undefined") {
				storeItemKeepzer(itemDate, itemType, itemJson, function(result) {
					log("Logged item: " + result);
					sendLogResult(result);
				});
			} else {
				// battery status has changed so we first send battery and then the item
				log("battery: " + e.payload.battery);
				if (typeof e.payload.battery !== "undefined") {
					var batteryData = { "device": "Pebble", "battery": e.payload.battery + "%" };
					storeItemKeepzer(itemDate, batteryType, JSON.stringify(batteryData), function(result) {
						// if battery sending failed then don't even try the item, just send back failure
						log("Logged battery: " + result);
						if (result != 1 && result != 3) {
							log("Error logging battery, skipping item.");
							sendLogResult(result);
						} else {
							// try the item like normally
							log("Battery sent successfully, sending item: " + itemJson);
							storeItemKeepzer(itemDate, itemType, itemJson, function(result) {
								log("Logged item: " + result);
								sendLogResult(result);
							});
						}
					});
				}
			}
			break;
	}
  }
);
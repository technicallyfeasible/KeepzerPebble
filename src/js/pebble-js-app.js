var configUri = "http://192.168.1.164:61722/other/pebble";
var sensorUri = "http://192.168.1.164:61722/sensors/v1";
var appId = "a1b962c3-ce24-45be-9ee4-093692cbef79";

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

function getSensorIdKeepzer(done) {
	if (connecting)
		return;

	log("Getting sensor id");

	var req = new XMLHttpRequest();
	req.open('GET', sensorUri + '/discover/uniqueid?appId=' + appId + '&length=6', true);
	req.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
	req.onload = function(e) {
		if (req.readyState != 4) return;
		
		if (req.status == 200) {
			var response = JSON.parse(req.responseText);
			if (!response.isError && response.sensorId) {
				sensorId = response.sensorId;
				window.localStorage.setItem('sensorid', keytoken);
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
		// get keytoken and send to watch if it exists
	    keytoken = window.localStorage.getItem('keytoken');
		if (keytoken)
			sendKeyToken(keytoken);
		// get sensor id and send to watch if it exists
	    sensorId = window.localStorage.getItem('sensorid');
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
			log("Received sensorId: " + e.payload.sensorId);
			keytoken = e.payload.keyToken;
			window.localStorage.setItem('keytoken', keytoken);
			//sensorId = e.payload.sensorId;
			//window.localStorage.setItem('sensorid', sensorId);
			break;
		case "sensorid":
			// the watch sends the current sensorid for item logging
			log("Received sensorId: " + e.payload.sensorId);
			sensorId = e.payload.sensorId;
			window.localStorage.setItem('sensorid', sensorId);
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
			storeItemKeepzer(itemDate, itemType, itemJson, function(result) {
				log("Logged item: " + result);
				sendLogResult(result);
			});
			break;
	}
  }
);
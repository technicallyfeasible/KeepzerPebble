var initialised = false;
var options = {
	"items": [],		// items available for the user
	"keytoken": ""		// token to use for pushing items into the store
};
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

function logItem(index) {
}


Pebble.addEventListener("ready", function() {
    initialised = true;
    options = JSON.parse(window.localStorage.getItem('options'));
});

Pebble.addEventListener("showConfiguration", function() {
	var stringOptions = JSON.stringify(options);
    console.log("Showing config with options: " + stringOptions);
	var uri = 'http://192.168.1.164:61722/other/pebble#' + encodeURIComponent(stringOptions);
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
		case "status":
			// fetch connection status
			break;
		case "log":
			// log an item
			break;
		case "message":
			// log an item
			console.log("Message from Pebble: " + e.payload.message);
			break;
		case "account_token":
			sendAccountToken(Pebble.getAccountToken());
			break;
	}
  }
);
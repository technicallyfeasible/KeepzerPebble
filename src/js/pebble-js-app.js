var initialised = false;

function appMessageAck(e) {
    console.log("options sent to Pebble successfully");
}

function appMessageNack(e) {
    console.log("options not sent to Pebble: " + e.error.message);
}

function sendItem(index, name, length) {
	var message = {
		"type": "item",
		"item": index,
		"itemName": name,
		"itemCount": length
	}
	Pebble.sendAppMessage(message, function() { console.log("Item sent"); }, function() { console.log("Item not sent: " + e.error.message); });
}

Pebble.addEventListener("ready", function() {
    initialised = true;
});

Pebble.addEventListener("showConfiguration", function() {
    var options = JSON.parse(window.localStorage.getItem('options'));
    console.log("read options: " + JSON.stringify(options));
    console.log("showing configuration");
    var uri = 'http://www.technicallyfeasible.com/pebble.html#' + encodeURIComponent(JSON.stringify(options));
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response != '') {
		var options = JSON.parse(decodeURIComponent(e.response));
		console.log("storing options: " + JSON.stringify(options));
		window.localStorage.setItem('options', JSON.stringify(options));
		sendItem(0, options.text, 1);
		//Pebble.sendAppMessage(options, appMessageAck, appMessageNack);
    } else {
		console.log("no options received");
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
	}
  }
);
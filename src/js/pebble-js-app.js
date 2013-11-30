var initialised = false;

function appMessageAck(e) {
    console.log("options sent to Pebble successfully");
}

function appMessageNack(e) {
    console.log("options not sent to Pebble: " + e.error.message);
}

Pebble.addEventListener("ready", function() {
    initialised = true;
});

Pebble.addEventListener("showConfiguration", function() {
    var options = JSON.parse(window.localStorage.getItem('options'));
		var stringOptions = JSON.stringify(options);
    console.log("read options: " + stringOptions);
    console.log("showing configuration");
    var uri = 'http://www.technicallyfeasible.com/pebble.html#' + stringOptions;
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response != '') {
			var stringOptions = decodeURIComponent(e.response);
			var options = JSON.parse(stringOptions));
			console.log("storing options: " + stringOptions);
			window.localStorage.setItem('options', stringOptions);
			Pebble.sendAppMessage(options, appMessageAck, appMessageNack);
    } else {
			console.log("no options received");
    }
});

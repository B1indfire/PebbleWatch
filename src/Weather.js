//sets the local storage to default if not found, else set to found item
var temp = this.window.localStorage.getItem('temp') ? this.window.localStorage.getItem('temp') : 'C', 
	time = this.window.localStorage.getItem('time') ? this.window.localStorage.getItem('time') : '12h',
	color = this.window.localStorage.getItem('color') ? this.window.localStorage.getItem('color') : 'white';

//sets up the xhr Request object to send to appweather
var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
	// Construct URL
	var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
	pos.coords.latitude + "&lon=" + pos.coords.longitude;

	// Send request
	xhrRequest(url, 'GET', 
	function(responseText) {
		// parse resopnsetext
		var json = JSON.parse(responseText);

		// Temperature 
		var temperature = Math.round(json.main.temp - 273.15);
		console.log("Temperature is " + temperature);

		// Conditions
		var conditions = json.weather[0].main;      
		console.log("Conditions are " + conditions);
		
		//Icon
		var icon = json.weather[0].id;
		icon=parseIcon(icon);
		console.log("Icon number is: " + icon);
		
		// Assemble dictionary 
		var dictionary = {
			"KEY_TEMPERATURE": temperature,
			"KEY_CONDITIONS": conditions,
			"KEY_ICON": icon
		};

		// Send to Pebble
		Pebble.sendAppMessage(dictionary,
			function(e) {
				console.log("Weather info sent to Pebble successfully!");
			},
			function(e) {
				console.log("Error sending weather info to Pebble!");
			}
		);
	}      
	);
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

function sendConfig(temp, time, color){
	var dictionary = {
			"KEY_TEMP_FORMAT": temp,
			"KEY_TIME_FORMAT": time,
			"KEY_INVERT": color
		};
	Pebble.sendAppMessage(dictionary,
			function(e) {
				console.log("Weather info sent to Pebble successfully!");
			},
			function(e) {
				console.log("Error sending weather info to Pebble!");
			}
		);
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);

//Listens for when the config page needs to be opened.
Pebble.addEventListener('showConfiguration', function() {
	Pebble.openURL('http://jguo14jguo14.web.engr.illinois.edu/');
});

//Listens for when the config page is closed
Pebble.addEventListener('webviewclosed', function(e) {
	var configuration = JSON.parse(decodeURIComponent(e.response));
	temp = configuration.temp;
	time = configuration.time;
	color = configuration.color;
	this.window.localStorage.setItem('temp', temp);
	this.window.localStorage.setItem('time', time);
	this.window.localStorage.setItem('color', color);
	sendConfig(temp, time, color);
});

function parseIcon(number){
	if(number==800)
		return 1;//clear, 01d
	if(number==801)
		return 2;//02d
	if(number==802)
		return 3;//03d
	if(number==803 || number==804)
		return 4;//04d
	if((number>=300 && number<400) || (number>=520 && number<540))
		return 5;//drizzle, 09d
	if(number>=500 && number<505)
		return 6;//rain, 10d
	if(number>=200 && number<300)
		return 7;//Thunderstorm, 11d
	if((number>=600 && number<700) || number==511)
		return 8;//snow, 13d
	if(number>=700 && number<800)
		return 9;//atmosphere, 50d
	
}
//From http://developer.getpebble.com/getting-started/watchface-tutorial/part3/
#include "main.h"
#include "pebble.h"
#include <string.h>
#include "stdio.h"

#define KEY_TEMP_FORMAT 0
#define KEY_TIME_FORMAT 1
#define KEY_INVERT 2
#define KEY_TEMPERATURE 3
#define KEY_CONDITIONS 4
#define KEY_ICON 5


static bool temp_format=false;
static bool time_format=false;
static bool invert=false;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static BitmapLayer *s_wicon_layer;
static GBitmap *s_wicon_bitmap;
static BitmapLayer *s_battery_layer;
static GBitmap *s_battery_bitmap;
static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_bluetooth_bitmap;
static InverterLayer *s_invert_layer;

//INIT CONST ARRAY FOR BATTERY ICON
const int BATTERY_IMAGE[] = {
	RESOURCE_ID_IMAGE_BATT1,
	RESOURCE_ID_IMAGE_BATT2,
	RESOURCE_ID_IMAGE_BATT3,
	RESOURCE_ID_IMAGE_BATT4,
	RESOURCE_ID_IMAGE_BATT5,
	RESOURCE_ID_IMAGE_BATT6,
	RESOURCE_ID_IMAGE_BATT7,
};

/*
Notes:
-For some reason, it doesn't like having a separate date_handler. Weird. Time stops updating if it does. 
*/
// Called once per second
static void minute_handler(struct tm* tick_time, TimeUnits units_changed) {
	 if (!tick_time) {
		time_t now = time(NULL);
		tick_time = localtime(&now);
    }
	static char time_buffer[] = "00:00";
	static char date_buffer[] = "SUN 00/00/00";
	if(time_format){
		strftime(time_buffer, sizeof("00:00"), "%H:%M", tick_time);//24h
	}
	else {
		strftime(time_buffer, sizeof("00:00"), "%I:%M", tick_time);//12h
	}
	//strftime(time_buffer, sizeof(time_buffer), "%R", tick_time);
	text_layer_set_text(s_time_layer, time_buffer);
	
	
	strftime(date_buffer, sizeof("SUN 00/00/00"), "%a %m/%d/%y", tick_time);
	text_layer_set_text(s_date_layer, date_buffer);
	
	// Get weather update every 30 minutes
	if(tick_time->tm_min % 30 == 0) {
		// Begin dictionary
		DictionaryIterator *iter;
		app_message_outbox_begin(&iter);

		// Add a key-value pair
		dict_write_uint8(iter, 0, 0);

		// Send the message!
		app_message_outbox_send();
	}
}

static void date_handler(struct tm *tick_time, TimeUnits units_changed){
	//time_t curr=time(NULL);
  	//struct tm *tick_time = localtime(&curr);
	static char date_buffer[] = "SUN 00/00/00";
	strftime(date_buffer, sizeof("SUN 00/00/00"), "%a %m/%d/%y", tick_time);
	text_layer_set_text(s_date_layer, date_buffer);
	/*
	int date=tick_time->tm_mday;
	int month=tick_time->tm_mon;
	int year=1900+tick_time->tm_year;
	char[] w_day=w_day_to_string(tick_time->tm_wday);
	
	
	strcat(strcat(strcat(strcat(strcat(, date), "/"), month), "/"), year);
	
	text_layer_set_text(s_date_layer, w_day);
	*/
}

static void battery_handler(BatteryChargeState charge_state){
	s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[0]);
	bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
	
	if(charge_state.is_charging) {
		s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[6]);
		bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
	} 
	else {
		switch(charge_state.charge_percent){
			case 100:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[0]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 90:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[0]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 80:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[1]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 70:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[1]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 60:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[2]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 50:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[2]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 40:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[3]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 30:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[3]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 20:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[4]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 10:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[4]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			case 0:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[5]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
			default:
				s_battery_bitmap = gbitmap_create_with_resource(BATTERY_IMAGE[5]);
				bitmap_layer_set_bitmap(s_battery_layer, s_battery_bitmap);
				break;
		}
	}
}

static void bluetooth_handler(bool connected) {
	if(connected){
		s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUE1);
		bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bitmap);
	}
	else {
		s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUE2);
		bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bitmap);
	}
}

//Pulls information from the JSON file from the JS from the weather.
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	static char temperature_buffer[8];
	static char conditions_buffer[32];
	static char weather_layer_buffer[32];
	static int icon_buffer;
	static bool weather=false;

	// Read first item
	Tuple *t = dict_read_first(iterator);

	// For all items
	while(t != NULL) {
		// Which key was received?
		switch(t->key) {
		case KEY_TEMP_FORMAT: //need to change the key numbers
			if(strcmp(t->value->cstring, "C") == 0)//Celsius
				temp_format=false;
			else temp_format=true;
			break;
		case KEY_TIME_FORMAT:
			if(strcmp(t->value->cstring, "12h") == 0)//set time_format to false
				time_format=false;
			else time_format=true;
			break;
		case KEY_INVERT:
			if(strcmp(t->value->cstring, "white") == 0)//set invert to false
				invert=false;
			else invert=true;
			invert=(int)t->value->int32;
			break;
		case KEY_TEMPERATURE:
			weather=true;
			if(temp_format)
				snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)(t->value->int32*1.8)+32);
			else snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
			break;
		case KEY_CONDITIONS:
			snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
			break;
		case KEY_ICON:
			icon_buffer=(int)t->value->int32;
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
			break;
		}

		// Look for next item
		t = dict_read_next(iterator);
	}
	if(weather){
		// Assemble full string and display
		snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
		text_layer_set_text(s_weather_layer, weather_layer_buffer);


		if(icon_buffer==1){//strncmp(icon_buffer, "01d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_01d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==2){//strncmp(icon_buffer, "02d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_02d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==3){//strncmp(icon_buffer, "03d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_03d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==4){//strncmp(icon_buffer, "04d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_04d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==5){//strncmp(icon_buffer, "09d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_09d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==6){//strncmp(icon_buffer, "10d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_10d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==7){//strncmp(icon_buffer, "11d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_11d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==8){//strncmp(icon_buffer, "13d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_13d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else if(icon_buffer==9){//strncmp(icon_buffer, "50d", 4)==0){
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_50d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
		else{
			s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_01d);
			bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
		}
    }

}
//CALLBACKS
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init(void) {
	//REGISTER CALLBACKS
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	//INITIALIZE WINDOW
	s_main_window = window_create();
	window_stack_push(s_main_window, true);
	window_set_background_color(s_main_window, GColorWhite);
	
	 // Init the text layer used to show the time
	s_time_layer = text_layer_create(GRect(0, 40, 144, 55));
	text_layer_set_background_color(s_time_layer, GColorWhite);
	text_layer_set_text_color(s_time_layer, GColorBlack);
	text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	// Ensures time is displayed immediately (will break if NULL tick event accessed).
	
	
	layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_time_layer));
	
	//INITIALIZE DATE
	s_date_layer = text_layer_create(GRect(0, 15, 144, 25));
	text_layer_set_background_color(s_date_layer, GColorWhite);
	text_layer_set_text_color(s_date_layer, GColorBlack);
	text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	/*
	time_t curr = time(NULL);
	struct tm *current_date = localtime(&curr);
	date_handler(current_date, DAY_UNIT);
	tick_timer_service_subscribe(DAY_UNIT, &date_handler);*/
	
	layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_date_layer));
	
	time_t now = time(NULL);
	struct tm *current_time = localtime(&now);
	minute_handler(current_time, MINUTE_UNIT);
	tick_timer_service_subscribe(MINUTE_UNIT, &minute_handler);
	//date_handler(current_time, DAY_UNIT);
	//tick_timer_service_subscribe(DAY_UNIT, &date_handler);
	
	//INITIALIZE WEATHER WINDOW
	s_weather_layer = text_layer_create(GRect(55, 115, 144, 25));
	text_layer_set_background_color(s_weather_layer, GColorWhite);
	text_layer_set_text_color(s_weather_layer, GColorBlack);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
	text_layer_set_text(s_weather_layer, "Loading...");
	text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_get_root_layer(s_main_window), text_layer_get_layer(s_weather_layer));
	
	//INITIALIZE WEATHER ICON LAYER
	s_wicon_layer = bitmap_layer_create(GRect(0, 105, 50, 50));
	bitmap_layer_set_background_color(s_wicon_layer, GColorWhite);
	s_wicon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_50d);
	bitmap_layer_set_bitmap(s_wicon_layer, s_wicon_bitmap);
	layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(s_wicon_layer));
	
	//INITIALIZE BATTERY LAYER
	s_battery_layer = bitmap_layer_create(GRect(120, 0, 20, 20));
	//bitmap_layer_set_background_color(s_battery_layer, GColorWhite);
	BatteryChargeState charge_state = battery_state_service_peek();
	battery_handler(charge_state);
	layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(s_battery_layer));
	
	battery_state_service_subscribe(&battery_handler);
	
	//INITIALIZE BLUETOOTH LAYER
	s_bluetooth_layer = bitmap_layer_create(GRect(0, 0, 11, 14));
	bluetooth_handler(bluetooth_connection_service_peek());
	layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(s_bluetooth_layer));
	
	bluetooth_connection_service_subscribe(&bluetooth_handler);

	//INVERTS THE SCREEN AS PER CONFIG
	if(invert){
		s_invert_layer = inverter_layer_create(GRect(0,0,168,144));
		layer_add_child(window_get_root_layer(s_main_window), inverter_layer_get_layer(s_invert_layer));
	}
	
}

static void deinit(void) {
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	gbitmap_destroy(s_wicon_bitmap);
	bitmap_layer_destroy(s_wicon_layer);
	gbitmap_destroy(s_battery_bitmap);
	bitmap_layer_destroy(s_battery_layer);
	gbitmap_destroy(s_bluetooth_bitmap);
	bitmap_layer_destroy(s_bluetooth_layer);
	if(invert)
		inverter_layer_destroy(s_invert_layer);
	window_destroy(s_main_window);
}
// The main event/run loop for our app
int main(void) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Hello World");
	init();
	
	app_event_loop();
	deinit();
}

//Template taken from Pebble tutorial: https://developer.getpebble.com/getting-started/watchface-tutorial/part1.html
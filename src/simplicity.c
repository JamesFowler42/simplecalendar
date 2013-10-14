#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "common.h"

#define MY_UUID { 0x69, 0x8B, 0x3E, 0x04, 0xB1, 0x2E, 0x4F, 0xF5, 0xBF, 0xAD, 0x1B, 0xE6, 0xBD, 0xFE, 0xB4, 0xD7 }
PBL_APP_INFO(MY_UUID, "Smartwatch Pro", "Max Baeumle", 1, 0 /* App version */, DEFAULT_MENU_ICON, APP_INFO_WATCH_FACE);

AppContextRef app_context;

Window window;

TextLayer text_date_layer;
TextLayer text_time_layer;

Layer line_layer;

HeapBitmap icon_battery;
HeapBitmap icon_bt;
HeapBitmap icon_battery_charge;
HeapBitmap icon_entry_0;
HeapBitmap icon_entry_1;
HeapBitmap icon_entry_2;
HeapBitmap icon_entry_3;
HeapBitmap icon_entry_4;
HeapBitmap icon_entry_5;
HeapBitmap icon_pebble;

Layer battery_layer;
Layer bt_layer;
Layer entry_layer;
Layer pebble_layer;

TextLayer text_event_title_layer;
TextLayer text_event_start_date_layer;
TextLayer text_event_location_layer;

int8_t num_rows;
Event events[15];
uint8_t count;
uint8_t received_rows;
Event event;
Event temp_event;
BatteryStatus battery_status;
char event_date[50];
bool bt_ok = false;
int entry_no = 0;
int max_entries = 0;

bool calendar_request_outstanding = false;

int last_tm_mday = -1;
int last_tm_mday_date = -1;
CloseDay close[7];


/*
 * Unload and return what we have taken
 */
void window_unload(Window *window) {
  heap_bitmap_deinit(&icon_battery);
  heap_bitmap_deinit(&icon_battery_charge);	
  heap_bitmap_deinit(&icon_bt);	
  heap_bitmap_deinit(&icon_entry_0);
  heap_bitmap_deinit(&icon_entry_1);
  heap_bitmap_deinit(&icon_entry_2);
  heap_bitmap_deinit(&icon_entry_3);
  heap_bitmap_deinit(&icon_entry_4);
  heap_bitmap_deinit(&icon_entry_5);
  heap_bitmap_deinit(&icon_pebble);
}

/*
 * Centre line callback handler
 */
void line_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);

  graphics_draw_line(ctx, GPoint(0, 87), GPoint(54, 87));
  graphics_draw_line(ctx, GPoint(0, 88), GPoint(54, 88));
  graphics_draw_line(ctx, GPoint(88, 87), GPoint(143, 87));
  graphics_draw_line(ctx, GPoint(88, 88), GPoint(143, 88));
}

/*
 * Bluetooth icon callback handler
 */
void bt_layer_update_callback(Layer *layer, GContext *ctx) {
  if (bt_ok)
  	graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  else
  	graphics_context_set_compositing_mode(ctx, GCompOpClear);
  graphics_draw_bitmap_in_rect(ctx, &icon_bt.bmp, GRect(0, 0, 9, 12));
}

/*
 * Battery icon callback handler
 */
void battery_layer_update_callback(Layer *layer, GContext *ctx) {
  
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (battery_status.state == 1 && battery_status.level > 0 && battery_status.level <= 100) {
    graphics_draw_bitmap_in_rect(ctx, &icon_battery.bmp, GRect(0, 0, 24, 12));
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(7, 4, (uint8_t)((battery_status.level / 100.0) * 11.0), 4), 0, GCornerNone);
  } else if (battery_status.state == 2 || battery_status.state == 3) {
    graphics_draw_bitmap_in_rect(ctx, &icon_battery_charge.bmp, GRect(0, 0, 24, 12));
  }
}

/*
 * Calendar entry callback handler
 */
void entry_layer_update_callback(Layer *layer, GContext *ctx) {
  	graphics_context_set_compositing_mode(ctx, GCompOpAssign);
	
	if (entry_no == 0)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_0.bmp, GRect(0, 0, 60, 12));
	else if (entry_no == 1)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_1.bmp, GRect(0, 0, 60, 12));
	else if (entry_no == 2)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_2.bmp, GRect(0, 0, 60, 12));
	else if (entry_no == 3)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_3.bmp, GRect(0, 0, 60, 12));
	else if (entry_no == 4)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_4.bmp, GRect(0, 0, 60, 12));
	else if (entry_no == 5)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_5.bmp, GRect(0, 0, 60, 12));
}

/*
 * Pebble word callback handler
 */
void pebble_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, &icon_pebble.bmp, GRect(0, 0, 27, 8));
}

/*
 * Set the bluetooth status indicator
 */
void bt_status(bool new_bt_ok) {
	bt_ok = new_bt_ok;
    layer_mark_dirty(&bt_layer);
}

/*
 * Make a calendar request
 */
void calendar_request(DictionaryIterator *iter) {
  dict_write_int8(iter, REQUEST_CALENDAR_KEY, -1);
  uint8_t clock_style = clock_is_24h_style() ? CLOCK_STYLE_24H : CLOCK_STYLE_12H;
  dict_write_uint8(iter, CLOCK_STYLE_KEY, clock_style);
  bt_status(false);
  count = 0;
  received_rows = 0;
  calendar_request_outstanding = true;
  app_message_out_send();
  app_message_out_release();
}

/*
 * Make a battery status request
 */
void battery_request(DictionaryIterator *iter) {
  dict_write_uint8(iter, REQUEST_BATTERY_KEY, 1);
  bt_status(false);
  app_message_out_send();
  app_message_out_release();
}

/*
 * Display initialisation and further setup 
 */
void handle_init(AppContextRef ctx) {
  app_context = ctx;

  // Window
  window_init(&window, "Window");
  window_set_window_handlers(&window, (WindowHandlers){
    .unload = window_unload,
  });
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  // Resources
  resource_init_current_app(&APP_RESOURCES);
  heap_bitmap_init(&icon_battery, RESOURCE_ID_BATTERY_ICON);
  heap_bitmap_init(&icon_battery_charge, RESOURCE_ID_BATTERY_CHARGE_ICON);	
  heap_bitmap_init(&icon_bt, RESOURCE_ID_BT_LOGO);
  heap_bitmap_init(&icon_entry_0, RESOURCE_ID_ENTRY_0);
  heap_bitmap_init(&icon_entry_1, RESOURCE_ID_ENTRY_1);
  heap_bitmap_init(&icon_entry_2, RESOURCE_ID_ENTRY_2);
  heap_bitmap_init(&icon_entry_3, RESOURCE_ID_ENTRY_3);
  heap_bitmap_init(&icon_entry_4, RESOURCE_ID_ENTRY_4);
  heap_bitmap_init(&icon_entry_5, RESOURCE_ID_ENTRY_5);
  heap_bitmap_init(&icon_pebble, RESOURCE_ID_PEBBLE);

  // Date
  text_layer_init(&text_date_layer, window.layer.frame);
  text_layer_set_text_color(&text_date_layer, GColorWhite);
  text_layer_set_background_color(&text_date_layer, GColorClear);
  layer_set_frame(&text_date_layer.layer, GRect(0, 94, 143, 168-94));
  text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  text_layer_set_text_alignment	(&text_date_layer,GTextAlignmentCenter);
  layer_add_child(&window.layer, &text_date_layer.layer);

  // Time
  text_layer_init(&text_time_layer, window.layer.frame);
  text_layer_set_text_color(&text_time_layer, GColorWhite);
  text_layer_set_background_color(&text_time_layer, GColorClear);
  layer_set_frame(&text_time_layer.layer, GRect(0, 112, 143, 168-112));
  text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  text_layer_set_text_alignment	(&text_time_layer,GTextAlignmentCenter);
  layer_add_child(&window.layer, &text_time_layer.layer);

  // Line
  layer_init(&line_layer, window.layer.frame);
  line_layer.update_proc = &line_layer_update_callback;
  layer_add_child(&window.layer, &line_layer);
 
  // Event title
  text_layer_init(&text_event_title_layer, GRect(0, 18, window.layer.bounds.size.w, 21));
  text_layer_set_text_color(&text_event_title_layer, GColorWhite);
  text_layer_set_background_color(&text_event_title_layer, GColorClear);
  text_layer_set_font(&text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(&window.layer, &text_event_title_layer.layer);

  // Date 
  text_layer_init(&text_event_start_date_layer, GRect(0, 36, window.layer.bounds.size.w, 21));
  text_layer_set_text_color(&text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(&text_event_start_date_layer, GColorClear);
  text_layer_set_font(&text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(&window.layer, &text_event_start_date_layer.layer);

  // Location
  text_layer_init(&text_event_location_layer, GRect(0, 54, window.layer.bounds.size.w, 21));
  text_layer_set_text_color(&text_event_location_layer, GColorWhite);
  text_layer_set_background_color(&text_event_location_layer, GColorClear);
  text_layer_set_font(&text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &text_event_location_layer.layer);

  // Battery
  GRect frame;
  frame.origin.x = layer_get_bounds(&window.layer).size.w - 31;
  frame.origin.y = 6;
  frame.size.w = 24;
  frame.size.h = 12;

  layer_init(&battery_layer, frame);
  battery_layer.update_proc = &battery_layer_update_callback;
  layer_add_child(&window.layer, &battery_layer);

  battery_status.state = 0;
  battery_status.level = -1;

  // Bluetooth 	
  GRect frameb;
  frameb.origin.x = layer_get_bounds(&window.layer).size.w - 50;
  frameb.origin.y = 6;
  frameb.size.w = 9;
  frameb.size.h = 12;

  layer_init(&bt_layer, frameb);
  bt_layer.update_proc = &bt_layer_update_callback;
  layer_add_child(&window.layer, &bt_layer);

  // Calendar entry indicator	
  GRect framec;
  framec.origin.x = 0;
  framec.origin.y = 6;
  framec.size.w = 60;
  framec.size.h = 12;

  layer_init(&entry_layer, framec);
  entry_layer.update_proc = &entry_layer_update_callback;
  entry_no = 0;
  layer_add_child(&window.layer, &entry_layer);
	
  // Pebble word
  GRect framed;
  framed.origin.x = 58;
  framed.origin.y = 83;
  framed.size.w = 27;
  framed.size.h = 8;

  layer_init(&pebble_layer, framed);
  pebble_layer.update_proc = &pebble_layer_update_callback;
  layer_add_child(&window.layer, &pebble_layer);
	
  // Make sure the timers start but don't all go off together
  app_timer_send_event(ctx, 250, REQUEST_BATTERY_KEY);
  app_timer_send_event(ctx, 500, REQUEST_CALENDAR_KEY);
  app_timer_send_event(ctx, 750, ROTATE_EVENT);
}

/*
 * Crude conversion of two character dates to integer
 */
int date_to_i(char *val) {
	return ((val[0]-'0') * 10) + (val[1]-'0');
}

/*
 * Build a cache of dates vs day names
 */
void ensure_close_day_cache() {

	PblTm time;
	get_time(&time);
	
	if (time.tm_mday == last_tm_mday)
		return;
	
	last_tm_mday = time.tm_mday;
	
	for (int i=0; i < 7; i++) {
  	  get_time(&time);
	  if (i>0)
	    time_plus_day(&time, i);
	  string_format_time(close[i].date, sizeof(close[i].date), "%m/%d", &time);
	  string_format_time(close[i].dayName, sizeof(close[i].dayName), "%A", &time);
	}
	strcpy(close[0].dayName, TODAY);
	strcpy(close[1].dayName, TOMORROW);
}

/*
 * Alter the raw date and time returned by iOS to be really nice on the eyes at a glance
 */
void modify_calendar_time(char *output, int outlen, char *date, bool all_day) {
	
	// When "Show next events" is turned off in the app:
    // MM/dd
    // When "Show next events" is turned on:
    // MM/dd/yy

    // If all_day is false, time is added like so:
    // MM/dd(/yy) H:mm
    // If clock style is 12h, AM/PM is added:
    // MM/dd(/yy) H:mm a
	
	// Build a list of dates and day names closest to the current date
    ensure_close_day_cache();
	
	int time_position = 9;
	if (date[5] != '/')
		time_position = 6;

	// Find the date in the list prepared
	char temp[12];
	bool found = false;
	for (int i=0; i < 7; i++) {
		if (strncmp(close[i].date, date, 5) == 0) {
			strcpy(temp, close[i].dayName);
			found = true;
			break;
		}
	}

	// If not found then show the month and the day
	if (!found) {
		PblTm time;
	    get_time(&time);
		time.tm_mday = date_to_i(&date[3]);
		time.tm_mon = date_to_i(&date[0]) - 1;
		string_format_time(temp, sizeof(temp), "%b %e -", &time);
	}
	// Change the format based on whether there is a timestamp
	if (all_day)
		snprintf(output, outlen, "%s %s", temp, ALL_DAY);
	else
	    snprintf(output, outlen, "%s %s", temp, &date[time_position]);
	
}

/*
 * Clear the event display area
 */
void clear_event() {
  text_layer_set_text(&text_event_title_layer, "");
  text_layer_set_text(&text_event_start_date_layer, "");
  text_layer_set_text(&text_event_location_layer, "");
  entry_no = 0;
  layer_mark_dirty(&entry_layer);
}

/*
 * Show new details in the event display area
 */
void show_event(int num) {
  // Copy the right event across
  memset(&event, 0, sizeof(Event));
  memcpy(&event, &events[num - 1], sizeof(Event));

  // Now reload text layers
  text_layer_set_text(&text_event_title_layer, event.title);
  modify_calendar_time(event_date, sizeof(event_date), event.start_date, event.all_day);
  text_layer_set_text(&text_event_start_date_layer, event_date);
  text_layer_set_text(&text_event_location_layer, event.has_location ? event.location : "");

  // Now show which event is being viewed
  entry_no = num;
  layer_mark_dirty(&entry_layer);
}

/*
 * Decide which event to show. Works in a cycle. Clears the display if there is a calendar event outstanding. The phone could
 * have been offline for some while and hence what is being displayed could be inaccurate.
 */
void show_next_event() {
  if (calendar_request_outstanding || max_entries == 0) {
	clear_event();
  } else {
	entry_no++;
	if (entry_no > max_entries || entry_no > 5)
	  entry_no = 1;
	  show_event(entry_no);
  }
}

/*
 * Messages incoming from the phone
 */
void received_message(DictionaryIterator *received, void *context) {
 
   // Guess bluetooth is up to get here
   bt_status(true);

   // Gather the bits of a calendar together	
   Tuple *tuple = dict_find(received, CALENDAR_RESPONSE_KEY);
	  
   if (tuple) {
    	uint8_t i, j;

		if (count > received_rows) {
      		i = received_rows;
      		j = 0;
        } else {
      	    count = tuple->value->data[0];
      	    i = 0;
      	    j = 1;
        }

        while (i < count && j < tuple->length) {
    	    memcpy(&temp_event, &tuple->value->data[j], sizeof(Event));
      	    memcpy(&events[temp_event.index], &temp_event, sizeof(Event));

      	    i++;
      	    j += sizeof(Event);
        }

        received_rows = i;

        if (count == received_rows) {
			max_entries = count;
			calendar_request_outstanding = false;
	    }
	}

	tuple = dict_find(received, BATTERY_RESPONSE_KEY);

    if (tuple) {
        memset(&battery_status, 0, sizeof(BatteryStatus));
        memcpy(&battery_status, &tuple->value->data[0], sizeof(BatteryStatus));

        layer_mark_dirty(&battery_layer);
    }
}

/*
 * Timer handling. Includes a hold off for a period of time if there is resource contention
 */
void handle_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie) {
	
  // Clobber the timer
  app_timer_cancel_event(app_ctx, handle);

  // If we're rotating the visible event, get on with it. Slower overnight to save power
  if (cookie == ROTATE_EVENT) {
    show_next_event();
	  if (is_overnight()) 
    	app_timer_send_event(app_ctx, ROTATE_EVENT_INTERVAL_OVERNIGHT_MS, cookie);
	  else
    	app_timer_send_event(app_ctx, ROTATE_EVENT_INTERVAL_MS, cookie);
	return;
  }

  // If we're going to make a call to the phone, then a dictionary is a good idea.
  DictionaryIterator *iter;
  app_message_out_get(&iter);

  // We didn't get a dictionary - so go away and wait until resources are available
  if (!iter) {
	// Can't get an dictionary then come back in a second
    app_timer_send_event(app_ctx, 1000, cookie);
    return;
  }

  // Make the appropriate call to the server
  if (cookie == REQUEST_BATTERY_KEY) {
	battery_request(iter);
    app_timer_send_event(app_ctx, REQUEST_BATTERY_INTERVAL_MS, cookie);
  } else if (cookie == REQUEST_CALENDAR_KEY) {
	calendar_request(iter);
    app_timer_send_event(app_ctx, REQUEST_CALENDAR_INTERVAL_MS, cookie);
  } 
}

/*
 * Clock tick
 */
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {

    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00";
    static char date_text[] = "Xxxxxxxx xxx 00";
    //                           Thu, Sep 17

    char *time_format;
	
    // Only update the date when it's changed.
	if (t->tick_time->tm_mday != last_tm_mday_date) {
		last_tm_mday_date = t->tick_time->tm_mday;
        string_format_time(date_text, sizeof(date_text), "%a, %b %e", t->tick_time);
        text_layer_set_text(&text_date_layer, date_text);
	}

    if (clock_is_24h_style()) {
      time_format = "%R";
    } else {
      time_format = "%I:%M";
    }

    string_format_time(time_text, sizeof(time_text), time_format, t->tick_time);

    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
      memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }

    text_layer_set_text(&text_time_layer, time_text);
}

/*
 * Main - or at least the pebble land main
 */
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .messaging_info = (PebbleAppMessagingInfo){
      .buffer_sizes = {
        .inbound = 124,
        .outbound = 256,
      },
      .default_callbacks.callbacks = {
        .in_received = received_message,
      },
    },
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    },
    .timer_handler = &handle_timer,
  };
  app_event_loop(params, &handlers);
}

#ifndef common_h
#define common_h

#include "pebble_os.h"

#define RECONNECT_KEY 0
#define REQUEST_CALENDAR_KEY 1
#define CLOCK_STYLE_KEY 2
#define CALENDAR_RESPONSE_KEY 3
#define REQUEST_BATTERY_KEY 8
#define BATTERY_RESPONSE_KEY 9
	
#define ROTATE_EVENT 42

#define CLOCK_STYLE_12H 1
#define CLOCK_STYLE_24H 2

typedef struct {
  uint8_t index;
  char title[21];
  bool has_location;
  char location[21];
  bool all_day;
  char start_date[18];
  int32_t alarms[2];
} Event;

typedef struct {
  uint8_t state;
  int8_t level;
} BatteryStatus;

typedef struct {
  char date[6];
  char dayName[10];
} CloseDay;

#define REQUEST_BATTERY_INTERVAL_MS 120007
#define REQUEST_CALENDAR_INTERVAL_MS 600003
#define ROTATE_EVENT_INTERVAL_MS 3005
#define ROTATE_EVENT_INTERVAL_OVERNIGHT_MS 10005

#define OVERNIGHT_START 0
#define OVERNIGHT_END 6
	
#define TODAY "Today"
#define TOMORROW "Tomorrow"
#define ALL_DAY "All day"
	
void time_plus_day(PblTm *time, int daysToAdvance);
bool is_overnight();

#endif

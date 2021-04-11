
#define WAKEUP_FROM_GPIO    1
#define WAKEUP_FROM_OTHER   2

void enable_sleep();

uint8_t enable_wakeup_source();

int64_t millis();

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "math.h"
#include "boards.h"
#include "led_indication.h"
#include "timer/timer.h"

#define NORMAL_BLINK_INTERVAL MS_TO_OS_TICKS(1000)
#define FAST_BLINK_BASE_INTERVAL MS_TO_OS_TICKS(1000)
#define FAST_BLINK_CORE_INTERVAL MS_TO_OS_TICKS(80)

TIMER_DEF(normal_blink_timer);
TIMER_DEF(fast_blink_timer_base);
TIMER_DEF(fast_blink_timer_core);

void normal_blink_timer_handler()
{
	bsp_board_led_invert(2);
}
void fast_blink_timer_base_handler()
{
	timerStart(&fast_blink_timer_core, FAST_BLINK_CORE_INTERVAL);
}

void fast_blink_timer_core_handler()
{
	bsp_board_led_invert(2);
	static uint8_t blink_count = 0;
	blink_count++;
	if (blink_count == 6)
	{
		timerStop(&fast_blink_timer_core);
		blink_count = 0;
	}
}

void led_indication_set(led_indication_t indication_type)
{
	switch (indication_type)
	{

	case LED_INDICATION_NORMAL_BLINK:
		timerCreate(&normal_blink_timer, normal_blink_timer_handler, TIMER_MODE_PERIODIC);
		timerStart(&normal_blink_timer, NORMAL_BLINK_INTERVAL);
		break;

	case LED_INDICATION_FAST_BLINK:
		timerCreate(&fast_blink_timer_base, fast_blink_timer_base_handler, TIMER_MODE_PERIODIC);
		timerCreate(&fast_blink_timer_core, fast_blink_timer_core_handler, TIMER_MODE_PERIODIC);
		timerStart(&fast_blink_timer_base, FAST_BLINK_BASE_INTERVAL);
		break;

	default:
		break;
	}
}

void led_indication_clear(led_indication_t indication_type)
{
	switch (indication_type)
	{
	case LED_INDICATION_NORMAL_BLINK:
		timerStop(&normal_blink_timer);
		break;

	case LED_INDICATION_FAST_BLINK:
		timerStop(&fast_blink_timer_base);
		timerStop(&fast_blink_timer_core);
		break;

	default:
		break;
	}
}

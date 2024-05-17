#ifndef __SOFT_TIMER_H
#define __SOFT_TIMER_H

/* Macro to define  timer instance */
#define SOFT_TIMER_DEF(name) \
	static softTimer_node_t name;
#define MS_TO_TICKS(ms) (uint32_t)((uint64_t)ms * 32768 / 1000)	   // Macro to convert milliseconds into number of RTC ticks
#define US_TO_TICKS(us) (uint32_t)((uint64_t)us * 32768 / 1000000) // Macro to convert microseconds into number of RTC ticks
typedef enum
{
	SOFT_TIMER_MODE_SINGLE_SHOT,
	SOFT_TIMER_MODE_REPEATED
} soft_timer_mode_t;

typedef void (*timeout_handler_t)(void); // Timeout handler function

typedef struct timer_node
{
	bool trig;
	bool is_running;
	soft_timer_mode_t mode;
	timeout_handler_t timeout_handler;
	uint32_t interval;
	uint32_t trig_ticks;
	struct timer_node *next_node;
} softTimer_node_t; // Timer instance structure

// Function to create a timer by initializing the timer instance
void softTimer_create(softTimer_node_t *instance, timeout_handler_t timeout_handler, soft_timer_mode_t mode);

// Function to start the timer instance by adding it to the list of running timers.
void softTimer_start(softTimer_node_t *instance, uint32_t interval);

// Function to stop the timer instance
void softTimer_stop(softTimer_node_t *instance);

// Function to initialize the timer module
void softTimer_init();

#endif //__SOFT_TIMER_H

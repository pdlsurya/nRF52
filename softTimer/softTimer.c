/**
 * @file softTimer.c
 * @author Surya Poudel
 * @brief Software Timer implementation for NRF52840
 * @version 0.1
 * @date 2020-07-09
 * @copyright Copyright (c) 2020
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "math.h"
#include "boards.h"
#include "nrf_delay.h"
#include "softTimer.h"
#include "debug_log.h"

#define RTC_IRQ_PRIORITY 6 // IRQ Priority level
#define CC1_INT_MASK 1 << 17
#define RTC_PRESCALER 0
#define NRF_RTC_MAX_CNT 0x00FFFFFF // Maximum count of RTC
#define RTC_CC_OFFSET_MIN 3		   // Minimum offset required for CC register from current COUNTER value to generate an event.
#define MAX_TIMERS_CNT 16		   // Maximum number of timer instances allowed

uint8_t timers_count;	  // Number of running timers in the list
uint32_t next_trig_ticks; // RTC ticks to be loaded into CC register
bool rtc_started = false; // rtc_started flag

softTimer_node_t *head_node = NULL; // head node in the list

/*Function to add the timer instance in the queue of running timers.
 *This forms a linked list of running timers
 */
static void timer_list_node_add(softTimer_node_t *instance)
{
	// if there are no timers in the list, assign the head node with first timer instance.
	if (head_node == NULL)
	{
		head_node = instance;
		head_node->next_node = NULL;
		timers_count++;
		return;
	}

	softTimer_node_t *current_node = head_node;
	// traverse over the list until the last node
	while (current_node->next_node != NULL)
	{
		current_node = current_node->next_node;
	}
	current_node->next_node = instance;
	current_node->next_node->next_node = NULL;
	timers_count++;
}

/*Function to delete the timer from the beginning of the list.
 * This removes the head node and reassigns head node with the next timer in the list.
 */
static inline void delete_first_node(softTimer_node_t **head_node)
{
	softTimer_node_t *temp_node = (*head_node)->next_node;
	(*head_node)->next_node = NULL;
	*head_node = temp_node;
}

/*Function to delete the timer from the list
 */
static inline void timer_list_node_delete(softTimer_node_t *instance)
{
	softTimer_node_t *current_node = head_node;

	// if the timer correponds to the head node in the list, remove the timer node and reassign head node.
	if (instance == head_node && head_node->next_node != NULL)
	{
		delete_first_node(&head_node);
	}

	// if there is only one timer node in the list, unlink the node by making head node NULL.
	else if (instance == head_node && head_node->next_node == NULL)
	{
		head_node = NULL;
	}

	else
	{
		while (current_node->next_node != instance)
			current_node = current_node->next_node;

		// if the timer node corresponds to the last node in the list
		if (current_node->next_node->next_node == NULL)
		{
			current_node->next_node = NULL;
			timers_count--;
			return;
		}
		// if timer node corresponds to the node in the middle of the list, remove the node and reassign next_node pointer.
		current_node->next_node = instance->next_node;
		instance->next_node = NULL;
	}
	timers_count--;
}

/*Function to update trig flag of timer nodes to be triggered next.
 */
static inline void update_trig_flag()
{
	softTimer_node_t *current_node = head_node;

	while (current_node != NULL)
	{
		if (current_node->trig_ticks == next_trig_ticks)
			current_node->trig = true;

		// Check if the diffrence between next RTC ticks of timer to be loaded into CC register and the current COUNTER value is less than RTC_CC_OFFSET_MINIMUM
		// When this happens the COMPARE event may not be triggered by the RTC.
		// The nRF52 Series User Specification states that if the COUNTER value is N
		// writing N or N + 1 to a CC register may not trigger a COMPARE event.
		// In this case, the timer can be made to trigger by setting its trig flag manually

		else if ((current_node->trig_ticks - next_trig_ticks) < RTC_CC_OFFSET_MIN)
			current_node->trig = true;
		else
			current_node->trig = false;

		current_node = current_node->next_node;
	}
}
/*Function to create a timer by initizing the timer instance with corresponding timer parameters.
 * This does not start the timer. softTimer_start() function should be called to start
 * the corresponding timer.
 */
void softTimer_create(softTimer_node_t *instance, uint32_t interval, timeout_handler_t timeout_handler, soft_timer_mode_t mode)
{
	instance->interval = interval;
	instance->mode = mode;
	instance->is_running = false;
	instance->timeout_handler = timeout_handler;
	instance->next_node = NULL;
}

/* Function to get the next RTC ticks of the timer node to be triggered.
 * Corresponds to the next RTC ticks to be loaded into CC register.
 */
void get_next_trig_ticks()
{
	// return if no timers are running
	if (head_node == NULL)
		return;
	next_trig_ticks = head_node->trig_ticks;
	softTimer_node_t *current_node = head_node->next_node;

	// Determine the minimum number of RTC ticks from the list of timers that corresponds to the next trigger RTC ticks.
	while (current_node != NULL)
	{
		if (current_node->trig_ticks < next_trig_ticks)
			next_trig_ticks = current_node->trig_ticks;

		current_node = current_node->next_node;
	}
}

/*Function to update the value of CC register
 */
static inline void update_cc_register()
{
	// Check if the next RTC trigger ticks to be loaded into CC register are larger than the maximum COUNTER value of RTC.
	// if true, subtract maximum COUNTER value from the next rtc trigger ticks.
	if (next_trig_ticks > NRF_RTC_MAX_CNT)
	{
		next_trig_ticks = next_trig_ticks - NRF_RTC_MAX_CNT - 1;
		softTimer_node_t *current_node = head_node;
		while (current_node != NULL)
		{
			current_node->trig_ticks = current_node->trig_ticks - NRF_RTC_MAX_CNT - 1;
			current_node = current_node->next_node;
		}
	}

	NRF_RTC2->CC[1] = next_trig_ticks; // load CC register with the ticks corresponding to next timer to be triggered.
}

/* Function to start the timer instances
 * This stores the timer instances in a running timer queue and loads CC register with next minimum RTC ticks.
 */
void softTimer_start(softTimer_node_t *instance)
{
	// check if the timer is already in running state. If true, abort.
	if (instance->is_running)
	{
		return;
	}

	// Check for maximum number of allowed instance to run and generate an error accordingly.
	if (timers_count == MAX_TIMERS_CNT)
	{
		debug_log_print("ERROR:Maximum allowed running timer instances reached");
		return;
	}

	// if RTC is not started, trigger ticks will be equal to the interval ticks
	// Otherwise, next trigger ticks is determined by adding interval ticks to the current RTC counter value
	if (!rtc_started)
		instance->trig_ticks = instance->interval;
	else
		instance->trig_ticks = NRF_RTC2->COUNTER + instance->interval;

	// Set is_running flag for the started timer instance.
	instance->is_running = true;

	// add the timer in the queue of running timers
	timer_list_node_add(instance);
	debug_log_print("Timer cnt=%d", timers_count);
	// get  RTC ticks corresponding to next timers to be triggered.
	get_next_trig_ticks();

	// update cc register;
	update_cc_register();

	// update trig flag of the timers to be triggered next
	update_trig_flag();

	// Check if RTC peripheral is started. If not, start the peripheral and set rtc_stated flag.
	if (!rtc_started)
	{
		NRF_RTC2->TASKS_START = 1;
		rtc_started = true;
	}
}

/* Function to stop the corresponding timer instance.
 * This sets is_running flag to false to prevent subsequent events from this timer.
 */
void softTimer_stop(softTimer_node_t *instance)
{
	if (instance->is_running)
	{
		instance->is_running = false;
		timer_list_node_delete(instance);
		debug_log_print("Timer cnt=%d", timers_count);
	}
}

/*Function to trigger the timers
 */
static inline void trigger_timers()
{

	softTimer_node_t *current_node = head_node;
	while (current_node != NULL)
	{
		softTimer_node_t *temp = current_node->next_node;

		// Check if the trig flag has been set.If set, call the timeout_handler() and update the ticks for next event.
		if (current_node->trig)
		{
			current_node->trig = false;
			current_node->timeout_handler();

			// check if the timer stopped itself in timeout_handler. It true, skip the node and jump to next node.
			if (!current_node->is_running)
			{
				current_node = temp;
				continue;
			}
			// Check if the timer mode is SINGLE_SHOT. If true, stop the correponding timer
			if (current_node->mode == SOFT_TIMER_MODE_SINGLE_SHOT)
			{
				softTimer_stop(current_node);
				current_node = temp;
				continue;
			}
			current_node->trig_ticks = NRF_RTC2->COUNTER + current_node->interval;
		}
		current_node = current_node->next_node;
	}
}

/*
 * Function to handle the timer event generated in RTC1_IRQHandler
 */
void handle_timers()
{
	// check for trig flag and trigger corresponding timers
	trigger_timers();

	// get  RTC ticks corresponding to next timers to be triggered.
	get_next_trig_ticks();

	// update the CC register
	update_cc_register();

	// set the trig flag of all the timers to be triggerd next.
	update_trig_flag();
}

/* Function to initialize the timer module.
 * This initializes the RTC peripheral
 */
void softTimer_init()
{
	NRF_RTC2->INTENSET = CC1_INT_MASK;
	NRF_RTC2->PRESCALER = RTC_PRESCALER;
	NVIC_SetPriority(RTC2_IRQn, RTC_IRQ_PRIORITY);
	NVIC_EnableIRQ(RTC2_IRQn); // Enable RTC2 Interrupt
}

/*Interrrupt handler for RTC1 peripheral */
void RTC2_IRQHandler(void)
{
	NRF_RTC2->EVENTS_COMPARE[1] = 0;
	handle_timers(); // Execute timers
}

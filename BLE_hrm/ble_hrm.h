/*BLE HRM HEADER FILE */

#ifndef BLE_HRM_SRV_H__
#define BLE_HRM_SRV_H__


typedef struct
{
	uint16_t rr_intervals[2]; //RR intervals
	uint8_t rr_intervals_cnt;//RR intervals count

}rr_interval_t; //Structure for rr intervals

#endif // BLE_HRM_SRV_H__

/** @} */

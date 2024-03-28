#include <stdint.h>
#include <stdbool.h>

#include "ble.h"
#include "ble_conn_state.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_srv_common.h"

#include "ble_device_discovery.h"
#include "event_distribution_system.h"

#include "dbg_log.h"

#include "ble_hrm.h"

#define MAX_RR_CNT 2 

typedef struct
{
    uint16_t conn_handle;
    uint16_t hrm_char_handle;
    uint16_t cccd_handle;
}
ble_hrm_c_handles_t; 

static struct
{
    uint8_t val[6];
    size_t dlen;
}
hrm_data;

rr_interval_t rr_data;


static ble_hrm_c_handles_t ble_hrm_c_handles;

static uint16_t ble_val;

EDS_GROUP_DEFINE(BLE_SERVICE_C_HRM);


/**@brief Function for creating a message for writing to the CCCD. */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable)
{
    uint8_t buf[BLE_CCCD_VALUE_LEN];

    buf[0] = enable ? BLE_GATT_HVX_NOTIFICATION : 0;
    buf[1] = 0;

    ble_gattc_write_params_t const write_params =
    {
        .write_op = BLE_GATT_OP_WRITE_REQ,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = cccd_handle,
        .offset   = 0,
        .len      = sizeof(buf),
        .p_value  = buf
    };

    return sd_ble_gattc_write(conn_handle, &write_params);
}

static void device_discovery_evt_handler(size_t event, void *p_ctx)
{
    uint16_t conn_handle = *(uint16_t *)p_ctx;

    switch(event)
    {
        case BLE_DD_STARTED:
            break;
        case BLE_DD_COMPLETED:
        {
            //DBG_PRINT("Device discovery completed!\r\n");

            ble_dd_char_t *hrm_char;
            ble_dd_uuid_t hrm_uuid;

            hrm_uuid.type = BLE_UUID_TYPE_BLE;
            hrm_uuid.value_16 = BLE_UUID_HEART_RATE_MEASUREMENT_CHAR;

            ble_hrm_c_handles.conn_handle = conn_handle;

            hrm_char = ble_dd_get_char(hrm_uuid);
            if(hrm_char != NULL)
            {
                //DBG_PRINT("BLE HRM handle: %u\r\n", hrm_char->char_db.characteristic.handle_value);
                ble_hrm_c_handles.hrm_char_handle = hrm_char->char_db.characteristic.handle_value;
                ble_hrm_c_handles.cccd_handle = hrm_char->char_db.cccd_handle;
            }

            if((ble_hrm_c_handles.conn_handle      == BLE_CONN_HANDLE_INVALID) ||
               (ble_hrm_c_handles.hrm_char_handle   == BLE_GATT_HANDLE_INVALID) ||
               (ble_hrm_c_handles.cccd_handle      == BLE_GATT_HANDLE_INVALID))
            {
                DBG_PRINT("BLE HRM service cannot be recognized!\r\n");
            }
            else
            {
                cccd_configure(ble_hrm_c_handles.conn_handle, ble_hrm_c_handles.cccd_handle, true);
            }

            break;
        }
    }
}
EDS_EVENT_HANDLER_REGISTER(BLE_DEVICE_DISCOVERY, device_discovery_evt_handler);


static void ble_hrm_c_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint16_t role = ble_conn_state_role(p_ble_evt->evt.gap_evt.conn_handle);

    if (role == BLE_GAP_ROLE_CENTRAL)
    {
        switch(p_ble_evt->header.evt_id)
        {
            case BLE_GAP_EVT_CONNECTED:
                break;

            case BLE_GAP_EVT_DISCONNECTED:
                ble_hrm_c_handles.conn_handle = BLE_CONN_HANDLE_INVALID;
                break;

            case BLE_GATTC_EVT_TIMEOUT:
                ble_hrm_c_handles.conn_handle = BLE_CONN_HANDLE_INVALID;
                break;

            // This is for RX
            case BLE_GATTC_EVT_HVX:
            {
                if(ble_hrm_c_handles.conn_handle != BLE_CONN_HANDLE_INVALID)
                {

                    memcpy(hrm_data.val, p_ble_evt->evt.gattc_evt.params.hvx.data, p_ble_evt->evt.gattc_evt.params.hvx.len);
                    hrm_data.dlen = p_ble_evt->evt.gattc_evt.params.hvx.len;
                    LOG_PRINT("ble data length=%d \n",hrm_data.dlen);
                    uint8_t index=0;

                    // Need MCU for processing
                    //pwrmngr_instant_wake_MCU();

                    //DBG_PRINT("HVX on handle %u, length: %u\r\n", p_ble_evt->evt.gattc_evt.params.hvx.handle, p_ble_evt->evt.gattc_evt.params.hvx.len);
                    //DBG_PRINT("HRM DATA: %04X-%02X-%02X-%02X\r\n", hrm_data.val[0],hrm_data.val[1],hrm_data.val[2],hrm_data.val[3]);
                    if(hrm_data.val[index++] & 0x01)
                    {
                        ble_val = hrm_data.val[index+1];
                        ble_val <<= 8;
                        ble_val |= hrm_data.val[index];
                        EDS_EVENT_TRIGGER(BLE_SERVICE_C_HRM, 0, &ble_val);
                        index+=sizeof(uint16_t);
                    }

                    else
                    {
                        ble_val = (uint16_t)hrm_data.val[index++];
                        EDS_EVENT_TRIGGER(BLE_SERVICE_C_HRM, 0, &ble_val);
                    
                    }

                    //check if rr intervals are included in the ble data 
                    if(hrm_data.val[0] & 0x10) //RR interval mask=0x10
                    {
                    	uint8_t i;
                    	for(i=0;i<MAX_RR_CNT;i++)
                    	{
                    		if(index >= hrm_data.dlen)
                    			break;
                    	    rr_data.rr_intervals[i]=hrm_data.val[index+1];
                    	    rr_data.rr_intervals[i] <<= 8;
                    	    rr_data.rr_intervals[i]|=hrm_data.val[index];

                    	    index+=sizeof(uint16_t);
                    	}
                    	rr_data.rr_intervals_cnt=i;
                    	//for(uint8_t k=0;k<rr_data.rr_intervals_cnt;k++)
                    		//LOG_PRINT("rr=%d ms\n",rr_data.rr_intervals[k]);
                    	EDS_EVENT_TRIGGER(BLE_SERVICE_C_HRM,1,&rr_data);

                    }

                }
                else
                {
                    //DBG_PRINT("HRM RX from unknown handle\r\n");
                }

                break;
            }


            case BLE_GATTC_EVT_WRITE_RSP:
            {
                if(ble_hrm_c_handles.conn_handle != BLE_CONN_HANDLE_INVALID)
                {
                    if(ble_hrm_c_handles.cccd_handle == p_ble_evt->evt.gattc_evt.params.write_rsp.handle)
                    {
                        //DBG_PRINT("HRM started!\r\n");
                    }
                }
                else
                {
                    DBG_PRINT("HRM unknown error\r\n");
                }

                break;
            }

            default:
                break;
        }
    }
}

NRF_SDH_BLE_OBSERVER(ble_hrm_c, NRF_SDH_BLE_OBSERVER_PRIO_LEVELS-1, ble_hrm_c_ble_evt, NULL);


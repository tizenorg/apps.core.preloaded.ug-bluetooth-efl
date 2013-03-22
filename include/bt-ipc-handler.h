/*
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BT_IPC_HANDLER_H__
#define __BT_IPC_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt-type-define.h"

/* ================= IPC interface define  ================= */

#define BT_UG_IPC_INTERFACE "User.Bluetooth.UG"
#define BT_UG_IPC_RECIEVER "org.projectx.bluetooth"
#define BT_UG_IPC_REQUEST_OBJECT "/org/projectx/connect_device"
#define BT_UG_IPC_RESPONSE_OBJECT "/org/projectx/response_event"
#define BT_UG_IPC_METHOD_CONNECT "Connect"
#define BT_UG_IPC_METHOD_DISCONNECT "Disconnect"
#define BT_UG_IPC_METHOD_RESPONSE "Response"
#define BT_UG_IPC_METHOD_SEND "Send"
#define BT_UG_IPC_EVENT_CONNECTED "Connected"
#define BT_UG_IPC_EVENT_DISCONNECTED	"Disconnected"

#define BT_SYSPOPUP_IPC_RESPONSE_OBJECT "/org/projectx/bt_syspopup_res"
#define BT_SYSPOPUP_INTERFACE "User.Bluetooth.syspopup"
#define BT_SYSPOPUP_METHOD_RESPONSE "Response"

#define BT_UG_IPC_MSG_LEN				256

typedef enum {
	BT_AUDIO_DEVICE,
	BT_HEADSET_DEVICE,
	BT_STEREO_HEADSET_DEVICE,
	BT_HID_DEVICE,
	BT_NETWORK_DEVICE,
	BT_DEVICE_MAX,
} bt_device_type;

typedef enum {
	BT_IPC_AG_CONNECT_RESPONSE,
	BT_IPC_AG_DISCONNECT_RESPONSE,
	BT_IPC_AV_CONNECT_RESPONSE,
	BT_IPC_AV_DISCONNECT_RESPONSE,
	BT_IPC_HID_CONNECT_RESPONSE,
	BT_IPC_HID_DISCONNECT_RESPONSE,
	BT_IPC_SENDING_RESPONSE,
	BT_IPC_BROWSING_RESPONSE,
	BT_IPC_PRINTING_RESPONSE,
	BT_IPC_PAIRING_RESPONSE,
} bt_ipc_response_t;

typedef enum {
	BT_IPC_SUCCESS,
	BT_IPC_FAIL,
} bt_ipc_result_t;

typedef struct {
	int param1; /* Connect type: Headset / Stereo Headset / HID device */
	char param2[BT_UG_IPC_MSG_LEN];	/* Device address */
} __attribute__ ((packed)) bt_ug_ipc_param_t;

typedef struct {
	int param1; 		/* Reserved */
	char param2[BT_UG_IPC_MSG_LEN];	/* Device address */
	int param3;		/* file count */
	char *param4;		/* File path */
	char *param5;		/* mode */
	char *param6;	/* Device name */
	char *param7;		/* sending type */
} __attribute__ ((packed)) obex_ipc_param_t;

/**
  * Structure to pass notification from BT application
  */
typedef struct {
	int param1;
	int param2;
} bt_ug_param_info_t;

int _bt_ipc_init_event_signal(void *data);

int _bt_ipc_deinit_event_signal(void *data);

int _bt_ipc_register_popup_event_signal(E_DBus_Connection *conn,
					void *data);

int _bt_ipc_unregister_popup_event_signal(E_DBus_Connection *conn,
					  void *data);

int _bt_ipc_send_message(char *method_type, bt_ug_ipc_param_t *param,
			 void *data);

int _bt_ipc_send_obex_message(obex_ipc_param_t *param, void *data);

void _bt_ipc_update_connected_status(bt_ug_data *ugd, int connected_type,
						bool connected, int result,
						bt_address_t *addr);


#ifdef __cplusplus
}
#endif
#endif				/* __BT_IPC_HANDLER_H__ */

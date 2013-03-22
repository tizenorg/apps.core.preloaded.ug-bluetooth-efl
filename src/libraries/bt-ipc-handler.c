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

#include <vconf.h>
#include <bluetooth.h>

#include "bt-main-ug.h"
#include "bt-main-view.h"
#include "bt-ipc-handler.h"
#include "bt-string-define.h"
#include "bt-debug.h"
#include "bt-util.h"

/**********************************************************************
*                                                 Static Functions
***********************************************************************/

int _bt_ipc_init_event_signal(void *data)
{
	FN_START;

	bt_ug_data *ugd;
	E_DBus_Connection *conn;

	retvm_if(data == NULL, BT_UG_FAIL, "Invalid argument: data is NULL\n");

	ugd = (bt_ug_data *)data;
	retvm_if(ugd->EDBusHandle != NULL, BT_UG_FAIL,
	        "Invalid argument: ugd->EDBusHandle already exist\n");

	e_dbus_init();

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	retvm_if(conn == NULL, BT_UG_FAIL, "conn is NULL\n");

	e_dbus_request_name(conn, BT_UG_IPC_INTERFACE, 0, NULL, NULL);

	ugd->EDBusHandle = conn;

	FN_END;
	return TRUE;
}

int _bt_ipc_deinit_event_signal(void *data)
{
	FN_START;

	bt_ug_data *ugd;

	retvm_if(data == NULL, BT_UG_FAIL, "Invalid argument: data is NULL\n");

	ugd = (bt_ug_data *)data;
	retvm_if(ugd->EDBusHandle == NULL, BT_UG_FAIL,
	        "Invalid argument: ugd->EDBusHandle is NULL\n");

	FN_END;
	return TRUE;
}

static void __bt_ipc_receive_popup_event(void *data, DBusMessage * msg)
{
	FN_START;

	int response;
	char *member = NULL;
	bt_ug_data *ugd = NULL;

	retm_if(data == NULL, "Invalid argument: data is NULL\n");
	retm_if(msg == NULL, "Invalid argument: msg is NULL\n");

	ugd = (bt_ug_data *)data;

	member = (char *)dbus_message_get_member(msg);
	retm_if(member == NULL, "dbus signal member get failed\n");

	if (!strcmp(member, BT_SYSPOPUP_METHOD_RESPONSE)) {
		if (!dbus_message_get_args(msg, NULL,
					   DBUS_TYPE_INT32, &response,
					   DBUS_TYPE_INVALID)) {
			BT_DBG("User Event handling for [%s] failed\n", member);
			return;
		} else {
			BT_DBG("Success User Event handling response = %d ",
			       response);
			switch (ugd->confirm_req) {
			case BT_CONNECTION_REQ:
				_bt_main_retry_connection(data, response);
				break;

			case BT_PAIRING_REQ:
				/* response - 0: Yes, 1: No */
				_bt_main_retry_pairing(data, response);
				break;

			case BT_NONE_REQ:
			default:
				BT_DBG("Unidentified request\n");
				break;
			}
		}
	}

	_bt_ipc_unregister_popup_event_signal(ugd->EDBusHandle, data);
}

/**********************************************************************
*                                                Common Functions
***********************************************************************/

int _bt_ipc_register_popup_event_signal(E_DBus_Connection *conn, void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	E_DBus_Signal_Handler *sh = NULL;

	retvm_if(conn == NULL, BT_UG_FAIL, "Invalid argument: conn is NULL\n");
	retvm_if(data == NULL, BT_UG_FAIL, "Invalid argument: data is NULL\n");

	ugd = (bt_ug_data *)data;

	sh = e_dbus_signal_handler_add(conn,
				       NULL,
				       BT_SYSPOPUP_IPC_RESPONSE_OBJECT,
				       BT_SYSPOPUP_INTERFACE,
				       BT_SYSPOPUP_METHOD_RESPONSE,
				       __bt_ipc_receive_popup_event, data);

	retvm_if(sh == NULL, BT_UG_FAIL, "AG Response Event register failed\n");

	ugd->popup_sh = sh;

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_ipc_unregister_popup_event_signal(E_DBus_Connection *conn, void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retvm_if(conn == NULL, BT_UG_FAIL, "Invalid argument: conn is NULL\n");
	retvm_if(data == NULL, BT_UG_FAIL, "Invalid argument: data is NULL\n");

	ugd = (bt_ug_data *)data;

	retvm_if(ugd->popup_sh == NULL, BT_UG_FAIL, "Signal Handler is NULL\n");

	e_dbus_signal_handler_del(conn, ugd->popup_sh);

	ugd->popup_sh = NULL;

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_ipc_send_obex_message(obex_ipc_param_t *param, void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	DBusMessage *msg = NULL;
	char *param_str = NULL;
	DBusPendingCall *ret = NULL;

	retvm_if(param == NULL, BT_UG_FAIL,
		 "Invalid argument: param is NULL\n");
	retvm_if(data == NULL, BT_UG_FAIL, "Invalid argument: data is NULL\n");

	ugd = (bt_ug_data *)data;
	param_str = param->param2;
	retvm_if(ugd->EDBusHandle == NULL, BT_UG_FAIL,
		 "Invalid argument: ugd->EDBusHandle is NULL\n");
	retvm_if(param_str == NULL, BT_UG_FAIL,
		 "Invalid argument: param_str is NULL\n");

	BT_DBG("Request to connect [%d]\n", param->param1);
	BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", param->param2[0],
	       param->param2[1], param->param2[2], param->param2[3],
	       param->param2[4], param->param2[5]);

	msg = dbus_message_new_signal(BT_UG_IPC_REQUEST_OBJECT,
						      BT_UG_IPC_INTERFACE,
						      BT_UG_IPC_METHOD_SEND);

	retvm_if(msg == NULL, BT_UG_FAIL, "msg is NULL\n");

	if (!dbus_message_append_args(msg,
				      DBUS_TYPE_INT32, &param->param1,
				      DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
				      &param_str, BT_ADDRESS_LENGTH_MAX,
				      DBUS_TYPE_INT32, &param->param3,
				      DBUS_TYPE_STRING, &param->param4,
				      DBUS_TYPE_STRING, &param->param5,
				      DBUS_TYPE_STRING, &param->param6,
				      DBUS_TYPE_STRING, &param->param7,
				      DBUS_TYPE_INVALID)) {
		BT_DBG("Connect sending failed\n");
		dbus_message_unref(msg);
		return BT_UG_FAIL;
	}

	ret = e_dbus_message_send(ugd->EDBusHandle, msg, NULL, -1, NULL);
	dbus_message_unref(msg);

	FN_END;
	return ret ? BT_UG_ERROR_NONE : BT_UG_FAIL;
}

void _bt_ipc_update_connected_status(bt_ug_data *ugd, int connected_type,
						bool connected, int result,
						bt_address_t *addr)
{
	FN_START;

	bt_dev_t *item = NULL;
	char addr_str[BT_ADDRESS_STR_LEN + 1] = { 0 };

	_bt_util_addr_type_to_addr_string(addr_str, addr->bd_addr);

	item = _bt_main_get_dev_info_by_address(ugd->paired_device, addr_str);

	if (item == NULL)
		item = _bt_main_get_dev_info(ugd->paired_device, ugd->paired_item);

	ugd->connect_req = FALSE;

	if (item == NULL)
		return;

	item->status = BT_IDLE;

	if (connected == TRUE) {
		item->connected_mask |= (result == BT_UG_ERROR_NONE) ? \
			connected_type : 0x00;
	} else {
		item->connected_mask &= (result == BT_UG_ERROR_NONE) ? \
			~connected_type : 0xFF;
	}

	elm_genlist_item_update((Elm_Object_Item *)item->genlist_item);

	if (!(ugd->profile_vd && ugd->profile_vd->genlist))
		return;

	/* Check if the device update and the Profile view device is same */
	/* Go through the ugd->profile_vd->genlist and check device address */
	bt_dev_t *dev_info = NULL;
	Elm_Object_Item *dev_item;

	dev_item = elm_genlist_first_item_get(ugd->profile_vd->genlist);

	if (dev_item == NULL) {
		BT_DBG("No item in the list \n");
		return;
	}

	while (dev_item != NULL) {
		dev_info = (bt_dev_t *)elm_object_item_data_get(dev_item);

		if (dev_info == NULL)
			dev_item = elm_genlist_item_next_get(dev_item);
		else
			break;
	}

	/* dev_info can be NULL again, so a check is applied */
	if (dev_info == NULL) {
		BT_DBG("No item in the list \n");
		return;
	}

	/* Match the BD address */
	if (g_strcmp0(dev_info->addr_str, addr_str) != 0)
		return;

	dev_info->call_checked = dev_info->connected_mask & \
				BT_HEADSET_CONNECTED;

	dev_info->media_checked = dev_info->connected_mask & \
				BT_STEREO_HEADSET_CONNECTED;

	dev_info->hid_checked = dev_info->connected_mask & \
				BT_HID_CONNECTED;

	dev_info->network_checked = dev_info->connected_mask & \
				BT_NETWORK_CONNECTED;

	_bt_util_set_list_disabled(ugd->profile_vd->genlist,
				EINA_FALSE);
	FN_END;
}

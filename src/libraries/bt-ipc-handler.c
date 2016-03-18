/*
* ug-bluetooth-efl
*
* Copyright 2012 Samsung Electronics Co., Ltd
*
* Contact: Hocheol Seo <hocheol.seo@samsung.com>
*           GirishAshok Joshi <girish.joshi@samsung.com>
*           DoHyun Pyun <dh79.pyun@samsung.com>
*
* Licensed under the Flora License, Version 1.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.tizenopensource.org/license
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <vconf.h>
#include <bluetooth.h>
#include <gio/gio.h>

#include "bt-main-ug.h"
#include "bt-main-view.h"
#include "bt-ipc-handler.h"
#include "bt-string-define.h"
#include "bt-debug.h"
#include "bt-util.h"
#include "bt-widget.h"
#include "bt-callback.h"

static void __bt_on_bus_acquired(GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
	bt_ug_data *ugd = (bt_ug_data *)user_data;

	retm_if(user_data == NULL, "Invalid argument: user_data is NULL");
	retm_if(connection == NULL, "Invalid argument: connection is NULL");

	ugd->g_conn = connection;
}

static void __bt_on_name_acquired(GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
	BT_DBG("Acquired the name %s on the system bus", name);
}

static void __bt_on_name_lost(GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
	BT_DBG("Lost the name %s on the system bus", name);
}

int _bt_ipc_register_popup_event_signal(bt_ug_data *ugd)
{
	FN_START;

	retvm_if(ugd == NULL, BT_UG_FAIL, "Invalid argument: data is NULL");

	ugd->gdbus_owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
				BT_UG_IPC_INTERFACE,
				G_BUS_NAME_OWNER_FLAGS_NONE,
				__bt_on_bus_acquired,
				__bt_on_name_acquired,
				__bt_on_name_lost,
				ugd,
				NULL);
	retvm_if(ugd->gdbus_owner_id == 0, BT_UG_FAIL,
					"Failed registering event signal");

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_ipc_unregister_popup_event_signal(bt_ug_data *ugd)
{
	FN_START;

	retvm_if(ugd == NULL, BT_UG_FAIL, "Invalid argument: data is NULL");

	g_bus_unown_name(ugd->gdbus_owner_id);
	ugd->gdbus_owner_id = 0;

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_ipc_send_obex_message(obex_ipc_param_t *param, void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	GVariantBuilder *filepath_builder;
	GVariantBuilder *bd_addr;
	GError *error = NULL;
	gboolean ret;
	int i;

	retvm_if(param == NULL, BT_UG_FAIL,
		 "Invalid argument: param is NULL");
	retvm_if(data == NULL, BT_UG_FAIL, "Invalid argument: data is NULL");

	ugd = (bt_ug_data *)data;

	bd_addr = g_variant_builder_new(G_VARIANT_TYPE("ay"));
	for (i = 0; i < BT_ADDRESS_LENGTH_MAX; i++)
		g_variant_builder_add(bd_addr, "y", param->addr[i]);

	filepath_builder = g_variant_builder_new(G_VARIANT_TYPE("aay"));
	for (i = 0; i < param->file_cnt; i++)
		g_variant_builder_add(filepath_builder, "^ay", param->filepath[i]);

	ret = g_dbus_connection_emit_signal(ugd->g_conn,
					NULL,
					BT_UG_IPC_REQUEST_OBJECT,
					BT_UG_IPC_INTERFACE,
					BT_UG_IPC_METHOD_SEND,
					g_variant_new("(ayssaay)",
							bd_addr,
							param->dev_name,
							param->type,
							filepath_builder),
					&error);

	if (ret == FALSE) {
		BT_ERR("Unable to connect to dbus: %s", error->message);
		g_clear_error(&error);
	}

	g_variant_builder_unref(filepath_builder);
	g_variant_builder_unref(bd_addr);

	FN_END;
	return ret ? BT_UG_ERROR_NONE : BT_UG_FAIL;
}

void _bt_ipc_update_connected_status(void *data, int connected_type,
						bool connected, int result,
						bt_address_t *addr)
{
	FN_START;

	ret_if(!data);
	ret_if(!addr);
	bt_ug_data *ugd = (bt_ug_data *)data;
	ret_if(ugd->op_status == BT_DEACTIVATING || ugd->op_status == BT_DEACTIVATED);

	bt_dev_t *item = NULL;
	char addr_str[BT_ADDRESS_STR_LEN + 1] = { 0 };

	_bt_util_addr_type_to_addr_string(addr_str, addr->bd_addr);

	item = _bt_main_get_dev_info_by_address(ugd->paired_device, addr_str);

	if (item == NULL)
		item = _bt_main_get_dev_info(ugd->paired_device, ugd->paired_item);

	if (item == NULL)
		return;

	if (connected == TRUE) {
		item->status = BT_IDLE;
		item->connected_mask |= (result == BT_UG_ERROR_NONE) ? \
			connected_type : 0x00;
	} else {
		if (!ugd->disconn_req)
			item->status = BT_IDLE;

		item->connected_mask &= (result == BT_UG_ERROR_NONE) ? \
			~connected_type : 0xFF;
	}

	if (item->connected_mask == 0x00) {
		item->status = BT_IDLE;
		item->is_connected = 0;
	} else {
		item->is_connected = 1;
	}

	BT_DBG("is_connected : %d, connected_mask : 0x%02x",
			item->is_connected, item->connected_mask);

	if (result != BT_UG_ERROR_NONE &&
			item->connected_mask == 0x00) {
		BT_ERR("Connection Failed");
		_bt_update_genlist_item((Elm_Object_Item *)item->genlist_item);
		if (!ugd->profile_vd) {
			Evas_Object *btn1 = NULL;
			Evas_Object *btn2 = NULL;
			_bt_main_popup_del_cb(ugd, NULL, NULL);

			ugd->popup_data.type = BT_POPUP_CONNECTION_ERROR;
			ugd->popup_data.data = g_strdup(item->name);

			if (ugd->connect_req == true) {
				ugd->popup = _bt_create_popup(ugd, NULL, NULL, 0);
				retm_if(ugd->popup == NULL, "fail to create popup!");

				btn1 = elm_button_add(ugd->popup);
				elm_object_style_set(btn1, "popup");
				elm_object_domain_translatable_text_set(
					btn1 ,
					PKGNAME, "IDS_BR_SK_CANCEL");
				elm_object_part_content_set(ugd->popup, "button1", btn1);
				evas_object_smart_callback_add(btn1,
						"clicked", _bt_retry_connection_cb, item);

				btn2 = elm_button_add(ugd->popup);
				elm_object_style_set(btn2, "popup");
				elm_object_domain_translatable_text_set(
					btn2 ,
					PKGNAME, "IDS_ST_BUTTON_RETRY");
				elm_object_part_content_set(ugd->popup, "button2", btn2);
				evas_object_smart_callback_add(btn2,
						"clicked", _bt_retry_connection_cb, item);

				evas_object_data_set(ugd->popup, "bd_addr", (void *)item->addr_str);
				eext_object_event_callback_add(ugd->popup,
					EEXT_CALLBACK_BACK, _bt_retry_connection_cb, item);
				evas_object_show(ugd->popup);
			}
		}

	} else if (item->status == BT_IDLE) {
		/* No need to check for connected state as that is handled in _bt_connect_net_profile api */
		_bt_sort_paired_device_list(ugd, item, item->is_connected);
		_bt_update_genlist_item((Elm_Object_Item *)item->genlist_item);
	}

	ugd->connect_req = FALSE;

	if (ugd->bt_launch_mode == BT_LAUNCH_CONNECT_HEADSET &&
		connected_type == BT_HEADSET_CONNECTED &&
		connected == TRUE &&
		result == BT_UG_ERROR_NONE) {
		BT_DBG("BT_LAUNCH_CONNECT_HEADSET: Connected, destroying UG");
		_bt_ug_destroy(ugd, NULL);
	}

	ret_if(!ugd->profile_vd);
	BT_DBG("is_connected : %d, connected_mask : 0x%02x",
			item->is_connected, item->connected_mask);

	/* Check if the device update and the Profile view device is same */
	/* Go through the ugd->profile_vd->genlist and check device address */
	bt_dev_t *dev_info = NULL;
	Elm_Object_Item *dev_item;

	dev_item = elm_genlist_first_item_get(ugd->profile_vd->genlist);

	if (dev_item == NULL) {
		BT_DBG("No item in the list");
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
		BT_DBG("No item in the list");
		return;
	}

	/* Match the BD address */
	if (g_strcmp0(dev_info->addr_str, addr_str) != 0)
		return;

	dev_info->call_checked = dev_info->connected_mask & \
				BT_HEADSET_CONNECTED;

#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	dev_info->media_checked = dev_info->connected_mask & \
				  BT_MUSIC_PLAYER_CONNECTED;
#else
	dev_info->media_checked = dev_info->connected_mask & \
				BT_STEREO_HEADSET_CONNECTED;
#endif

	dev_info->hid_checked = dev_info->connected_mask & \
				BT_HID_CONNECTED;

	dev_info->network_checked = dev_info->connected_mask & \
				BT_NETWORK_CONNECTED;

	_bt_util_set_list_disabled(ugd->profile_vd->genlist,
				EINA_FALSE);
	FN_END;
}

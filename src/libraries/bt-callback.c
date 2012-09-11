/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vconf.h>
#include <vconf-keys.h>
#include <bluetooth.h>

#include "bt-debug.h"
#include "bt-main-ug.h"
#include "bt-main-view.h"
#include "bt-util.h"
#include "bt-widget.h"
#include "bt-string-define.h"
#include "bt-ipc-handler.h"
#include "bt-resource.h"

/**********************************************************************
*                                                 Static Functions
***********************************************************************/

static void __bt_cb_auto_discovery(void *data)
{
	FN_START;

	int ret;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	ret_if(elm_win_focus_get(ugd->win_main) == FALSE);

	/* If there is no paired devices, device searching starts. */
	if (ugd->search_req == TRUE ||
	     eina_list_count(ugd->paired_device) == 0) {
		_bt_main_remove_all_searched_devices(ugd);

		ret = bt_adapter_start_device_discovery();
		if (!ret) {
			ugd->op_status = BT_SEARCHING;
			elm_toolbar_item_icon_set(ugd->scan_item,
						BT_ICON_CONTROLBAR_STOP);
			elm_genlist_item_update(ugd->status_item);

			if (ugd->searched_title == NULL)
				_bt_main_add_searched_title(ugd);
		} else {
			BT_DBG("Discovery failed : Error Cause[%d]", ret);
		}
	}

	ugd->search_req = FALSE;

	FN_END;
}

static void __bt_cb_enable(int result, void *data)
{
	FN_START;

	int timeout = 0;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	if (ugd->op_status == BT_ACTIVATED) {
		BT_DBG("Already enabled state");
		return;
	}

	if (result != BT_ERROR_NONE) {
		BT_DBG("Failed to enable Bluetooth : Error Cause[%d]", result);
		ugd->op_status = BT_DEACTIVATED;
	} else {
		ugd->op_status = BT_ACTIVATED;
		ugd->aul_pairing_req = FALSE;
	}

	if (vconf_get_int(BT_VCONF_VISIBLE_TIME, &timeout) != 0) {
		BT_DBG("Fail to get the timeout value");
	}

	if (timeout == BT_ALWAYS_ON) {
		if (bt_adapter_set_visibility(
			BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE,
			timeout) ==  BT_ERROR_NONE) {

			ugd->visible = TRUE;
			ugd->remain_time = 0;
			ugd->visibility_timeout = timeout;
			ugd->selected_radio = _bt_util_get_timeout_index(timeout);
		} else {
			if (vconf_set_int(BT_VCONF_VISIBLE_TIME, 0) != 0)
				DBG("Set vconf failed\n");

			ugd->visible = FALSE;
			ugd->remain_time = 0;
			ugd->visibility_timeout = BT_ZERO;
			ugd->selected_radio = _bt_util_get_timeout_index(BT_ZERO);
		}
	}

	elm_object_item_disabled_set(ugd->visible_item, EINA_FALSE);
	elm_genlist_item_update(ugd->status_item);
	elm_genlist_item_update(ugd->visible_item);

	ret_if(ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY);

	elm_object_item_disabled_set(ugd->scan_item, EINA_FALSE);

	if (ugd->paired_title)
		elm_genlist_item_update(ugd->paired_title);

	if (ugd->searched_title)
		elm_genlist_item_update(ugd->searched_title);

	/* In the NFC case, we don't need to display the paired device */
	if (ugd->bt_launch_mode != BT_LAUNCH_USE_NFC)
		_bt_main_draw_paired_devices(ugd);

	__bt_cb_auto_discovery(ugd);

	FN_END;
}

static void __bt_cb_disable(int result, void *data)
{
	FN_START;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	if (ugd->op_status == BT_DEACTIVATED) {
		BT_DBG("Already disabled state");
		return;
	}

	if (ugd->popup) {
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	if (result != BT_ERROR_NONE) {
		BT_DBG("Failed to enable Bluetooth : Error Cause[%d]", result);
		ugd->op_status = BT_ACTIVATED;
	} else {
		if (ugd->timeout_id) {
			g_source_remove(ugd->timeout_id);
			ugd->timeout_id = 0;
		}

		ugd->op_status = BT_DEACTIVATED;
		ugd->visible = FALSE;
		ugd->visibility_timeout = 0;
		ugd->remain_time = 0;
		ugd->selected_radio = 0;

		elm_toolbar_item_icon_set(ugd->scan_item, BT_ICON_CONTROLBAR_SCAN);
		elm_genlist_item_update(ugd->visible_item);
		elm_genlist_item_subitems_clear(ugd->visible_item);
		elm_object_item_disabled_set(ugd->visible_item, EINA_TRUE);

		if (ugd->bt_launch_mode != BT_LAUNCH_VISIBILITY) {
			elm_object_item_disabled_set(ugd->scan_item,
							EINA_FALSE);

			_bt_main_remove_all_paired_devices(ugd);
			_bt_main_remove_all_searched_devices(ugd);
		}
	}

	elm_genlist_item_update(ugd->status_item);

	if (ugd->paired_title)
		elm_genlist_item_update(ugd->paired_title);

	if (ugd->searched_title)
		elm_genlist_item_update(ugd->searched_title);

	FN_END;
}

static void __bt_cb_search_completed(int result, void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	ugd->op_status = BT_ACTIVATED;
	elm_toolbar_item_icon_set(ugd->scan_item, BT_ICON_CONTROLBAR_SCAN);

	if (ugd->status_item)
		elm_genlist_item_update(ugd->status_item);

	if (ugd->paired_title)
		elm_genlist_item_update(ugd->paired_title);

	if (ugd->searched_title)
		elm_genlist_item_update(ugd->searched_title);

	if (ugd->searched_device == NULL ||
	     eina_list_count(ugd->searched_device) == 0) {
		/* Don't add the no device item, if no device item already exist */
		ret_if(ugd->no_device_item != NULL);
		ugd->no_device_item = _bt_main_add_no_device_found(ugd);
	}

	FN_END;
}


static bool __bt_cb_match_discovery_type(unsigned int major_class,
						unsigned int service_class,
						unsigned int mask)
{
	FN_START;

	bt_device_major_mask_t major_mask = BT_DEVICE_MAJOR_MASK_MISC;

	if (mask == 0x000000)
		return TRUE;

	BT_DBG("mask: %x", mask);

	BT_DBG("service_class: %x", service_class);

	/* Check the service_class */
	if (service_class & mask)
		return TRUE;

	/* Check the major class */
	switch (major_class) {
		case BT_MAJOR_DEV_CLS_COMPUTER:
			major_mask = BT_DEVICE_MAJOR_MASK_COMPUTER;
			break;
		case BT_MAJOR_DEV_CLS_PHONE:
			major_mask = BT_DEVICE_MAJOR_MASK_PHONE;
			break;
		case BT_MAJOR_DEV_CLS_LAN_ACCESS_POINT:
			major_mask = BT_DEVICE_MAJOR_MASK_LAN_ACCESS_POINT;
			break;
		case BT_MAJOR_DEV_CLS_AUDIO:
			major_mask = BT_DEVICE_MAJOR_MASK_AUDIO;
			break;
		case BT_MAJOR_DEV_CLS_PERIPHERAL:
			major_mask = BT_DEVICE_MAJOR_MASK_PERIPHERAL;
			break;
		case BT_MAJOR_DEV_CLS_IMAGING:
			major_mask = BT_DEVICE_MAJOR_MASK_IMAGING;
			break;
		case BT_MAJOR_DEV_CLS_WEARABLE:
			major_mask = BT_DEVICE_MAJOR_MASK_WEARABLE;
			break;
		case BT_MAJOR_DEV_CLS_TOY:
			major_mask = BT_DEVICE_MAJOR_MASK_TOY;
			break;
		case BT_MAJOR_DEV_CLS_HEALTH:
			major_mask = BT_DEVICE_MAJOR_MASK_HEALTH;
			break;
		default:
			major_mask = BT_DEVICE_MAJOR_MASK_MISC;
			break;
	}

	BT_DBG("major_mask: %x", major_mask);

	if (mask & major_mask)
		return TRUE;

	FN_END;

	return FALSE;
}


static void __bt_cb_new_device_found(bt_adapter_device_discovery_info_s *info,
				     void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;

	ret_if(info == NULL);
	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	/* Check the service_class */
	if (__bt_cb_match_discovery_type(
				info->bt_class.major_device_class,
				info->bt_class.major_service_class_mask,
				ugd->search_type) == FALSE) {
		BT_DBG("No matched device type");
		return;
	}

	if (info->is_bonded == TRUE) {
		BT_DBG("Paired device found");
		return;
	}

	if (_bt_main_check_and_update_device(ugd->searched_device,
					info->remote_address,
					info->remote_name) >= 0) {
		/* Update all realized items */
		elm_genlist_realized_items_update(ugd->main_genlist);
	} else {
		dev = _bt_main_create_searched_device_item((void *)info);
		if (NULL == dev) {
			BT_DBG("Create new device item failed \n");
			return;
		}

		if (_bt_main_is_matched_profile(ugd->search_type,
						dev->major_class,
						dev->service_class,
						ugd->service) == TRUE) {
			if (_bt_main_add_searched_device(ugd, dev) == NULL) {
				BT_DBG("Fail to add the searched device \n");
				return;
			}

			ugd->searched_device =
			    eina_list_append(ugd->searched_device, dev);

			if (ugd->searched_title)
				elm_genlist_item_update(ugd->searched_title);
		}
	}

	FN_END;
}

/**********************************************************************
*                                                Common Functions
***********************************************************************/

void _bt_cb_state_changed(int result,
			bt_adapter_state_e adapter_state,
			void *user_data)
{
	FN_START;

	if (adapter_state == BT_ADAPTER_ENABLED)
		__bt_cb_enable(result, user_data);
	else
		__bt_cb_disable(result, user_data);

	FN_END;
}

void _bt_cb_discovery_state_changed(int result,
			bt_adapter_device_discovery_state_e discovery_state,
			bt_adapter_device_discovery_info_s *discovery_info,
			void *user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ret_if(user_data == NULL);

	ugd = (bt_ug_data *)user_data;

	/* This UG is not in the searching state. */
	ret_if(ugd->op_status != BT_SEARCHING);

	if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FOUND)
		__bt_cb_new_device_found(discovery_info, user_data);
	else if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FINISHED)
		__bt_cb_search_completed(result, user_data);

	FN_END;
}

void _bt_cb_bonding_created(int result, bt_device_info_s *dev_info,
				void *user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;

	ret_if(dev_info == NULL);
	ret_if(user_data == NULL);

	ugd = (bt_ug_data *)user_data;

	ugd->op_status = BT_ACTIVATED;

	/* Enable UG to send another pairing request */
	ugd->aul_pairing_req = FALSE;

	/* In the NFC cas, we don't need to display the paired device */
	if (ugd->bt_launch_mode == BT_LAUNCH_USE_NFC)
		return;

	dev = _bt_main_get_dev_info_by_address(ugd->searched_device,
					dev_info->remote_address);

	if (result != BT_ERROR_NONE) {
		BT_DBG("Failed to pair with device : Error Cause[%d]", result);
		retm_if(dev == NULL, "dev is NULL\n");

		dev->status = BT_IDLE;
		elm_genlist_item_update(ugd->searched_item);

		if (result != BT_ERROR_CANCELLED) {
			_bt_main_launch_syspopup(ugd, BT_SYSPOPUP_REQUEST_NAME,
					BT_STR_BLUETOOTH_ERROR_TRY_AGAIN_Q,
					BT_SYSPOPUP_TWO_BUTTON_TYPE);
		}
	} else {
		bt_dev_t *new_dev = NULL;
		Elm_Object_Item *item = NULL;

		if (dev != NULL) {
			/* Remove the item in searched dialogue */
			_bt_main_remove_searched_device(ugd, dev);
		}

		/* Add the item in paired dialogue group */
		new_dev = _bt_main_create_paired_device_item(dev_info);

		item = _bt_main_add_paired_device(ugd, new_dev);

		ugd->paired_device =
		    eina_list_append(ugd->paired_device, new_dev);

		_bt_main_connect_device(ugd, new_dev);

		ugd->searched_item = NULL;
		ugd->paired_item = item;
	}

	elm_object_item_disabled_set(ugd->scan_item, EINA_FALSE);

	FN_END;
}

void _bt_cb_bonding_destroyed(int result, char *remote_address,
					void *user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *new_item = NULL;
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retm_if(remote_address == NULL, "Invalid argument: param is NULL\n");
	retm_if(user_data == NULL, "Invalid argument: param is NULL\n");

	ugd = (bt_ug_data *)user_data;

	if (result != BT_ERROR_NONE) {
		BT_DBG("Failed to unbond: [%d]", result);
		return;
	}

	EINA_LIST_FOREACH(ugd->paired_device, l, item) {
		if (memcmp((const char *)item->addr_str,
		    (const char *)remote_address,
		    BT_ADDRESS_LENGTH_MAX) == 0) {
			new_item = calloc(1, sizeof(bt_dev_t));
			if (new_item == NULL)
				break;

			memcpy(new_item, item, sizeof(bt_dev_t));

			_bt_main_remove_paired_device(ugd, item);

			if (ugd->no_device_item) {
				elm_object_item_del(ugd->no_device_item);
				ugd->no_device_item = NULL;
			}

			if (_bt_main_add_searched_device(ugd, new_item) != NULL) {
				ugd->searched_device = eina_list_append(
						ugd->searched_device, new_item);

				if (ugd->searched_title)
					elm_genlist_item_update(
							ugd->searched_title);
			}

			BT_DBG("unbonding_count: %d", ugd->unbonding_count);

			if (ugd->unbonding_count <= 1) {
				_bt_profile_delete_view((void *)new_item);
			} else {
				ugd->unbonding_count--;
			}

			break;
		}
	}

	FN_END;
}

void _bt_cb_service_searched(int result, bt_device_sdp_info_s *sdp_info,
				void* user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_service_class_t service_mask = 0;
	bt_dev_t *item = NULL;
	Evas_Object *btn = NULL;

	ret_if(user_data == NULL);

	ugd = (bt_ug_data *)user_data;

	ugd->op_status = BT_ACTIVATED;

	BT_DBG("Result: %d", result);

	item = _bt_main_get_dev_info_by_address(ugd->paired_device,
					sdp_info->remote_address);

	if (item == NULL) {
		ugd->waiting_service_response = FALSE;
		ugd->auto_service_search = FALSE;
		return;
	}

	item->status = BT_IDLE;
	elm_genlist_item_update((Elm_Object_Item *)item->genlist_item);

	if (result == BT_ERROR_NONE) {
		_bt_util_get_service_mask_from_uuid_list(
						sdp_info->service_uuid,
						sdp_info->service_count,
						&service_mask);

		item->service_list = service_mask;

		if (ugd->waiting_service_response == TRUE) {
			_bt_main_connect_device(ugd, item);
		}
	} else {
		BT_DBG("Failed to get the service list [%d]", result);

		if (ugd->waiting_service_response == TRUE) {
			if (ugd->popup) {
				evas_object_del(ugd->popup);
				ugd->popup = NULL;
			}

			ugd->popup =
			    _bt_create_popup(ugd->win_main, BT_STR_ERROR,
					BT_STR_UNABLE_TO_GET_THE_SERVICE_LIST,
					_bt_main_popup_del_cb, ugd, 2);
			ugd->back_cb = _bt_util_launch_no_event;

			btn = elm_button_add(ugd->popup);
			elm_object_text_set(btn, BT_STR_CANCEL);
			elm_object_part_content_set(ugd->popup, "button1", btn);
			evas_object_smart_callback_add(btn, "clicked",
				(Evas_Smart_Cb)_bt_main_popup_del_cb, ugd);
		}
	}

	ugd->waiting_service_response = FALSE;
	ugd->auto_service_search = FALSE;

	FN_END;
}

void _bt_cb_hid_state_changed(int result, bool connected,
			const char *remote_address,
			void *user_data)
{
	FN_START;

	bt_address_t address = { { 0 } };

	BT_DBG("Bluetooth HID Event [%d] Received");

	if (remote_address == NULL) {
		BT_DBG("No address information");
		return;
	}

	_bt_util_addr_string_to_addr_type(address.bd_addr, remote_address);

	_bt_ipc_update_connected_status(user_data, BT_HID_CONNECTED,
					connected, result, &address);
	FN_END;
}

void _bt_cb_audio_state_changed(int result, bool connected,
				const char *remote_address,
				bt_audio_profile_type_e type,
				void *user_data)
{
	FN_START;

	bt_address_t address = { { 0 } };
	int connected_type;

	BT_DBG("Bluetooth Audio Event [%d] Received");

	if (remote_address == NULL) {
		BT_DBG("No address information");
		return;
	}

	if (type == BT_AUDIO_PROFILE_TYPE_A2DP)
		connected_type = BT_STEREO_HEADSET_CONNECTED;
	else
		connected_type = BT_HEADSET_CONNECTED;

	_bt_util_addr_string_to_addr_type(address.bd_addr, remote_address);

	_bt_ipc_update_connected_status(user_data, connected_type,
					connected, result, &address);
	FN_END;
}


/*
* ug-setting-bluetooth-efl
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
#include <vconf-keys.h>
#include <bluetooth.h>
#include <efl_extension.h>
#include <bundle_internal.h>

#include "bt-debug.h"
#include "bt-main-ug.h"
#include "bt-main-view.h"
#include "bt-util.h"
#include "bt-widget.h"
#include "bt-string-define.h"
#include "bt-ipc-handler.h"
#include "bt-resource.h"
#include "bt-net-connection.h"

/**********************************************************************
*                                                 Static Functions
***********************************************************************/

static Eina_Bool __bt_cb_auto_discovery(void *data)
{
	FN_START;

	int ret;
	bt_ug_data *ugd = NULL;

	retv_if(data == NULL, ECORE_CALLBACK_CANCEL);

	ugd = (bt_ug_data *)data;

	retv_if((elm_win_focus_get(ugd->win_main) == FALSE), ECORE_CALLBACK_CANCEL);

	_bt_main_remove_all_searched_devices(ugd);

	ret = bt_adapter_start_device_discovery();
	if (!ret) {
		ugd->op_status = BT_SEARCHING;
		elm_object_text_set(ugd->scan_btn, BT_STR_STOP);

		if (ugd->searched_title == NULL)
				_bt_main_add_searched_title(ugd);
	} else {
		BT_ERR("Discovery failed : Error Cause[%d]", ret);
	}

	FN_END;
	return ECORE_CALLBACK_CANCEL;
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
		BT_ERR("Failed to enable Bluetooth [%d]", result);
		ugd->op_status = BT_DEACTIVATED;
		_bt_update_genlist_item(ugd->onoff_item);
		return;
	} else {
		ugd->op_status = BT_ACTIVATED;
		elm_genlist_item_item_class_update(ugd->onoff_item,
						ugd->on_itc);
		_bt_update_genlist_item(ugd->onoff_item);
		ugd->aul_pairing_req = FALSE;
	}

	if (vconf_get_int(BT_FILE_VISIBLE_TIME, &timeout) != 0) {
		BT_ERR("Fail to get the timeout value");
	}

	if (timeout == BT_ALWAYS_ON) {
		ugd->visible = TRUE;
		ugd->remain_time = 0;
		ugd->visibility_timeout = timeout;
		ugd->selected_radio = _bt_util_get_timeout_index(timeout);
	}

	_bt_util_set_phone_name();

	if(!ugd->device_name_item)
		_bt_main_add_device_name_item(ugd, ugd->main_genlist);

	if(!ugd->visible_item)
		_bt_main_add_visible_item(ugd, ugd->main_genlist);

	if (ugd->empty_status_item) {
		elm_object_item_del(ugd->empty_status_item);
		ugd->empty_status_item = NULL;
	}

	_bt_update_genlist_item(ugd->visible_item);

	if (ugd->bt_launch_mode == BT_LAUNCH_ONOFF) {
		g_idle_add((GSourceFunc) _bt_idle_destroy_ug, ugd);
		return;
	} else if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY) {
		return;
	}

	ugd->scan_btn = _bt_main_create_scan_button(ugd);

	_bt_update_genlist_item(ugd->paired_title);
	_bt_update_genlist_item(ugd->searched_title);

	/* In the NFC case, we don't need to display the paired device */
	if (ugd->bt_launch_mode != BT_LAUNCH_USE_NFC)
		_bt_main_draw_paired_devices(ugd);

	if (!ecore_idler_add(__bt_cb_auto_discovery, ugd))
		BT_ERR("idler can not be added\n\n");

	FN_END;
}

static void __bt_cb_disable(int result, void *data)
{
	FN_START;
	bt_ug_data *ugd = NULL;
	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;
	int ret = 0;

	if (ugd->op_status == BT_DEACTIVATED) {
		BT_DBG("Already disabled state");
		return;
	}

	if (ugd->help_more_popup) {
		evas_object_del(ugd->help_more_popup);
		ugd->help_more_popup = NULL;
	}

	if (ugd->popup) {
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	if (ugd->visibility_popup) {
		evas_object_del(ugd->visibility_popup);
		ugd->visibility_popup = NULL;
	}

	if (ugd->rename_popup) {
		evas_object_del(ugd->rename_popup);
		ugd->rename_popup = NULL;
		ugd->rename_entry = NULL;
	}

	if (result != BT_ERROR_NONE) {
		BT_ERR("Failed to disable Bluetooth [%d]", result);
		ugd->op_status = BT_ACTIVATED;
	} else {
		/* Delete profile view */
		if (ugd->profile_vd != NULL) {
			elm_naviframe_item_pop(ugd->navi_bar);
			ugd->profile_vd = NULL;
		}

		if (ugd->timeout_id) {
			g_source_remove(ugd->timeout_id);
			ugd->timeout_id = 0;
		}

		ugd->op_status = BT_DEACTIVATED;
		ugd->visible = FALSE;
		ugd->remain_time = 0;

		if (ugd->visibility_timeout != BT_ALWAYS_ON) {
			ugd->visibility_timeout = 0;
			ugd->selected_radio = 0;
		}

		evas_object_del(ugd->scan_btn);
		ugd->scan_btn = NULL;
		elm_genlist_item_item_class_update(ugd->onoff_item,
						ugd->off_itc);

		if (ugd->visible_item && EINA_TRUE == elm_genlist_item_expanded_get(
							ugd->visible_item)) {
			elm_genlist_item_expanded_set(ugd->visible_item,
							EINA_FALSE);
		}

		if(ugd->visible_item) {
			elm_object_item_del(ugd->visible_item);
			ugd->visible_item = NULL;
		}

		if(ugd->device_name_item) {
			elm_object_item_del(ugd->device_name_item);
			ugd->device_name_item = NULL;
		}

		if (ugd->bt_launch_mode != BT_LAUNCH_VISIBILITY &&
		     ugd->bt_launch_mode != BT_LAUNCH_ONOFF) {
			elm_object_disabled_set(ugd->scan_btn, EINA_FALSE);
			_bt_main_remove_all_paired_devices(ugd);
			_bt_main_remove_all_searched_devices(ugd);
		}

		if (ugd->bt_launch_mode == BT_LAUNCH_ONOFF) {
			ret = _bt_idle_destroy_ug((void *)ugd);
			if (ret != BT_UG_ERROR_NONE)
				BT_DBG("fail to destory ug");
			return;
		}
	}

	_bt_update_genlist_item(ugd->onoff_item);
	_bt_update_genlist_item(ugd->visible_item);
	_bt_update_genlist_item(ugd->paired_title);
	_bt_update_genlist_item(ugd->searched_title);

	FN_END;
}

static void __bt_cb_search_completed(int result, void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	if (ugd->op_status == BT_SEARCHING)
		ugd->op_status = BT_ACTIVATED;

	ugd->is_discovery_started = FALSE;

	elm_object_text_set(ugd->scan_btn, BT_STR_SCAN);

	_bt_update_genlist_item(ugd->paired_title);
	_bt_update_genlist_item(ugd->searched_title);

	if (ugd->searched_device == NULL ||
		eina_list_count(ugd->searched_device) == 0) {
		/* Don't add the no device item, if no device item already exist */
		ret_if(ugd->no_device_item != NULL);

		if (ugd->op_status != BT_DEACTIVATED) {
			ugd->no_device_item = _bt_main_add_no_device_found(ugd);
		}
		_bt_update_genlist_item(ugd->no_device_item);
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
	bt_device_info_s *device_info = NULL;
	int i;
	ret_if(info == NULL);
	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;
	ret_if(ugd->op_status != BT_SEARCHING);

	/* Check the service_class */
	if (__bt_cb_match_discovery_type(
				info->bt_class.major_device_class,
				info->bt_class.major_service_class_mask,
				ugd->search_type) == FALSE) {
		BT_DBG("No matched device type");
		return;
	}

#ifdef TIZEN_BT_A2DP_SINK_ENABLE
/*
	The Class of Device field shall be set to the following:
	1. Mandatory to set the ¡®Rendering¡¯ bit for the SNK and the ¡®Capturing¡¯ bit for the SRC in the Service Class field.
*/
#if 0	/* The below code also filters other devices that is not A2DP source role, so it is disabled in the tizen platform */
	retm_if (!(info->bt_class.major_service_class_mask & BT_COD_SC_CAPTURING),
		"Display A2DP source only in A2DP sink role. Skip this device");
#endif
#endif

	if (info->is_bonded == TRUE) {
		BT_DBG("Paired device found");
		if (_bt_main_check_and_update_device(ugd->paired_device,
						info->remote_address,
						info->remote_name) >= 0) {
			/* Update all realized items */
			elm_genlist_realized_items_update(ugd->main_genlist);
			return;
		}

		device_info = (bt_device_info_s *)g_malloc0(sizeof(bt_device_info_s));
		/* Fix : NULL_RETURNS */
		if (device_info == NULL)
			return;

		device_info->bt_class = info->bt_class;
		device_info->remote_address = g_strdup(info->remote_address);
		device_info->remote_name = g_strdup(info->remote_name);
		device_info->service_count = info->service_count;

		if (info->service_uuid != NULL && info->service_count > 0) {
			device_info->service_uuid = g_new0(char *, info->service_count + 1);

			for (i = 0; i < info->service_count; i++) {
				device_info->service_uuid[i] = g_strdup(info->service_uuid[i]);
			}
		}
		dev = _bt_main_create_paired_device_item(device_info);

		for (i = 0; i < info->service_count; i++) {
			g_free(device_info->service_uuid[i]);
		}

		g_free(device_info->remote_address);
		g_free(device_info->remote_name);
		g_free(device_info);

		retm_if (!dev, "create paired device item fail!");

		dev->ugd = (void *)ugd;

		if (_bt_main_is_matched_profile(ugd->search_type,
						info->bt_class.major_device_class,
						info->bt_class.major_service_class_mask,
						ugd->service,
						info->bt_class.minor_device_class) == TRUE) {
			if (_bt_main_add_paired_device(ugd, dev) != NULL) {
				ugd->paired_device =
				    eina_list_append(ugd->paired_device, dev);
			}
		} else {
			BT_ERR("Device class and search type do not match");
			free(dev);
		}
		if (_bt_main_check_and_update_device(ugd->paired_device,
						info->remote_address,
						info->remote_name) >= 0) {
			_bt_update_device_list(ugd);
		}
		return;
	}

	if(ugd->searched_device == NULL)
		_bt_update_genlist_item(ugd->searched_title);

	if (_bt_main_check_and_update_device(ugd->searched_device,
					info->remote_address,
					info->remote_name) >= 0) {
		_bt_update_device_list(ugd);
	} else {
		dev = _bt_main_create_searched_device_item((void *)info);
		if (NULL == dev) {
			BT_ERR("Create new device item failed");
			return;
		}

		if (_bt_main_is_matched_profile(ugd->search_type,
						dev->major_class,
						dev->service_class,
						ugd->service,
						dev->minor_class) == TRUE) {
			if (_bt_main_add_searched_device(ugd, dev) == NULL) {
				BT_ERR("Fail to add the searched device");
				return;
			}

			ugd->searched_device =
			    eina_list_append(ugd->searched_device, dev);

		} else {
			BT_DBG("Searched device does not match the profile");
			free(dev);
		}
	}

	FN_END;
}

static gboolean __bt_cb_visible_timeout_cb(gpointer user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	time_t current_time;
	int time_diff;

	ugd = (bt_ug_data *)user_data;
	/* Get the current time */
	time(&current_time);

	/* Calculate time elapsed from remain_time */
	time_diff = difftime(current_time, ugd->start_time);
	BT_DBG("Time difference in seconds %d", time_diff);

	/* Update UI */
	if (ugd->remain_time <= time_diff) {
		g_source_remove(ugd->timeout_id);
		ugd->timeout_id = 0;
		ugd->visibility_timeout = 0;
		ugd->remain_time = 0;
		ugd->selected_radio = 0;

		elm_genlist_realized_items_update(ugd->main_genlist);

		return FALSE;
	}

	elm_genlist_item_fields_update(ugd->visible_item, "elm.text.multiline",
				       ELM_GENLIST_ITEM_FIELD_TEXT);

	FN_END;
	return TRUE;
}

static void __bt_retry_pairing_cb(void *data,
				     Evas_Object *obj, void *event_info)
{
	FN_START;
	ret_if (obj == NULL || data == NULL);

	bt_ug_data *ugd = (bt_ug_data *)data;
	const char *event = elm_object_text_get(obj);

	if (ugd->popup) {
		BT_DBG("delete popup");
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	if ((!g_strcmp0(event, BT_STR_RETRY) || (!g_strcmp0(event, BT_STR_OK))) &&
		_bt_main_request_pairing_with_effect(ugd,
		ugd->searched_item) != BT_UG_ERROR_NONE) {
		ugd->searched_item = NULL;
	}
	if (!g_strcmp0(event, BT_STR_CANCEL))
		ugd->op_status = BT_ACTIVATED;
	FN_END;
}


/**********************************************************************
*                                                Common Functions
***********************************************************************/

void _bt_cb_state_changed(int result,
			bt_adapter_state_e adapter_state,
			void *user_data)
{
	BT_INFO("bluetooth %s", adapter_state == BT_ADAPTER_ENABLED ?
				"enabled" : "disabled");

	if (adapter_state == BT_ADAPTER_ENABLED)
		__bt_cb_enable(result, user_data);
	else
		__bt_cb_disable(result, user_data);
}

void _bt_cb_discovery_state_changed(int result,
			bt_adapter_device_discovery_state_e discovery_state,
			bt_adapter_device_discovery_info_s *discovery_info,
			void *user_data)
{
	bt_ug_data *ugd = NULL;

	ret_if(user_data == NULL);

	ugd = (bt_ug_data *)user_data;

	if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_STARTED) {
		BT_INFO("BR/EDR discovery started");
		ugd->is_discovery_started = TRUE;
		/*Now enable the Scan button, so that user may call cancel discovery */
			elm_object_disabled_set(ugd->scan_btn, EINA_FALSE);
	} else if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FOUND)
		__bt_cb_new_device_found(discovery_info, user_data);
	else if (discovery_state == BT_ADAPTER_DEVICE_DISCOVERY_FINISHED) {
		BT_INFO("BR/EDR discovery finished");
		ret_if(ugd->is_discovery_started == FALSE);

		__bt_cb_search_completed(result, user_data);
		if (ugd->op_status == BT_PAIRING)
			elm_object_disabled_set(ugd->scan_btn, EINA_TRUE);
		else
			elm_object_disabled_set(ugd->scan_btn, EINA_FALSE);
	} else
		BT_ERR("Unknown device discovery state");

}

void _bt_cb_visibility_mode_changed
	(int result, bt_adapter_visibility_mode_e visibility_mode, void *user_data)
{
	FN_START;

	ret_if(user_data == NULL);
	bt_ug_data *ugd = (bt_ug_data *)user_data;
	bt_adapter_state_e bt_state = BT_ADAPTER_DISABLED;
	bt_adapter_visibility_mode_e mode =
	    BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;
	int remain_time = 0;
	int ret = 0;

	if(ugd->visibility_changed_by_ug) {
		ugd->visibility_changed_by_ug = FALSE;
		FN_END;
		return;
	}
BT_DBG("");
	if (bt_adapter_get_state(&bt_state) != BT_ERROR_NONE) {
		BT_ERR("bt_adapter_get_state() failed.");
		return;
	}

	if (bt_state != BT_ADAPTER_DISABLED) {
		if (bt_adapter_get_visibility(&mode, &remain_time) !=
			BT_ERROR_NONE) {
			BT_ERR("bt_adapter_get_visibility() failed.");
			return;
		}
	}

	if (visibility_mode == BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE) {
		ugd->visible = FALSE;
		ugd->visibility_timeout = 0;
		ugd->selected_radio = 0;
	} else if (visibility_mode == BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE) {
		ugd->visible = TRUE;
		ugd->visibility_timeout = -1;
		ugd->selected_radio = 4;
	} else {
		/* BT_ADAPTER_VISIBILITY_MODE_LIMITED_DISCOVERABLE */
		/* Need to add the code for getting timeout */
		ret = vconf_get_int(BT_FILE_VISIBLE_TIME, &ugd->visibility_timeout);
		if (ret != 0) {
			BT_ERR("Failt to get the timeout value");
		}
BT_DBG("");

		ugd->remain_time = remain_time;

		if (ugd->remain_time > 0) {

			if (ugd->timeout_id) {
				g_source_remove(ugd->timeout_id);
				ugd->timeout_id = 0;
			}
			/* Set current time snapshot */
			time(&(ugd->start_time));
			ugd->timeout_id = g_timeout_add(BT_VISIBILITY_TIMEOUT,
							(GSourceFunc)
							__bt_cb_visible_timeout_cb,
							ugd);
		} else {
			ugd->visibility_timeout = 0;
		}
	}


	if(ugd->visible_item) {
		elm_genlist_item_fields_update(ugd->visible_item,
						"elm.text",
						ELM_GENLIST_ITEM_FIELD_TEXT);
		elm_genlist_item_fields_update(ugd->visible_item,
						"elm.text.multiline",
						ELM_GENLIST_ITEM_FIELD_TEXT);

		elm_radio_value_set(ugd->radio_main, ugd->selected_radio);
	}
	FN_END;
}

void _bt_cb_bonding_created(int result, bt_device_info_s *dev_info,
				void *user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;
	Evas_Object *btn1 = NULL;
	Evas_Object *btn2 = NULL;

	ret_if(dev_info == NULL);
	ret_if(user_data == NULL);

	ugd = (bt_ug_data *)user_data;
	ret_if(ugd->op_status == BT_DEACTIVATING);

	if (ugd->op_status == BT_PAIRING)
		ugd->op_status = BT_ACTIVATED;

	/* Enable UG to send another pairing request */
	ugd->aul_pairing_req = FALSE;

	/* In the NFC cas, we don't need to display the paired device */
	if (ugd->bt_launch_mode == BT_LAUNCH_USE_NFC)
		return;

	dev = _bt_main_get_dev_info_by_address(ugd->searched_device,
					dev_info->remote_address);

	if (dev == NULL && ugd->searched_item != NULL)
		dev = _bt_main_get_dev_info(ugd->searched_device, ugd->searched_item);

	elm_object_disabled_set(ugd->scan_btn, EINA_FALSE);

	if (result != BT_ERROR_NONE) {
		BT_ERR("Failed to pair with device [%d]", result);
		retm_if(dev == NULL, "dev is NULL");

		dev->status = BT_IDLE;

		elm_genlist_item_item_class_update(dev->genlist_item,
						ugd->searched_device_itc);
		_bt_update_genlist_item((Elm_Object_Item *)dev->genlist_item);


		BT_ERR("Authentication Failed");
		if (_bt_util_is_battery_low() == TRUE) {
			_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
		} else if (result != BT_ERROR_NOT_IN_PROGRESS
				&& result != BT_ERROR_CANCELLED){
			ugd->op_status = BT_PAIRING;

			_bt_main_popup_del_cb(ugd, NULL, NULL);

			ugd->popup_data.type = BT_POPUP_PAIRING_ERROR;
			ugd->popup_data.data = g_strdup(dev->name);
			ugd->popup = _bt_create_popup(ugd, NULL, ugd, 0);
			retm_if(ugd->popup == NULL, "fail to create popup!");

			btn1 = elm_button_add(ugd->popup);
			elm_object_style_set(btn1, "popup");
			elm_object_domain_translatable_text_set(
				btn1,
				PKGNAME, "IDS_BR_SK_CANCEL");
			elm_object_part_content_set(ugd->popup, "button1", btn1);
			evas_object_smart_callback_add(btn1,
					"clicked", __bt_retry_pairing_cb, ugd);

			btn2 = elm_button_add(ugd->popup);
			elm_object_style_set(btn2, "popup");
			elm_object_domain_translatable_text_set(
				btn2,
				PKGNAME, "IDS_ST_BUTTON_RETRY");
			elm_object_part_content_set(ugd->popup, "button2", btn2);
			evas_object_smart_callback_add(btn2,
					"clicked", __bt_retry_pairing_cb, ugd);

			eext_object_event_callback_add(ugd->popup,
					EEXT_CALLBACK_BACK,
					_bt_back_btn_popup_del_cb, ugd);
			evas_object_show(ugd->popup);
		}

	} else {
		bt_dev_t *new_dev = NULL;
		Elm_Object_Item *item = NULL;
		void *profile = NULL;

		if (_bt_main_check_and_update_device(ugd->paired_device,
					dev_info->remote_address,
					dev_info->remote_name) >= 0) {
			_bt_update_device_list(ugd);
		} else {
			if (dev != NULL) {
				/* Remove the item in searched dialogue */
				_bt_main_remove_searched_device(ugd, dev);
			}

			/* Add the item in paired dialogue group */
			new_dev = _bt_main_create_paired_device_item(dev_info);
			if (new_dev == NULL) {
				BT_ERR("new_dev is NULL");
				return;
			}
#ifndef TIZEN_BT_A2DP_SINK_ENABLE
			profile = _bt_get_registered_net_profile(ugd->connection,
								new_dev->bd_addr);

			if (profile)
				_bt_set_profile_state_changed_cb(profile, new_dev);
#endif
			if (_bt_main_is_matched_profile(ugd->search_type,
					new_dev->major_class,
					new_dev->service_class,
					ugd->service,
					new_dev->minor_class) == TRUE) {
				item = _bt_main_add_paired_device_on_bond(ugd, new_dev);
			} else {
				BT_ERR("Device did not match search type");
				_bt_util_free_device_item(new_dev);
				return;
			}

			if (item) {
				ugd->paired_device =
				    eina_list_append(ugd->paired_device, new_dev);

			}

			/* Don't try to auto-connect in the network case */
			if (profile == NULL && ugd->searched_item != NULL) {
				if (_bt_main_is_connectable_device(new_dev))
					_bt_main_connect_device(ugd, new_dev);
			} else {
				BT_DBG("Net profile exists");
				int connected = _bt_util_is_profile_connected(
						BT_NETWORK_SERVER_CONNECTED,
						new_dev->bd_addr);
				bt_address_t address = { { 0 } };
				new_dev->connected_mask |=
					connected ? BT_NETWORK_CONNECTED : 0x00;
				memcpy(address.bd_addr, new_dev->bd_addr,
					BT_ADDRESS_LENGTH_MAX);

				_bt_ipc_update_connected_status(user_data,
					BT_NETWORK_CONNECTED,
					connected, BT_UG_ERROR_NONE,
					&address);
			}

			ugd->searched_item = NULL;
			ugd->paired_item = item;

		}
	}

	FN_END;
}

void _bt_cb_bonding_destroyed(int result, char *remote_address,
					void *user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retm_if(remote_address == NULL, "Invalid argument: param is NULL\n");
	retm_if(user_data == NULL, "Invalid argument: param is NULL\n");

	ugd = (bt_ug_data *)user_data;

	EINA_LIST_FOREACH(ugd->paired_device, l, item) {
		if (item == NULL)
			break;

		if (g_strcmp0(item->addr_str, remote_address) == 0) {
			item->status = BT_IDLE;

			if (result != BT_ERROR_NONE) {
				BT_ERR("Failed to unbond: [%d]", result);
				return;
			}

			if (ugd->profile_vd)
				_bt_profile_delete_view(item->ugd);

			_bt_main_remove_paired_device(ugd, item);
			item->connected_mask = 0x00;
			item->is_connected = 0x00;
			item->genlist_item = NULL;

			if (item->net_profile != NULL) {
				BT_INFO("unset profile state change callback");
				_bt_unset_profile_state_changed_cb(item->net_profile);
				item->net_profile = NULL;
			}

			if (ugd->no_device_item) {
				elm_object_item_del(ugd->no_device_item);
				ugd->no_device_item = NULL;
			}

			if (_bt_main_add_searched_device(ugd, item) != NULL) {
				ugd->searched_device = eina_list_append(
						ugd->searched_device, item);
			}

			break;
		}
	}

	if (ugd->paired_device == NULL && ugd->op_status != BT_PAIRING) {
		if (!ecore_idler_add(__bt_cb_auto_discovery, ugd))
			BT_ERR("idler can not be added\n\n");

	}

	FN_END;
}

void _bt_cb_service_searched(int result, bt_device_sdp_info_s *sdp_info,
				void* user_data)
{
	FN_START;

	int i;
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

	if (item == NULL)
		item = _bt_main_get_dev_info(ugd->paired_device, ugd->paired_item);

	if (item == NULL) {
		ugd->waiting_service_response = FALSE;
		return;
	}

	item->status = BT_IDLE;
	_bt_update_genlist_item((Elm_Object_Item *)item->genlist_item);

	if (result == BT_ERROR_NONE) {
		bt_device_get_service_mask_from_uuid_list(
						sdp_info->service_uuid,
						sdp_info->service_count,
						&service_mask);

		if (sdp_info->service_uuid != NULL && sdp_info->service_count > 0) {

			_bt_util_free_device_uuids(item);
			item->uuids = g_new0(char *, sdp_info->service_count + 1);

			for (i = 0; i < sdp_info->service_count; i++) {
				item->uuids[i] = g_strdup(sdp_info->service_uuid[i]);
			}

			item->uuid_count = sdp_info->service_count;
		}

		item->service_list = service_mask;

		if (ugd->waiting_service_response == TRUE) {
			_bt_main_connect_device(ugd, item);
		}
	} else {
		BT_ERR("Failed to get the service list [%d]", result);

		if (ugd->waiting_service_response == TRUE) {
			_bt_main_popup_del_cb(ugd, NULL, NULL);

			ugd->popup_data.type = BT_POPUP_GET_SERVICE_LIST_ERROR;
			ugd->popup = _bt_create_popup(ugd,
					_bt_main_popup_del_cb, ugd, 2);
			retm_if(ugd->popup == NULL, "fail to create popup!");
			ugd->back_cb = _bt_util_launch_no_event;

			btn = elm_button_add(ugd->popup);
			elm_object_style_set(btn, "popup");
			elm_object_domain_translatable_text_set(
				btn ,
				PKGNAME, "IDS_BR_SK_CANCEL");
			elm_object_part_content_set(ugd->popup, "button1", btn);
			evas_object_smart_callback_add(btn, "clicked",
				(Evas_Smart_Cb)_bt_main_popup_del_cb, ugd);
			eext_object_event_callback_add(ugd->popup, EEXT_CALLBACK_BACK,
					_bt_main_popup_del_cb, ugd);

			evas_object_show(ugd->popup);

		}
	}

	ugd->waiting_service_response = FALSE;

	FN_END;
}

void _bt_cb_hid_state_changed(int result, bool connected,
			const char *remote_address,
			void *user_data)
{
	FN_START;
	ret_if(!user_data);
	ret_if(!remote_address);
	bt_ug_data *ugd = (bt_ug_data *)user_data;
	ret_if(ugd->op_status == BT_DEACTIVATING || ugd->op_status == BT_DEACTIVATED);

	bt_address_t address = { { 0 } };

	BT_INFO("Bluetooth HID Event [%d] [%d]", result, connected);

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
	ret_if(!user_data);
	ret_if(!remote_address);
	bt_ug_data *ugd = (bt_ug_data *)user_data;
	ret_if(ugd->op_status == BT_DEACTIVATING || ugd->op_status == BT_DEACTIVATED);

	bt_address_t address = { { 0 } };
	int connected_type;

	BT_INFO("Bluetooth Audio Event [%d] [%d] [%0x]", result, connected, type);

	if (type == BT_AUDIO_PROFILE_TYPE_A2DP)
		connected_type = BT_STEREO_HEADSET_CONNECTED;
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	else if (type == BT_AUDIO_PROFILE_TYPE_A2DP_SINK)
		connected_type = BT_MUSIC_PLAYER_CONNECTED;
#endif
	else
		connected_type = BT_HEADSET_CONNECTED;

	_bt_util_addr_string_to_addr_type(address.bd_addr, remote_address);

	if (ugd->bt_launch_mode == BT_LAUNCH_CONNECT_AUDIO_SOURCE) {
		int ret;
		bundle *b = NULL;
		b = bundle_create();
		retm_if (!b, "Unable to create bundle");

		bundle_add(b, "event-type", "terminate");

		ret = syspopup_launch("bt-syspopup", b);
		if (0 > ret)
			BT_ERR("Popup launch failed...retry %d", ret);
		bundle_free(b);
		if (connected && result == BT_ERROR_NONE) {
			_bt_ug_destroy(ugd, result);
			return;
		}
	}

	if (connected == 1) {
		if(ugd->popup != NULL) {
			char *bd_addr = evas_object_data_get(ugd->popup, "bd_addr");
			if (bd_addr != NULL) {
				if (g_strcmp0(bd_addr, remote_address) == 0) {
					evas_object_del(ugd->popup);
					ugd->popup = NULL;
				}
			}
		}
	}
	_bt_ipc_update_connected_status(user_data, connected_type,
					connected, result, &address);
	FN_END;
}

void _bt_cb_device_connection_state_changed(bool connected,
						bt_device_connection_info_s *conn_info,
						void *user_data)
{
	FN_START;
	ret_if(!user_data);

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;

	ugd = (bt_ug_data *)user_data;
	dev = _bt_main_get_dev_info_by_address(ugd->paired_device, conn_info->remote_address);
	retm_if(dev == NULL, "dev is NULL");

	BT_INFO("device connection state changed: Connected %d", connected);
	if (!connected && ugd->disconn_req == true) {
		ugd->disconn_req = false;
		if (dev->status != BT_DEV_UNPAIRING)
			dev->status = BT_IDLE;
		_bt_sort_paired_device_list(ugd, dev, dev->is_connected);
		_bt_update_genlist_item((Elm_Object_Item *) dev->genlist_item);
	}
	FN_END;
}
void _bt_cb_adapter_name_changed(char *device_name, void *user_data)
{
	FN_START;

	DBG_SECURE("Name: [%s]", device_name);

	FN_END;
}

void _bt_cb_nap_state_changed(bool connected, const char *remote_address,
				const char *interface_name, void *user_data)
{
	FN_START;
	ret_if(!user_data);
	ret_if(!remote_address);
	bt_ug_data *ugd = (bt_ug_data *)user_data;
	ret_if(ugd->op_status == BT_DEACTIVATING || ugd->op_status == BT_DEACTIVATED);

	bt_address_t address = { { 0 } };

	BT_INFO("Bluetooth NAP Event [%d]", connected);
	DBG_SECURE("interface = %s", interface_name);

	_bt_util_addr_string_to_addr_type(address.bd_addr, remote_address);

	_bt_ipc_update_connected_status(user_data, BT_NETWORK_CONNECTED,
					connected, BT_UG_ERROR_NONE, &address);
	FN_END;
}

void _bt_retry_connection_cb(void *data,
				Evas_Object *obj, void *event_info)
{
	FN_START;
	ret_if (obj == NULL || data == NULL);

	bt_dev_t *dev = (bt_dev_t *)data;
	bt_ug_data *ugd = dev->ugd;
	const char *event = elm_object_text_get(obj);

	if (ugd->popup) {
		BT_DBG("delete popup");
		_bt_update_genlist_item((Elm_Object_Item *)dev->genlist_item);
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	if (!g_strcmp0(event, BT_STR_RETRY)) {
		_bt_main_connect_device(ugd, dev);
	}

	FN_END;
}

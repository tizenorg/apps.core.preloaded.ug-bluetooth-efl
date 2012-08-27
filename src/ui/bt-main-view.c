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

#include <Ecore.h>
#include <eina_list.h>
#include <aul.h>
#include <bluetooth.h>
#include <syspopup_caller.h>
#include <dbus/dbus.h>
#include <vconf.h>

#include "bt-main-ug.h"
#include "bt-string-define.h"
#include "bt-main-view.h"
#include "bt-profile-view.h"
#include "bt-ipc-handler.h"
#include "bt-debug.h"
#include "bt-util.h"
#include "bt-callback.h"
#include "bt-widget.h"
#include "bt-resource.h"
#include "bt-dbus-method.h"

/**********************************************************************
*                                      Static Functions declaration
***********************************************************************/

static void __bt_main_onoff_btn_cb(void *data, Evas_Object *obj,
				   void *event_info);

static void __bt_main_set_controlbar_mode(void *data, int enable_mode);

static service_h __bt_main_get_visibility_result(bt_ug_data *ugd,
						gboolean result);

static service_h __bt_main_get_pick_result(bt_ug_data *ugd,
						gboolean result);

/**********************************************************************
*                                               Static Functions
***********************************************************************/

static char *__bt_main_status_label_get(void *data, Evas_Object *obj,
					const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
	char *dev_name = NULL;
	char *ptr = NULL;

	retv_if(data == NULL, NULL);

	ugd = (bt_ug_data *)data;

	if (!strcmp(part, "elm.text.1")) {
		switch (ugd->op_status) {
		case BT_ACTIVATING:
			g_strlcpy(buf, BT_STR_ACTIVATING_ING,
				BT_GLOBALIZATION_STR_LENGTH);
			break;

		case BT_DEACTIVATING:
			g_strlcpy(buf, BT_STR_DEACTIVATING_ING,
				BT_GLOBALIZATION_STR_LENGTH);
			break;

		default:
			g_strlcpy(buf, BT_STR_BLUETOOTH,
				BT_GLOBALIZATION_STR_LENGTH);
			break;
		}
	} else if (strcmp(part, "elm.text.2") == 0) {
		memset(ugd->phone_name, 0x00, BT_DEVICE_NAME_LENGTH_MAX + 1);

		if (bt_adapter_get_name(&dev_name) ==
					BT_ERROR_NONE) {
			g_strlcpy(ugd->phone_name, dev_name,
				BT_DEVICE_NAME_LENGTH_MAX);
			g_free(dev_name);
		} else {
			_bt_util_get_phone_name(ugd->phone_name,
					sizeof(ugd->phone_name)-1);
		}

		/* Check the utf8 valitation & Fill the NULL in the invalid location*/
                if (!g_utf8_validate(ugd->phone_name, -1, (const char **)&ptr))
                        *ptr = '\0';

		g_strlcpy(buf, ugd->phone_name, BT_GLOBALIZATION_STR_LENGTH);
	} else {

		BT_DBG("empty text for label. \n");
		return NULL;
	}


	FN_END;

	return strdup(buf);
}

static Evas_Object *__bt_main_status_icon_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	Evas_Object *btn = NULL;
	bool activated = FALSE;

	retv_if(data == NULL, NULL);

	ugd = (bt_ug_data *)data;

	if (ugd->op_status == BT_ACTIVATING ||
	     ugd->op_status == BT_DEACTIVATING ||
	      ugd->op_status == BT_SEARCHING) {
		__bt_main_set_controlbar_mode(ugd, BT_CONTROL_BAR_DISABLE);

		if (ugd->op_status != BT_SEARCHING)
			return NULL;
	}

	if (!strcmp(part, "elm.icon")) {
		activated = (ugd->op_status == BT_DEACTIVATED) ? FALSE : TRUE;

		if (activated == TRUE)
			__bt_main_set_controlbar_mode(ugd,
						      BT_CONTROL_BAR_ENABLE);
		else
			__bt_main_set_controlbar_mode(ugd,
						      BT_CONTROL_BAR_DISABLE);

		btn = elm_check_add(obj);
		elm_object_style_set(btn, "on&off");
		evas_object_show(btn);
		evas_object_pass_events_set(btn, EINA_TRUE);
		evas_object_propagate_events_set(btn, EINA_FALSE);
		elm_check_state_set(btn, activated);	/* set on or off */

		/* add smart callback */
		evas_object_smart_callback_add(btn, "changed",
					       __bt_main_onoff_btn_cb, ugd);
		ugd->onoff_btn = btn;
	}

	FN_END;
	return btn;
}

static char *__bt_main_visible_label_get(void *data, Evas_Object *obj,
				      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH+BT_EXTRA_STR_LEN] = { 0 };
	char remain_time[BT_EXTRA_STR_LEN] = { 0 };
	bt_ug_data *ugd = NULL;

	if (data == NULL)
		return NULL;

	ugd = (bt_ug_data *)data;

	if (!strcmp(part, "elm.text.1")) {
		g_strlcpy(buf, BT_STR_VISIBLE, BT_GLOBALIZATION_STR_LENGTH);
	} else if (strcmp(part, "elm.text.2") == 0) {
		if (ugd->visibility_timeout <= 0) {
			_bt_util_get_timeout_string(ugd->visibility_timeout,
						buf, sizeof(buf));
		} else {
			/* Display remain timeout */
			_bt_util_convert_time_to_string(ugd->remain_time,
							remain_time,
							sizeof(remain_time));

			snprintf(buf, sizeof(buf), BT_STR_PS_REMAINING, remain_time);
		}
	} else {
		BT_DBG("empty text for label. \n");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_main_timeout_value_label_get(void *data, Evas_Object *obj,
				      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0 };
	int timeout = 0;
	bt_ug_data *ugd = NULL;
	bt_radio_item *item = NULL;

	retv_if(data == NULL, NULL);

	item = (bt_radio_item *)data;
	retv_if(item->ugd == NULL, NULL);

	ugd = (bt_ug_data *)item->ugd;

	if (!strcmp(part, "elm.text")) {
		timeout = _bt_util_get_timeout_value(item->index);
		_bt_util_get_timeout_string(timeout, buf, sizeof(buf));
	} else {
		BT_DBG("empty text for label. \n");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static Evas_Object *__bt_main_timeout_value_icon_get(void *data, Evas_Object *obj,
				      const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_radio_item *item = NULL;
	Evas_Object *btn = NULL;

	retv_if(data == NULL, NULL);

	item = (bt_radio_item *)data;
	retv_if(item->ugd == NULL, NULL);

	ugd = (bt_ug_data *)item->ugd;

	if (!strcmp(part, "elm.icon")) {
		btn = elm_radio_add(obj);
		elm_radio_state_value_set(btn, item->index);
		elm_radio_group_add(btn, ugd->radio_main);
		elm_radio_value_set(btn, ugd->selected_radio);

		evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);

		evas_object_size_hint_align_set(btn, EVAS_HINT_FILL,
						EVAS_HINT_FILL);
	}

	FN_END;
	return btn;
}

static gboolean __bt_main_visible_timeout_cb(gpointer user_data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ugd = (bt_ug_data *)user_data;

	ugd->remain_time--;

	if (ugd->remain_time <= 0) {
		g_source_remove(ugd->timeout_id);
		ugd->timeout_id = 0;
		ugd->visibility_timeout = 0;
		ugd->remain_time = 0;
		ugd->selected_radio = 0;

		elm_genlist_item_update(ugd->visible_item);

		return FALSE;
	}

	elm_genlist_item_update(ugd->visible_item);

	FN_END;
	return TRUE;
}

static int __bt_idle_destroy_ug(void *data)
{
	FN_START;

	bt_ug_data *ugd = data;
	service_h service = NULL;

	retv_if(ugd == NULL, BT_UG_FAIL);


	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY)
		service = __bt_main_get_visibility_result(ugd, TRUE);
	else if (ugd->bt_launch_mode == BT_LAUNCH_PICK)
		service = __bt_main_get_pick_result(ugd, TRUE);

	_bt_ug_destroy(data, (void *)service);

	if (service)
		service_destroy(service);

	FN_END;
	return BT_UG_ERROR_NONE;
}

static void __bt_main_timeout_value_item_sel(void *data, Evas_Object *obj,
				    void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_radio_item *item = NULL;
	int ret;
	int timeout;

	ret_if(data == NULL);

	item = (bt_radio_item *)data;

	ugd = (bt_ug_data *)item->ugd;

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
				      EINA_FALSE);

	timeout = _bt_util_get_timeout_value(item->index);

	if (timeout < 0) {
		ret = bt_adapter_set_visibility(
			BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE,
			0);
	} else if (timeout == 0) {
		ret = bt_adapter_set_visibility(
			BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE,
			0);
	} else {
		ret = bt_adapter_set_visibility(
			BT_ADAPTER_VISIBILITY_MODE_LIMITED_DISCOVERABLE,
			timeout);

		if (ret == BT_ERROR_NONE) {
			if (ugd->timeout_id) {
				g_source_remove(ugd->timeout_id);
				ugd->timeout_id = 0;
			}

			ugd->remain_time = timeout;

			ugd->timeout_id = g_timeout_add_seconds(1,
					__bt_main_visible_timeout_cb, ugd);
		}
	}

	if (ret != BT_ERROR_NONE) {
		BT_DBG("bt_adapter_set_visibility() failed");
		return;
	}

	ugd->selected_radio = item->index;
	ugd->visibility_timeout = timeout;

	elm_genlist_item_update((Elm_Object_Item *)event_info);
	elm_genlist_item_update(ugd->visible_item);

	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY)
		g_idle_add((GSourceFunc)__bt_idle_destroy_ug, ugd);

	FN_END;
	return;
}

static void __bt_main_timeout_value_del(void *data, Evas_Object *obj)
{
	FN_START;

	bt_radio_item *item = (bt_radio_item *)data;
	if (item)
		free(item);

	FN_END;
}

static gboolean __bt_main_is_connectable_device(bt_dev_t *dev)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retv_if(dev == NULL, FALSE);
	retv_if(dev->ugd == NULL, FALSE);

	ugd = (bt_ug_data *)dev->ugd;

	if(ugd->bt_launch_mode != BT_LAUNCH_NORMAL) {
		/* In no normal mode,
		    connectable devices is only shown in the list */
		BT_DBG("Not in the normal mode");
		return TRUE;
	}

	if (dev->service_list == 0) {
		BT_DBG("No service list");
		return FALSE;
	}

	if ((dev->service_list & BT_SC_HFP_SERVICE_MASK) ||
	     (dev->service_list & BT_SC_HSP_SERVICE_MASK) ||
	      (dev->service_list & BT_SC_A2DP_SERVICE_MASK) ||
	       (dev->service_list & BT_SC_HID_SERVICE_MASK)) {
		/* Connectable device */
		return TRUE;
	}

	FN_END;
	return FALSE;
}

static char *__bt_main_list_label_get(void *data, Evas_Object *obj,
				      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0 };
	bt_dev_t *dev = NULL;

	retv_if(data == NULL, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.text.1") || !strcmp(part, "elm.text")) {
		g_strlcpy(buf, dev->name, BT_GLOBALIZATION_STR_LENGTH);
		BT_DBG("label : %s", buf);
	} else if (strcmp(part, "elm.text.2") == 0) {
		if (dev->status == BT_IDLE) {
			if (__bt_main_is_connectable_device(dev) == 0) {
				snprintf(buf, BT_GLOBALIZATION_STR_LENGTH,
						BT_SET_FONT_SIZE,
						BT_GENLIST_FONT_32_INC,
						BT_STR_PAIRED);
			} else if (dev->connected_mask > 0) {
				snprintf(buf, BT_GLOBALIZATION_STR_LENGTH,
						BT_SET_FONT_SIZE_COLOR,
						BT_GENLIST_FONT_32_INC,
						BT_GENLIST_SUBTEXT_COLOR,
						BT_STR_TAP_TO_DISCONNECT);
			} else {
				snprintf(buf, BT_GLOBALIZATION_STR_LENGTH,
						BT_SET_FONT_SIZE,
						BT_GENLIST_FONT_32_INC,
						BT_STR_TAP_TO_CONNECT);
			}
		} else if (dev->status == BT_CONNECTING) {
			snprintf(buf, BT_GLOBALIZATION_STR_LENGTH,
					BT_SET_FONT_SIZE,
					BT_GENLIST_FONT_32_INC,
					BT_STR_CONNECTING);
		} else if (dev->status == BT_SERVICE_SEARCHING) {
			snprintf(buf, BT_GLOBALIZATION_STR_LENGTH,
					BT_SET_FONT_SIZE,
					BT_GENLIST_FONT_32_INC,
					BT_STR_SEARCHING_SERVICES);
		} else if (dev->status == BT_DISCONNECTING) {
			snprintf(buf, BT_GLOBALIZATION_STR_LENGTH,
					BT_SET_FONT_SIZE_COLOR,
					BT_GENLIST_FONT_32_INC,
					BT_GENLIST_SUBTEXT_COLOR,
					BT_STR_DISCONNECTING);
		}
	} else {		/* for empty item */

		BT_DBG("empty text for label. \n");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static void __bt_main_list_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;
	int ret;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	ugd = dev->ugd;

	if (ugd->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if(ret != BT_ERROR_NONE)
			BT_DBG("Fail to stop discovery: %d", ret);
	}

	if (ugd->connect_req == TRUE) {
		_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_FAILED);
		return;
	}

	/* Create the profile view */
	_bt_profile_create_view(dev);

	FN_END;
}

static Evas_Object *__bt_main_list_icon_get(void *data, Evas_Object *obj,
					    const char *part)
{
	FN_START;

	Evas_Object *icon = NULL;
	char *dev_icon_file = NULL;
	bt_dev_t *dev = NULL;

	retv_if(data == NULL, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.icon.1") || !strcmp(part, "elm.icon")) {
		dev_icon_file =
		    _bt_main_get_device_icon(dev->major_class,
					     dev->minor_class,
					     dev->connected_mask);
		icon = _bt_create_icon(obj, dev_icon_file);
	} else if (!strcmp(part, "elm.icon.2")) {
		BT_DBG("status : %d", dev->status);

		if (dev->status == BT_IDLE) {
			icon = elm_button_add(obj);
			elm_object_style_set(icon, "reveal");
			evas_object_smart_callback_add(icon, "clicked",
					(Evas_Smart_Cb)__bt_main_list_select_cb, (void *)dev);
			evas_object_propagate_events_set(icon, EINA_FALSE);
		} else {
			icon = _bt_create_progressbar(obj, NULL);
		}
	}

	FN_END;
	return icon;
}

static char *__bt_main_searched_label_get(void *data, Evas_Object *obj,
				      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0 };
	bt_dev_t *dev = NULL;

	if (data == NULL)
		return NULL;

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.text")) {
		g_strlcpy(buf, dev->name, BT_GLOBALIZATION_STR_LENGTH);
		BT_DBG("label : %s", buf);
	}

	FN_END;
	return strdup(buf);
}


static Evas_Object *__bt_main_searched_icon_get(void *data,
					Evas_Object *obj, const char *part)
{
	FN_START;

	Evas_Object *icon = NULL;
	char *dev_icon_file = NULL;
	bt_dev_t *dev = NULL;

	retv_if(data == NULL, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.icon.1") || !strcmp(part, "elm.icon")) {
		dev_icon_file =
		    _bt_main_get_device_icon(dev->major_class,
					     dev->minor_class,
					     dev->connected_mask);
		icon = _bt_create_icon(obj, dev_icon_file);
	} else if (!strcmp(part, "elm.icon.2")) {
		if (dev->status != BT_IDLE) {
			icon = _bt_create_progressbar(obj, NULL);
		}
	}

	FN_END;
	return icon;
}

static char *__bt_main_no_device_label_get(void *data, Evas_Object *obj,
				      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0 };

	if (!strcmp(part, "elm.text")) {
		g_strlcpy(buf, BT_STR_NO_DEVICE_FOUND,
				BT_GLOBALIZATION_STR_LENGTH);
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_main_paired_title_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	if (strcmp(part, "elm.text") == 0) {
		/*Label */
		strncpy(buf, BT_STR_PAIRED_DEVICES,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_main_searched_title_label_get(void *data, Evas_Object *obj,
						const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
	bt_ug_data *ugd = NULL;

	retv_if(data == NULL, NULL);

	ugd = (bt_ug_data *)data;

	if (strcmp(part, "elm.text") == 0) {
		/* Label */
		if (ugd->searched_device == NULL ||
		     eina_list_count(ugd->searched_device) == 0) {
			strncpy(buf, BT_STR_BLUETOOTH_DEVICES,
					BT_GLOBALIZATION_STR_LENGTH);
		} else {
			strncpy(buf, BT_STR_AVAILABLE_DEVICES,
					BT_GLOBALIZATION_STR_LENGTH);
		}
	} else {
		BT_DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static Evas_Object *__bt_main_title_icon_get(void *data, Evas_Object *obj,
					     const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	Evas_Object *progressbar = NULL;

	retv_if(data == NULL, NULL);
	retv_if(obj == NULL, NULL);
	retv_if(part == NULL, NULL);

	ugd = (bt_ug_data *)data;

	if (!strcmp(part, "elm.icon") && ugd->op_status == BT_SEARCHING) {
		progressbar = _bt_create_progressbar(obj, "list_process_small");
	}

	FN_END;
	return progressbar;
}

static void __bt_main_status_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
				      EINA_FALSE);

	__bt_main_onoff_btn_cb(data, ugd->onoff_btn, NULL);

	FN_END;
	return;
}

static void __bt_main_visible_item_sel(void *data, Evas_Object *obj,
				    void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	Eina_Bool expanded = EINA_FALSE;

	ret_if(data == NULL);
	ret_if(event_info == NULL);

	ugd = (bt_ug_data *)data;

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
				      EINA_FALSE);

	expanded = elm_genlist_item_expanded_get(event_info);
	elm_genlist_item_expanded_set(event_info, !expanded);

	FN_END;
	return;
}

static service_h __bt_main_get_visibility_result(bt_ug_data *ugd,
						gboolean result)
{
	service_h service = NULL;
	char mode_str[BT_RESULT_STR_MAX] = { 0 };
	const char *result_str;

	retv_if(ugd == NULL, NULL);

	service_create(&service);

	retv_if(service == NULL, NULL);

	if (result == TRUE)
		result_str = BT_RESULT_SUCCESS;
	else
		result_str = BT_RESULT_FAIL;

	if (service_add_extra_data(service, "result",
					result_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	snprintf(mode_str, BT_RESULT_STR_MAX, "%d", (int)ugd->selected_radio);

	if (service_add_extra_data(service, "visibility",
					(const char *)mode_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	return service;
}

static service_h __bt_main_get_pick_result(bt_ug_data *ugd,
						gboolean result)
{
	service_h service = NULL;
	const char *result_str;
	char address[BT_ADDRESS_STR_LEN] = { 0 };
	char value_str[BT_RESULT_STR_MAX] = { 0 };
	bt_dev_t *dev;

	retv_if(ugd == NULL, NULL);
	retv_if(ugd->pick_device == NULL, NULL);

	dev = ugd->pick_device;

	service_create(&service);

	retv_if(service == NULL, NULL);

	if (result == TRUE)
		result_str = BT_RESULT_SUCCESS;
	else
		result_str = BT_RESULT_FAIL;

	if (service_add_extra_data(service, "result",
					result_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	_bt_util_addr_type_to_addr_result_string(address, dev->bd_addr);

	if (service_add_extra_data(service, "address",
					(const char *)address) < 0) {
		BT_DBG("Fail to add extra data");
	}

	if (service_add_extra_data(service, "name",
					(const char *)dev->name) < 0) {
		BT_DBG("Fail to add extra data");
	}

	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->rssi);

	if (service_add_extra_data(service, "rssi",
					(const char *)value_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->is_bonded);

	if (service_add_extra_data(service, "is_bonded",
					(const char *)value_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->major_class);

	if (service_add_extra_data(service, "major_class",
					(const char *)value_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->minor_class);

	if (service_add_extra_data(service, "minor_class",
					(const char *)value_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%ld",
				(long int)dev->service_class);

	if (service_add_extra_data(service, "service_class",
					(const char *)value_str) < 0) {
		BT_DBG("Fail to add extra data");
	}

	return service;
}

static void __bt_main_quit_btn_cb(void *data, Evas_Object *obj,
				  void *event_info)
{
	FN_START;

	service_h service = NULL;
	bt_ug_data *ugd = (bt_ug_data *)data;

	ret_if(ugd == NULL);

	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY) {
		service = __bt_main_get_visibility_result(ugd, FALSE);

		_bt_ug_destroy(data, (void *)service);

		if (service)
			service_destroy(service);
	} else if (ugd->bt_launch_mode == BT_LAUNCH_PICK) {
		service_create(&service);

		if (service == NULL) {
			_bt_ug_destroy(data, NULL);
			return;
		}

		if (service_add_extra_data(service, "result",
						BT_RESULT_FAIL) < 0) {
			BT_DBG("Fail to add extra data");
		}

		_bt_ug_destroy(data, (void *)service);

		service_destroy(service);
	} else {
		_bt_ug_destroy(data, NULL);
	}

	FN_END;
}

static int __bt_main_enable_bt(void *data)
{
	FN_START;
	int ret;
	retv_if(data == NULL, -1);
	bt_ug_data *ugd = (bt_ug_data *)data;

	if (_bt_util_is_battery_low() == TRUE) {
		/* Battery is critical low */
		_bt_main_create_information_popup(ugd,BT_STR_LOW_BATTERY);
		elm_genlist_item_update(ugd->status_item);
		return -1;
	}

	ret = bt_adapter_enable();
	if (ret != BT_ERROR_NONE) {
		BT_ERR("Failed to enable bluetooth [%d]", ret);
	} else {
		ugd->op_status = BT_ACTIVATING;
		elm_object_item_disabled_set(ugd->scan_item, EINA_TRUE);
	}

	FN_END;
	return 0;
}

static void __bt_main_onoff_btn_cb(void *data, Evas_Object *obj,
				   void *event_info)
{
	FN_START;
	ret_if(data == NULL);

	int ret;
	bt_ug_data *ugd = (bt_ug_data *)data;

	if (ugd->op_status == BT_DEACTIVATED) {
		ret = __bt_main_enable_bt(data);
	} else {
		ret = bt_adapter_disable();
		if (ret != BT_ERROR_NONE) {
			BT_ERR("Failed to disable bluetooth [%d]", ret);
		} else {
			ugd->op_status = BT_DEACTIVATING;
			elm_object_item_disabled_set(ugd->scan_item, EINA_TRUE);
		}
	}

	elm_genlist_item_update(ugd->status_item);

	FN_END;
}

static void __bt_main_controlbar_btn_cb(void *data, Evas_Object *obj,
					      void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retm_if(data == NULL, "Invalid argument: bt_ug_data is NULL\n");

	ugd = (bt_ug_data *)data;

	if (ugd->op_status == BT_SEARCHING) {
		if (bt_adapter_stop_device_discovery() == BT_ERROR_NONE) {
			elm_toolbar_item_icon_set(ugd->scan_item,
						BT_ICON_CONTROLBAR_SCAN);
		}
	} else {
		_bt_main_scan_device(ugd);
	}

	FN_END;
}

static void __bt_main_disconnect_cb(void *data, Evas_Object *obj,
				    void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;

	retm_if(data == NULL, "Invalid argument: data is NULL\n");

	dev = (bt_dev_t *)data;
	retm_if(dev->ugd == NULL, "ugd is NULL\n");

	ugd = dev->ugd;

	if (ugd->popup) {
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	_bt_main_disconnect_device(ugd, dev);

	FN_END;
}

static void __bt_main_popup_menu_cb(void *data, Evas_Object *obj,
				    void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retm_if(data == NULL, "Invalid argument: data is NULL\n");

	ugd = (bt_ug_data *)data;

	if (ugd->popup_menu)
		evas_object_del(ugd->popup_menu);

	ugd->popup_menu = NULL;

	if (ugd->popup == NULL)
		ugd->back_cb = NULL;

	FN_END;
}

static char *__bt_main_popup_menu_label_get(void *data, Evas_Object *obj,
					    const char *part)
{
	FN_START;

	bt_dev_t *dev = NULL;
	char buf[BT_MAX_MENU_NAME_LEN] = { 0, };
	int index = 0;

	retvm_if(data == NULL, NULL, "Invalid argument: data is NULL\n");

	index = (int)data;
	dev = (bt_dev_t *)evas_object_data_get(obj, "dev_info");

	retvm_if(dev == NULL, NULL, "dev is NULL\n");

	if (strcmp(part, "elm.text") != 0) {
		BT_DBG("It is not in elm.text part\n");
		return NULL;
	}

	switch (index) {
	case BT_CONNECT_MENU:
		snprintf(buf, sizeof(buf), BT_STR_DISCONNECT);
		break;
	default:
		snprintf(buf, sizeof(buf), " ");
		break;
	}

	BT_DBG("popup_menu_label_get() end. %s", buf);

	FN_END;
	return strdup(buf);
}

/* change popup style for popup menu */
static void __bt_main_select_menu_cb(void *data, Evas_Object *obj,
				     void *event_info)
{
	FN_START;

	Elm_Object_Item *selected_menu = NULL;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;

	retm_if(data == NULL, "Invalid argument: data is NULL\n");
	retm_if(event_info == NULL, "Invalid argument: event_info is NULL\n");

	ugd = (bt_ug_data *)data;

	selected_menu = (Elm_Object_Item *)event_info;

	elm_genlist_item_selected_set(selected_menu, EINA_FALSE);

	dev = _bt_main_get_dev_info(ugd->paired_device, ugd->paired_item);
	retm_if(dev == NULL, "dev is NULL\n");

	ugd->back_cb = NULL;

	if (_bt_is_profile_connected(BT_HEADSET_CONNECTED, ugd->conn,
						dev->bd_addr) == TRUE) {
		BT_DBG("Disconnecting AG service \n");
		if (bt_audio_disconnect(dev->addr_str,
				BT_AUDIO_PROFILE_TYPE_ALL) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_DBG("Fail to connect Headset device");
		}
	} else if (_bt_is_profile_connected(BT_STEREO_HEADSET_CONNECTED, ugd->conn,
						dev->bd_addr) == TRUE) {
		BT_DBG("Disconnecting AV service \n");
		if (bt_audio_disconnect(dev->addr_str,
				BT_AUDIO_PROFILE_TYPE_A2DP) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_DBG("Fail to connect Headset device");
		}
	} else if (_bt_is_profile_connected(BT_HID_CONNECTED, ugd->conn,
						dev->bd_addr) == TRUE) {
		BT_DBG("Disconnecting HID service!!\n");

		if (bt_hid_host_disconnect(dev->addr_str) == BT_ERROR_NONE) {
			dev->status = BT_DISCONNECTING;
			elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);
		} else {
			BT_DBG("Fail to connect HID device");
		}
		return;
	}

	elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);
	FN_END;
}

static void __bt_main_set_controlbar_mode(void *data, int enable_mode)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bool mode;
	int count = 0;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	mode = enable_mode ? FALSE : TRUE;

	if (ugd->paired_device)
		count = eina_list_count(ugd->paired_device);

	if (count == 0 || (ugd->op_status != BT_ACTIVATED)) {
		BT_DBG("device count: %d, op_status: %d",
		       eina_list_count(ugd->paired_device), ugd->op_status);
		mode = TRUE;
	}

	FN_END;
}

static void __bt_main_paired_item_sel_cb(void *data, Evas_Object *obj,
					 void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;
	Evas_Object *btn = NULL;
	Evas_Object *popup = NULL;
	Evas_Object *popup_btn = NULL;
	char msg[BT_DISCONNECT_TEXT_LENGTH] = { 0 };
	int ret;

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
				      EINA_FALSE);

	retm_if(data == NULL, "Invalid argument: bt_ug_data is NULL\n");

	ugd = (bt_ug_data *)data;
	item = (Elm_Object_Item *)event_info;

	ret_if(ugd->waiting_service_response == TRUE);
	ret_if(ugd->op_status == BT_PAIRING);

	if (ugd->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if(ret == BT_ERROR_NONE) {
			ugd->op_status = BT_ACTIVATED;

			if (ugd->status_item)
				elm_genlist_item_update(ugd->status_item);

			if (ugd->paired_title)
				elm_genlist_item_update(ugd->paired_title);

			if (ugd->searched_title)
				elm_genlist_item_update(ugd->searched_title);
		}
		return;
	}

	dev = _bt_main_get_dev_info(ugd->paired_device, item);
	retm_if(dev == NULL, "Invalid argument: device info is NULL\n");
	retm_if(dev->status != BT_IDLE,
			"Connecting / Disconnecting is in progress");

	if ((ugd->auto_service_search || ugd->waiting_service_response) &&
		  (dev->service_list == 0)) {
		ugd->paired_item = item;

		if (ugd->popup) {
			evas_object_del(ugd->popup);
			ugd->popup = NULL;
		}
		ugd->popup =
		    _bt_create_popup(ugd->win_main, BT_STR_INFORMATION,
				     BT_STR_GETTING_SERVICE_LIST,
				     _bt_main_popup_del_cb, ugd, 2);

		btn = elm_button_add(ugd->popup);
		elm_object_text_set(btn, BT_STR_OK);
		elm_object_part_content_set(ugd->popup, "button1", btn);
		evas_object_smart_callback_add(btn, "clicked",
			(Evas_Smart_Cb)_bt_main_popup_del_cb, ugd);

		return;
	}

	if (ugd->bt_launch_mode == BT_LAUNCH_NORMAL ||
	     ugd->bt_launch_mode == BT_LAUNCH_USE_NFC) {

		ugd->paired_item = item;

		if (dev->service_list == 0 &&
		     ugd->auto_service_search == FALSE) {

			if(bt_device_start_service_search(
				(const char *)dev->addr_str) == BT_ERROR_NONE) {

				dev->status = BT_SERVICE_SEARCHING;
				ugd->waiting_service_response = TRUE;
				ugd->request_timer =
				    ecore_timer_add(BT_SEARCH_SERVICE_TIMEOUT,
					    (Ecore_Task_Cb)
					    _bt_main_service_request_cb,
					    ugd);

				elm_genlist_item_update(ugd->paired_item);
				return;
			} else {
				BT_DBG("service search error");
				return;
			}
		}

		if (dev->connected_mask == 0) {
			/* Not connected case */
			_bt_main_connect_device(ugd, dev);
		} else {
			/* connected case */
			snprintf(msg, sizeof(msg), "%s %s<br>%s", BT_STR_END_CONNECTION,
							dev->name,
							BT_STR_DISCONNECT_Q);

			popup = _bt_create_popup(ugd->win_main, BT_STR_INFORMATION,
					msg,
					_bt_main_popup_del_cb, ugd, 0);

			if (popup == NULL)
				return;

			ugd->popup = popup;

			popup_btn = elm_button_add(popup);
			elm_object_text_set(popup_btn, BT_STR_OK);
			elm_object_part_content_set(popup, "button1", popup_btn);
			evas_object_smart_callback_add(popup_btn, "clicked",
						__bt_main_disconnect_cb, dev);

			popup_btn = elm_button_add(popup);
			elm_object_text_set(popup_btn, BT_STR_CANCEL);
			elm_object_part_content_set(popup, "button2", popup_btn);
			evas_object_smart_callback_add(popup_btn, "clicked",
						_bt_main_popup_del_cb, ugd);
		}
	} else if (ugd->bt_launch_mode == BT_LAUNCH_SEND_FILE) {
		obex_ipc_param_t param;
		char *value = NULL;

		if (_bt_util_is_battery_low() == TRUE) {
			/* Battery is critical low */
			_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
			return;
		}

		BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
		       dev->bd_addr[0], dev->bd_addr[1],
		       dev->bd_addr[2], dev->bd_addr[3],
		       dev->bd_addr[4], dev->bd_addr[5]);

		memset(&param, 0x00, sizeof(obex_ipc_param_t));
		memcpy(param.param2, dev->bd_addr, BT_ADDRESS_LENGTH_MAX);

		if (service_get_extra_data(ugd->service, "filecount", &value) < 0)
			BT_DBG("Get data error");

		ret_if(value == NULL);

		param.param3 = atoi(value);
		free(value);
		value = NULL;

		if (service_get_extra_data(ugd->service, "files", &value) < 0)
			BT_DBG("Get data error");

		ret_if(value == NULL);

		param.param4 = value;
		param.param5 = g_strdup("normal");

		if (_bt_ipc_send_obex_message(&param, ugd) != BT_UG_ERROR_NONE) {
			_bt_main_launch_syspopup(ugd, BT_SYSPOPUP_REQUEST_NAME,
					BT_STR_UNABLE_TO_SEND,
					BT_SYSPOPUP_ONE_BUTTON_TYPE);
			ugd->syspoup_req = TRUE;
		}

		free(value);
		g_free(param.param5);

		_bt_ug_destroy(ugd, NULL);
	} else if (ugd->bt_launch_mode == BT_LAUNCH_PICK) {
		ugd->pick_device = dev;
		g_idle_add((GSourceFunc)__bt_idle_destroy_ug, ugd);
	}

	FN_END;
}

static void __bt_main_searched_item_sel_cb(void *data, Evas_Object *obj,
					   void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;
	int ret;

	elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
				      EINA_FALSE);

	retm_if(data == NULL, "Invalid argument: bt_ug_data is NULL\n");

	ugd = (bt_ug_data *)data;
	if (ugd->op_status == BT_PAIRING || ugd->syspoup_req == TRUE)
		return;

	item = (Elm_Object_Item *)event_info;

	dev = _bt_main_get_dev_info(ugd->searched_device,
				  (Elm_Object_Item *)event_info);
	retm_if(dev == NULL, "Invalid argument: device info is NULL\n");

	if (ugd->bt_launch_mode == BT_LAUNCH_USE_NFC) {
		char address[18] = { 0 };
		service_h service = NULL;

		service_create(&service);

		ret_if(service == NULL);

		BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
		       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
		       dev->bd_addr[4], dev->bd_addr[5]);

		_bt_util_addr_type_to_addr_string(address, dev->bd_addr);

		if (service_add_extra_data(service, "device_address",
						(const char *)address) < 0) {
			BT_DBG("Fail to add extra data");
		}

		if (service_add_extra_data(service, "device_name",
						(const char *)dev->name) < 0) {
			BT_DBG("Fail to add extra data");
		}

		_bt_ug_destroy(ugd, (void *)service);

		service_destroy(service);

		return;
	}else if (ugd->bt_launch_mode == BT_LAUNCH_PICK) {
		ugd->pick_device = dev;
		g_idle_add((GSourceFunc)__bt_idle_destroy_ug, ugd);
		return;
	}

	ugd->searched_item = item;

	if (ugd->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if(ret == BT_ERROR_NONE)
			ugd->op_status = BT_ACTIVATED;
		else
			return;
	} else {
		if (_bt_util_is_battery_low() == TRUE) {
			/* Battery is critical low */
			_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
			return;
		}

		if (_bt_main_request_pairing_with_effect(ugd, item) !=
		    BT_UG_ERROR_NONE)
			ugd->searched_item = NULL;
	}

	FN_END;
}

static void __bt_main_item_expanded(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;

	Elm_Object_Item *it = event_info;
	Elm_Object_Item *git = NULL;
	Evas_Object *gl = elm_object_item_widget_get(it);
	bt_radio_item *item = NULL;
	int i = 0;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);
	ret_if(gl == NULL);

	ugd = (bt_ug_data *)data;

	for (i = 0; i < BT_MAX_TIMEOUT_ITEMS; i++) {
		item = calloc(1, sizeof(bt_radio_item));
		ret_if(item == NULL);

		item->index = i;
		item->ugd = ugd;

		git = elm_genlist_item_append(gl, ugd->timeout_value_itc,
					(void *)item, it,
					ELM_GENLIST_ITEM_NONE,
					__bt_main_timeout_value_item_sel,
					(void *)item);

		item->it = git;
	}

	FN_END;
}

static void __bt_main_item_contracted(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_subitems_clear(item);

	FN_END;
}

static Evas_Object *__bt_main_add_genlist_dialogue(Evas_Object *parent,
						   bt_ug_data *ugd)
{
	FN_START;
	retv_if(ugd == NULL, NULL);

	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;

	genlist = elm_genlist_add(parent);

	/* We shoud set the mode to compress
	     for using dialogue/2text.2icon.3.tb */
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);

	elm_object_style_set(genlist, "dialogue");

	ugd->radio_main = elm_radio_add(genlist);
	elm_radio_state_value_set(ugd->radio_main, 0);
	elm_radio_value_set(ugd->radio_main, 0);

	ugd->selected_radio = _bt_util_get_timeout_index(ugd->visibility_timeout);

	/* Set item class for dialogue seperator */
	ugd->sp_itc = elm_genlist_item_class_new();
	retv_if (ugd->sp_itc == NULL, NULL);

	ugd->sp_itc->item_style = "dialogue/separator/21/with_line";
	ugd->sp_itc->func.content_get = NULL;
	ugd->sp_itc->func.text_get = NULL;
	ugd->sp_itc->func.state_get = NULL;
	ugd->sp_itc->func.del = NULL;

	/* Set item class for dialogue end padding */
	ugd->end_itc = elm_genlist_item_class_new();
	retv_if (ugd->end_itc == NULL, NULL);

	ugd->end_itc->item_style = "dialogue/separator/end";
	ugd->end_itc->func.content_get = NULL;
	ugd->end_itc->func.text_get = NULL;
	ugd->end_itc->func.state_get = NULL;
	ugd->end_itc->func.del = NULL;

	/* Set item class for bluetooth status and on/off button */
	ugd->status_itc = elm_genlist_item_class_new();
	retv_if (ugd->status_itc == NULL, NULL);

	ugd->status_itc->item_style = "dialogue/2text.1icon.6";
	ugd->status_itc->func.text_get = __bt_main_status_label_get;
	ugd->status_itc->func.content_get = __bt_main_status_icon_get;
	ugd->status_itc->func.state_get = NULL;
	ugd->status_itc->func.del = NULL;

	if (ugd->bt_launch_mode == BT_LAUNCH_NORMAL) {
		/* Set item class for visibility */
		ugd->visible_itc = elm_genlist_item_class_new();
		retv_if (ugd->visible_itc == NULL, NULL);

		ugd->visible_itc->item_style = "dialogue/2text.3/expandable";
		ugd->visible_itc->func.text_get = __bt_main_visible_label_get;
		ugd->visible_itc->func.content_get = NULL;
		ugd->visible_itc->func.state_get = NULL;
		ugd->visible_itc->func.del = NULL;
	}

	/* Set item class for paired dialogue title */
	ugd->paired_title_itc = elm_genlist_item_class_new();
	retv_if (ugd->paired_title_itc == NULL, NULL);

	ugd->paired_title_itc->item_style = "dialogue/title";
	ugd->paired_title_itc->func.text_get = __bt_main_paired_title_label_get;
	ugd->paired_title_itc->func.content_get = NULL;
	ugd->paired_title_itc->func.state_get = NULL;
	ugd->paired_title_itc->func.del = NULL;

	/* Set item class for searched dialogue title */
	ugd->searched_title_itc = elm_genlist_item_class_new();
	retv_if (ugd->searched_title_itc == NULL, NULL);

	ugd->searched_title_itc->item_style = "dialogue/title";
	ugd->searched_title_itc->func.text_get =
	    __bt_main_searched_title_label_get;
	ugd->searched_title_itc->func.content_get = __bt_main_title_icon_get;
	ugd->searched_title_itc->func.state_get = NULL;
	ugd->searched_title_itc->func.del = NULL;

	/* Set item class for paired device */
	ugd->device_itc = elm_genlist_item_class_new();
	retv_if (ugd->device_itc == NULL, NULL);

	if (ugd->bt_launch_mode == BT_LAUNCH_PICK)
		ugd->device_itc->item_style = "dialogue/1text.2icon";
	else
		ugd->device_itc->item_style = "dialogue/2text.2icon.3.tb";

	ugd->device_itc->func.text_get = __bt_main_list_label_get;
	ugd->device_itc->func.content_get = __bt_main_list_icon_get;
	ugd->device_itc->func.state_get = NULL;
	ugd->device_itc->func.del = NULL;

	/* Set item class for searched device */
	ugd->searched_itc = elm_genlist_item_class_new();
	retv_if (ugd->searched_itc == NULL, NULL);

	ugd->searched_itc->item_style = "dialogue/1text.2icon";
	ugd->searched_itc->func.text_get = __bt_main_searched_label_get;
	ugd->searched_itc->func.content_get = __bt_main_searched_icon_get;
	ugd->searched_itc->func.state_get = NULL;
	ugd->searched_itc->func.del = NULL;

	/* Set item class for no device */
	ugd->no_device_itc = elm_genlist_item_class_new();
	retv_if (ugd->no_device_itc == NULL, NULL);

	ugd->no_device_itc->item_style = "dialogue/1text";
	ugd->no_device_itc->func.text_get = __bt_main_no_device_label_get;
	ugd->no_device_itc->func.content_get = NULL;
	ugd->no_device_itc->func.state_get = NULL;
	ugd->no_device_itc->func.del = NULL;

	/* Set item class for timeout value */
	ugd->timeout_value_itc = elm_genlist_item_class_new();
	retv_if(ugd->timeout_value_itc == NULL, NULL);

	ugd->timeout_value_itc->item_style = "dialogue/1text.1icon.2/expandable2";
	ugd->timeout_value_itc->func.text_get = __bt_main_timeout_value_label_get;
	ugd->timeout_value_itc->func.content_get = __bt_main_timeout_value_icon_get;
	ugd->timeout_value_itc->func.state_get = NULL;
	ugd->timeout_value_itc->func.del = __bt_main_timeout_value_del;

	/* Seperator */
	git = elm_genlist_item_append(genlist, ugd->sp_itc, ugd, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	/* Status and on/off button */
	git = elm_genlist_item_append(genlist, ugd->status_itc, ugd, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    __bt_main_status_item_sel, ugd);
	ugd->status_item = git;

	if (ugd->bt_launch_mode == BT_LAUNCH_NORMAL) {
		/* visibility */
		git = elm_genlist_item_append(genlist, ugd->visible_itc, ugd,
					NULL, ELM_GENLIST_ITEM_TREE,
					__bt_main_visible_item_sel, ugd);
		ugd->visible_item = git;
		elm_object_item_disabled_set(ugd->visible_item, EINA_TRUE);
	}

	evas_object_show(genlist);

	evas_object_smart_callback_add(genlist, "expanded", __bt_main_item_expanded, ugd);
	evas_object_smart_callback_add(genlist, "contracted", __bt_main_item_contracted, ugd);

	FN_END;
	return genlist;
}

static Evas_Object *__bt_main_add_visibility_dialogue(Evas_Object *parent,
						   bt_ug_data *ugd)
{
	FN_START;
	retv_if(ugd == NULL, NULL);

	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;

	genlist = elm_genlist_add(parent);

	elm_object_style_set(genlist, "dialogue");

	ugd->radio_main = elm_radio_add(genlist);
	elm_radio_state_value_set(ugd->radio_main, 0);
	elm_radio_value_set(ugd->radio_main, 0);

	ugd->selected_radio = _bt_util_get_timeout_index(ugd->visibility_timeout);

	/* Set item class for dialogue seperator */
	ugd->sp_itc = elm_genlist_item_class_new();
	retv_if (ugd->sp_itc == NULL, NULL);

	ugd->sp_itc->item_style =  "dialogue/separator/21/with_line";
	ugd->sp_itc->func.content_get = NULL;
	ugd->sp_itc->func.text_get = NULL;
	ugd->sp_itc->func.state_get = NULL;
	ugd->sp_itc->func.del = NULL;

	/* Set item class for bluetooth status and on/off button */
	ugd->status_itc = elm_genlist_item_class_new();
	retv_if (ugd->status_itc == NULL, NULL);

	ugd->status_itc->item_style = "dialogue/2text.1icon.6";
	ugd->status_itc->func.text_get = __bt_main_status_label_get;
	ugd->status_itc->func.content_get = __bt_main_status_icon_get;
	ugd->status_itc->func.state_get = NULL;
	ugd->status_itc->func.del = NULL;

	/* Set item class for visibility */
	ugd->visible_itc = elm_genlist_item_class_new();
	retv_if (ugd->visible_itc == NULL, NULL);

	ugd->visible_itc->item_style = "dialogue/2text.3/expandable";
	ugd->visible_itc->func.text_get = __bt_main_visible_label_get;
	ugd->visible_itc->func.content_get = NULL;
	ugd->visible_itc->func.state_get = NULL;
	ugd->visible_itc->func.del = NULL;

	/* Set item class for timeout value */
	ugd->timeout_value_itc = elm_genlist_item_class_new();
	retv_if(ugd->timeout_value_itc == NULL, NULL);

	ugd->timeout_value_itc->item_style = "dialogue/1text.1icon.2/expandable2";
	ugd->timeout_value_itc->func.text_get = __bt_main_timeout_value_label_get;
	ugd->timeout_value_itc->func.content_get = __bt_main_timeout_value_icon_get;
	ugd->timeout_value_itc->func.state_get = NULL;
	ugd->timeout_value_itc->func.del = __bt_main_timeout_value_del;

	/* Seperator */
	git = elm_genlist_item_append(genlist, ugd->sp_itc, ugd, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	/* Status and on/off button */
	git = elm_genlist_item_append(genlist, ugd->status_itc, ugd, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    __bt_main_status_item_sel, ugd);
	ugd->status_item = git;

	/* visibility */
	git = elm_genlist_item_append(genlist, ugd->visible_itc, ugd,
				NULL, ELM_GENLIST_ITEM_TREE,
				__bt_main_visible_item_sel, ugd);
	ugd->visible_item = git;
	elm_object_item_disabled_set(ugd->visible_item, EINA_TRUE);

	evas_object_show(genlist);

	evas_object_smart_callback_add(genlist, "expanded", __bt_main_item_expanded, ugd);
	evas_object_smart_callback_add(genlist, "contracted", __bt_main_item_contracted, ugd);

	FN_END;
	return genlist;
}

static gboolean __bt_main_system_popup_timer_cb(gpointer user_data)
{
	FN_START;

	int ret = 0;
	bt_ug_data *ugd = NULL;
	bundle *b = NULL;

	retv_if(user_data == NULL, FALSE);

	ugd = (bt_ug_data *)user_data;

	b = ugd->popup_bundle;

	ugd->popup_bundle = NULL;
	ugd->popup_timer = 0;

	if (NULL == b) {
		BT_DBG("bundle is NULL\n");
		return FALSE;
	}

	ret = syspopup_launch("bt-syspopup", b);
	if (0 > ret) {
		BT_DBG("Sorry Can not launch popup\n");
	} else {
		BT_DBG("Finally Popup launched \n");
	}

	bundle_free(b);

	FN_END;
	return (0 > ret) ? TRUE : FALSE;
}

static char *__bt_main_get_name(bt_dev_t *dev)
{
	FN_START;

	char *conv_str = NULL;
	char tmp_str[BT_DEVICE_NAME_LENGTH_MAX + 1] = { 0, };
	char buf[BT_FILE_NAME_LEN_MAX + 1] = { 0, };

	retv_if(dev == NULL, NULL);

	if (strlen(dev->name) > 0) {
		strncpy(tmp_str, dev->name, BT_DEVICE_NAME_LENGTH_MAX);
		tmp_str[BT_DEVICE_NAME_LENGTH_MAX] = '\0';

		conv_str = elm_entry_utf8_to_markup(tmp_str);
		if (conv_str)
			strncpy(buf, conv_str, BT_DEVICE_NAME_LENGTH_MAX);
		else
			strncpy(buf, tmp_str, BT_DEVICE_NAME_LENGTH_MAX);
	}

	if (conv_str)
		free(conv_str);

	FN_END;
	return strdup(buf);
}

static bool __bt_main_get_mime_type(char *file)
{
	FN_START;

	char mime_type[BT_FILE_NAME_LEN_MAX] = {0, };
	int len = strlen("image");

	retv_if(file == NULL, FALSE);

	if (aul_get_mime_from_file(file, mime_type,
		     BT_FILE_NAME_LEN_MAX) == AUL_R_OK) {
		BT_DBG(" mime type    =%s \n \n \n", mime_type);
		if (0 != strncmp(mime_type, "image", len))
			return FALSE;
	} else {
		BT_DBG("Error getting mime type");
		return FALSE;
	}

	FN_END;
	return TRUE;
}

static bool __bt_main_is_image_file(service_h service)
{
	FN_START;

	char *value = NULL;
	int number_of_files = 0;
	char *token = NULL;
	char *param = NULL;
	int i = 0;

	retvm_if(service == NULL, FALSE, "Invalid data bundle");

	if (service_get_extra_data(service, "filecount", &value) < 0)
		BT_DBG("Get data error");

	retv_if(value == NULL, FALSE);

	number_of_files = atoi(value);
	BT_DBG("[%d] files\n", number_of_files);
	free(value);
	value = NULL;

	if (number_of_files <= 0) {
		BT_DBG("No File\n");
		return FALSE;
	}

	if (service_get_extra_data(service, "files", &value) < 0)
		BT_DBG("Get data error");

	retv_if(value == NULL, FALSE);

	param = value;
	while (((token = strstr(param, "?")) != NULL) &&
			i < number_of_files) {
		*token = '\0';
		if (!__bt_main_get_mime_type(param)) {
			*token = '?';
			free(value);
			return FALSE;
		}
		BT_DBG("File [%d] [%s]\n", i, param);
		*token = '?';
		param = token + 1;
		i++;
	}
	if (i == (number_of_files - 1)) {
		if (!__bt_main_get_mime_type(param)) {
			free(value);
			return FALSE;
		}
		BT_DBG("File [%d] [%s]\n", i, param);
	} else {
		BT_DBG("Count not match : [%d] / [%d]\n",
			number_of_files, i);
		free(value);
		return FALSE;
	}

	free(value);

	FN_END;
	return TRUE;
}

/**********************************************************************
*                                              Common Functions
***********************************************************************/

void _bt_main_scan_device(bt_ug_data *ugd)
{
	FN_START;
	int ret;

	ret_if(ugd == NULL);

	if (ugd->op_status != BT_DEACTIVATED &&
	     ugd->op_status != BT_ACTIVATED) {
		BT_DBG("current bluetooth status [%d]", ugd->op_status);
		return;
	}

	if (_bt_util_is_battery_low() == TRUE) {
		/* Battery is critical low */
		_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
		return;
	}

	if (ugd->op_status == BT_DEACTIVATED) {
		ret = __bt_main_enable_bt((void *)ugd);
		if(!ret) {
			/* After activating, searching start by this flag. */
			ugd->search_req = TRUE;
			elm_object_item_disabled_set(ugd->scan_item, EINA_TRUE);
		}
	} else {
		_bt_main_remove_all_searched_devices(ugd);

		ret = bt_adapter_start_device_discovery();
		if (!ret) {
			ugd->op_status = BT_SEARCHING;
			elm_toolbar_item_icon_set(ugd->scan_item,
						BT_ICON_CONTROLBAR_STOP);

			if (ugd->searched_title == NULL)
				_bt_main_add_searched_title(ugd);
		} else {
			BT_DBG("Operation failed : Error Cause[%d]", ret);
		}
	}

	elm_genlist_item_update(ugd->status_item);

	FN_END;
}

void _bt_main_draw_selection_info(bt_ug_data *ugd, char *message)
{
	if (ugd->selectioninfo)
		evas_object_del(ugd->selectioninfo);

	ugd->selectioninfo = _bt_create_selectioninfo(ugd->win_main,
					message, ugd->rotation,
					_bt_main_selectioninfo_hide_cb,
					ugd, BT_DELETED_TIMEOUT);
}

int _bt_main_service_request_cb(void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retvm_if(data == NULL, BT_UG_FAIL,
		 "Invalid argument: bt_ug_data is NULL\n");

	ugd = (bt_ug_data *)data;

	if (ugd->request_timer) {
		ecore_timer_del(ugd->request_timer);
		ugd->request_timer = NULL;
	}

	ugd->auto_service_search = FALSE;

	/* Need to modify API: Address parameter */
	if (ugd->waiting_service_response == TRUE) {
		bt_dev_t *dev = NULL;

		ugd->waiting_service_response = FALSE;
		bt_device_cancel_service_search();

		dev =
		    _bt_main_get_dev_info(ugd->paired_device, ugd->paired_item);
		retvm_if(dev == NULL, BT_UG_FAIL, "dev is NULL\n");

		dev->status = BT_IDLE;
		elm_genlist_item_update(ugd->paired_item);

		_bt_main_connect_device(ugd, dev);
	} else {
		ugd->paired_item = NULL;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

char *_bt_main_get_device_icon(int major_class, int minor_class, int connected)
{
	FN_START;
	char *icon = BT_ICON_UNKNOWN;

	BT_DBG("major_class: %d, minor_class: %d\n", major_class, minor_class);

	switch (major_class) {
	case BT_MAJOR_DEV_CLS_COMPUTER:
		icon = (connected == 0) ? BT_ICON_PC : BT_ICON_CONNECTED_PC;
		break;
	case BT_MAJOR_DEV_CLS_PHONE:
		icon = (connected == 0) ? BT_ICON_PHONE :
					BT_ICON_CONNECTED_PHONE;
		break;
	case BT_MAJOR_DEV_CLS_AUDIO:
		BT_DBG("minor_class: %x", minor_class);

		if (minor_class == BTAPP_MIN_DEV_CLS_HEADPHONES) {
			icon = (connected == 0) ? BT_ICON_HEADPHONE :
						BT_ICON_CONNECTED_HEADPHONE;
		} else {
			icon = (connected == 0) ? BT_ICON_HEADSET :
						BT_ICON_CONNECTED_HEADSET;
		}
		break;
	case BT_MAJOR_DEV_CLS_LAN_ACCESS_POINT:
		icon = (connected == 0) ? BT_ICON_UNKNOWN :
					BT_ICON_CONNECTED_UNKNOWN;
		break;
	case BT_MAJOR_DEV_CLS_IMAGING:
		icon = (connected == 0) ? BT_ICON_PRINTER :
					BT_ICON_CONNECTED_PRINTER;
		break;
	case BT_MAJOR_DEV_CLS_PERIPHERAL:
		if (minor_class == BTAPP_MIN_DEV_CLS_KEY_BOARD) {
			icon = (connected == 0) ? BT_ICON_KEYBOARD :
					BT_ICON_CONNECTED_KEYBOARD;
		} else if (minor_class == BTAPP_MIN_DEV_CLS_POINTING_DEVICE) {
			icon = (connected == 0) ? BT_ICON_MOUSE :
					BT_ICON_CONNECTED_MOUSE;
		}
		break;
	case BT_MAJOR_DEV_CLS_HEALTH:
		icon = (connected == 0) ? BT_ICON_HEALTH :
					BT_ICON_CONNECTED_HEALTH;
		break;

		/* end */
	default:
		icon = (connected == 0) ? BT_ICON_UNKNOWN :
					BT_ICON_CONNECTED_UNKNOWN;
		break;
	}

	FN_END;
	return icon;
}

void _bt_main_popup_del_cb(void *data, Evas_Object *obj,
				    void *event_info)
{
	FN_START;
	retm_if(data == NULL, "Invalid argument: struct bt_appdata is NULL\n");

	bt_ug_data *ugd = (bt_ug_data *)data;

	if (ugd->popup) {
		BT_DBG("delete popup\n");
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	ugd->back_cb = NULL;

	FN_END;
}

void _bt_main_selectioninfo_hide_cb(void *data, Evas * e,
					Evas_Object *obj, void *event_info)
{
	FN_START;
	retm_if(data == NULL, "Invalid argument: struct bt_appdata is NULL\n");

	bt_ug_data *ugd = (bt_ug_data *)data;

	evas_object_del(ugd->selectioninfo);
	ugd->selectioninfo = NULL;

	FN_END;
}

Elm_Object_Item *_bt_main_add_paired_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git;

	retvm_if(ugd == NULL, NULL, "Invalid argument: ugd is NULL\n");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL\n");

	/* Paired device Title */
	if (ugd->paired_title == NULL) {
		if (ugd->searched_title == NULL) {
			git = elm_genlist_item_append(ugd->main_genlist,
						ugd->paired_title_itc,
						(void *)ugd, NULL,
						ELM_GENLIST_ITEM_NONE,
						NULL, NULL);

		} else {
			git = elm_genlist_item_insert_before(ugd->main_genlist,
						ugd->paired_title_itc,
						(void *)ugd, NULL,
						ugd->searched_title,
						ELM_GENLIST_ITEM_NONE,
						NULL, NULL);
		}

		elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		ugd->paired_title = git;

		git = elm_genlist_item_append(ugd->main_genlist,
					    ugd->end_itc,
					    (void *)ugd, NULL,
					    ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		ugd->paired_padding = git;

		__bt_main_set_controlbar_mode(ugd, BT_CONTROL_BAR_ENABLE);
	}

	dev->ugd = (void *)ugd;
	dev->is_bonded = TRUE;
	dev->status = BT_IDLE;

	/* Add the device item in the list */
	git = elm_genlist_item_insert_after(ugd->main_genlist, ugd->device_itc,
					  dev, NULL, ugd->paired_title,
					  ELM_GENLIST_ITEM_NONE,
					  __bt_main_paired_item_sel_cb, ugd);

	dev->genlist_item = git;

	FN_END;
	return git;
}

Elm_Object_Item *_bt_main_add_searched_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git;

	retvm_if(ugd == NULL, NULL, "Invalid argument: ugd is NULL\n");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL\n");

	if (ugd->searched_title == NULL)
		_bt_main_add_searched_title(ugd);

	/* Searched device Item */
	if (ugd->searched_device == NULL)
		git = elm_genlist_item_insert_after(ugd->main_genlist, ugd->searched_itc,
					dev, NULL, ugd->searched_title, ELM_GENLIST_ITEM_NONE,
					__bt_main_searched_item_sel_cb, ugd);
	else {
		bt_dev_t *item_dev = NULL;
		Elm_Object_Item *item = NULL;
		Elm_Object_Item *next = NULL;

		item = elm_genlist_item_next_get(ugd->searched_title);

		/* check the RSSI value of searched device list add arrange its order */
		while (item != NULL) {
			item_dev = _bt_main_get_dev_info(ugd->searched_device, item);
			retv_if(item_dev == NULL, NULL);

			if (item_dev->rssi > dev->rssi) {
				next = elm_genlist_item_next_get(item);
				if (next == NULL) {
					git = elm_genlist_item_insert_after(ugd->main_genlist,
							ugd->searched_itc,
							dev, NULL, item, ELM_GENLIST_ITEM_NONE,
							__bt_main_searched_item_sel_cb, ugd);
					break;
				}
				item = next;
			} else {
				git = elm_genlist_item_insert_before(ugd->main_genlist,
							ugd->searched_itc,
							dev, NULL, item, ELM_GENLIST_ITEM_NONE,
							__bt_main_searched_item_sel_cb, ugd);
				break;
			}
		}
	}

	dev->genlist_item = git;
	dev->status = BT_IDLE;
	dev->ugd = (void *)ugd;
	dev->is_bonded = FALSE;

	FN_END;
	return git;
}

Elm_Object_Item *_bt_main_add_no_device_found(bt_ug_data *ugd)
{
	FN_START;

	Elm_Object_Item *git = NULL;

	retvm_if(ugd == NULL, NULL, "Invalid argument: ugd is NULL\n");
	retvm_if(ugd->searched_title == NULL, NULL, "title is NULL\n");

	/* No device found Item */
	git = elm_genlist_item_insert_after(ugd->main_genlist, ugd->no_device_itc,
				NULL, NULL, ugd->searched_title, ELM_GENLIST_ITEM_NONE,
				NULL, ugd);

	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	FN_END;
	return git;
}

int _bt_main_draw_list_view(bt_ug_data *ugd)
{
	FN_START;

	int ret = 0;
	Evas_Object *navi = NULL;
	Evas_Object *bg = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *toolbar = NULL;
	Evas_Object *back_btn = NULL;
	Elm_Object_Item *item = NULL;
	Elm_Object_Item *navi_it;

	retv_if(ugd == NULL, BT_UG_FAIL);

	navi = _bt_create_naviframe(ugd->base);

	/* create back button */
	back_btn = elm_button_add(navi);

	bg = _bt_create_bg(navi, "group_list");

	layout = _bt_create_layout(navi, NULL, NULL);

	genlist = __bt_main_add_genlist_dialogue(layout, ugd);

	navi_it = elm_naviframe_item_push(navi, BT_STR_BLUETOOTH, back_btn, NULL, genlist,
			       NULL);

	/* create controlbar */
	toolbar = _bt_create_controlbar(navi, "default");

	/* Divide the controlbar by 2 items */
	/* Scan item */
	item = elm_toolbar_item_append(toolbar,
				BT_ICON_CONTROLBAR_SCAN, NULL,
				__bt_main_controlbar_btn_cb,
				ugd);

	ugd->scan_item = item;

	/* Add NULL item according to guideline of UX team */
	item = elm_toolbar_item_append(toolbar,
					NULL, NULL,
					NULL, NULL);
	elm_object_item_disabled_set(item, EINA_TRUE);

	elm_object_item_part_content_set(navi_it, "controlbar", toolbar);

	ugd->navi_it = navi_it;

	/* Style set should be called after elm_naviframe_item_push(). */
	elm_object_style_set(back_btn, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_btn, "clicked",
				       __bt_main_quit_btn_cb, (void *)ugd);

	ugd->navi_bar = navi;
	ugd->main_layout = layout;
	ugd->main_genlist = genlist;

	/* In the NFC case, we don't need to display the paired device */
	if (ugd->op_status == BT_ACTIVATED &&
	     ugd->bt_launch_mode != BT_LAUNCH_USE_NFC) {
		_bt_main_draw_paired_devices(ugd);

		if (ugd->paired_device == NULL ||
		     eina_list_count(ugd->paired_device) == 0) {
			if (_bt_util_is_battery_low() == FALSE) {
				ret = bt_adapter_start_device_discovery();
				if (!ret) {
					ugd->op_status = BT_SEARCHING;
					elm_toolbar_item_icon_set(ugd->scan_item,
								BT_ICON_CONTROLBAR_STOP);
				} else
					BT_DBG("Operation failed : Error Cause[%d]", ret);
			}
			elm_genlist_item_update(ugd->status_item);
		}
	} else if (ugd->op_status == BT_DEACTIVATED &&
	     ugd->bt_launch_mode != BT_LAUNCH_NORMAL) {
		 __bt_main_enable_bt(ugd);
	}

	if (ugd->op_status == BT_ACTIVATED || ugd->op_status == BT_SEARCHING)
		elm_object_item_disabled_set(ugd->visible_item, EINA_FALSE);


	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_main_draw_visibility_view(bt_ug_data *ugd)
{
	FN_START;

	Evas_Object *navi = NULL;
	Evas_Object *bg = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *toolbar = NULL;
	Evas_Object *back_btn = NULL;
	Elm_Object_Item *navi_it;

	retv_if(ugd == NULL, BT_UG_FAIL);

	navi = _bt_create_naviframe(ugd->base);

	/* create back button */
	back_btn = elm_button_add(navi);

	bg = _bt_create_bg(navi, "group_list");

	layout = _bt_create_layout(navi, NULL, NULL);

	genlist = __bt_main_add_visibility_dialogue(layout, ugd);

	navi_it = elm_naviframe_item_push(navi, BT_STR_VISIBLE, back_btn, NULL, genlist,
			       NULL);

	ugd->navi_it = navi_it;

	/* create controlbar */
	toolbar = _bt_create_controlbar(navi, "default");

	/* Style set should be called after elm_naviframe_item_push(). */
	elm_object_style_set(back_btn, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_btn, "clicked",
				       __bt_main_quit_btn_cb, (void *)ugd);

	ugd->navi_bar = navi;
	ugd->main_layout = layout;
	ugd->main_genlist = genlist;

	if (ugd->op_status == BT_ACTIVATED)
		elm_object_item_disabled_set(ugd->visible_item, EINA_FALSE);


	FN_END;
	return BT_UG_ERROR_NONE;
}

static bool __bt_cb_adapter_bonded_device(bt_device_info_s *device_info,
						void *user_data)
{
	bt_dev_t *dev = NULL;
	bool connected;
	bt_ug_data *ugd = NULL;

	retv_if(user_data == NULL, false);

	ugd = (bt_ug_data *)user_data;

	dev = _bt_main_create_paired_device_item(device_info);

	if (!dev)
		return false;

	dev->ugd = (void *)ugd;

	if (dev->service_list & BT_SC_HFP_SERVICE_MASK ||
	     dev->service_list & BT_SC_HSP_SERVICE_MASK ||
	      dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
		connected = _bt_is_profile_connected(BT_HEADSET_CONNECTED,
						ugd->conn, dev->bd_addr);
		dev->connected_mask |= connected ? BT_HEADSET_CONNECTED : 0x00;

		connected = _bt_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
						ugd->conn, dev->bd_addr);
		dev->connected_mask |= connected ? BT_STEREO_HEADSET_CONNECTED : 0x00;
	} else if (dev->service_list & BT_SC_HID_SERVICE_MASK) {
		connected = _bt_is_profile_connected(BT_HID_CONNECTED,
						ugd->conn, dev->bd_addr);
		dev->connected_mask |= connected ? BT_HID_CONNECTED : 0x00;
	}

	BT_DBG("connected mask: %d", dev->connected_mask);

	if (_bt_main_is_matched_profile(ugd->search_type,
				dev->major_class,
				dev->service_class,
				ugd->service) == TRUE) {
		BT_DBG("Device count [%d]",
		       eina_list_count(ugd->paired_device));
		ugd->paired_device =
		    eina_list_append(ugd->paired_device, dev);
	}

	return true;
}

void _bt_main_draw_paired_devices(bt_ug_data *ugd)
{
	FN_START;
	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	ret_if(ugd == NULL);

	if (bt_adapter_foreach_bonded_device(__bt_cb_adapter_bonded_device,
						(void *)ugd) != BT_ERROR_NONE) {
		BT_DBG("bt_adapter_foreach_bonded_device() failed");
		return;
	}

	EINA_LIST_FOREACH(ugd->paired_device, l, item) {
		if (item)
			_bt_main_add_paired_device(ugd, item);
	}

	FN_END;
	return;
}

void _bt_main_remove_paired_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	retm_if(ugd == NULL, "Invalid argument: ugd is NULL\n");
	retm_if(dev == NULL, "Invalid argument: dev is NULL\n");

	EINA_LIST_FOREACH_SAFE(ugd->paired_device, l, l_next, item) {
		if (item && (item == dev)) {
			elm_object_item_del(dev->genlist_item);
			ugd->paired_device =
			    eina_list_remove_list(ugd->paired_device, l);
			free(item);
		}
	}

	if (ugd->paired_device == NULL ||
	     eina_list_count(ugd->paired_device) == 0) {
		elm_object_item_del(ugd->paired_title);
		ugd->paired_title = NULL;

		if (ugd->paired_padding) {
			elm_object_item_del(ugd->paired_padding);
			ugd->paired_padding = NULL;
		}
	}

	FN_END;
	return;
}

void _bt_main_remove_all_paired_devices(bt_ug_data *ugd)
{
	FN_START;

	bt_dev_t *dev;
	Eina_List *l;
	Eina_List *l_next;
	Elm_Object_Item *item;
	Elm_Object_Item *next;

	ret_if(ugd == NULL);

	if (ugd->paired_title) {
		item = elm_genlist_item_next_get(ugd->paired_title);

		elm_object_item_del(ugd->paired_title);
		ugd->paired_title = NULL;

		while (item != NULL && (item != ugd->searched_title)) {
			next = elm_genlist_item_next_get(item);
			elm_object_item_del(item);
			item = next;
		}

		ugd->paired_padding = NULL;
	}

	EINA_LIST_FOREACH_SAFE(ugd->paired_device, l, l_next, dev) {
		ugd->paired_device =
		    eina_list_remove_list(ugd->paired_device, l);
		free(dev);
	}

	FN_END;
	return;
}

void _bt_main_remove_searched_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	ret_if(ugd == NULL);

	EINA_LIST_FOREACH_SAFE(ugd->searched_device, l, l_next, item) {
		if (item && (item == dev)) {
			elm_object_item_del(dev->genlist_item);
			ugd->searched_device =
			    eina_list_remove_list(ugd->searched_device, l);
			free(item);
		}
	}

	if (ugd->searched_device == NULL ||
	     eina_list_count(ugd->searched_device) == 0) {
		elm_object_item_del(ugd->searched_title);
		ugd->searched_title = NULL;

		if (ugd->searched_padding) {
			elm_object_item_del(ugd->searched_padding);
			ugd->searched_padding = NULL;
		}
	}

	FN_END;
	return;
}

void _bt_main_remove_all_searched_devices(bt_ug_data *ugd)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	Elm_Object_Item *item;
	Elm_Object_Item *next;

	ret_if(ugd == NULL);

	if (ugd->searched_title) {
		item = elm_genlist_item_next_get(ugd->searched_title);

		elm_object_item_del(ugd->searched_title);
		ugd->searched_title = NULL;

		while (item != NULL) {
			next = elm_genlist_item_next_get(item);
			elm_object_item_del(item);
			item = next;
		}

		ugd->searched_padding = NULL;
		ugd->no_device_item = NULL;
	}

	EINA_LIST_FOREACH_SAFE(ugd->searched_device, l, l_next, dev) {
		ugd->searched_device =
		    eina_list_remove_list(ugd->searched_device, l);
		free(dev);
	}

	FN_END;
	return;
}

gboolean _bt_main_is_headset_connected(bt_ug_data *ugd)
{
	gboolean connected = FALSE;
	Eina_List *l = NULL;
	bt_dev_t *item = NULL;

	retv_if(ugd == NULL, FALSE);

	EINA_LIST_FOREACH(ugd->paired_device, l, item) {
		if (item) {
			if ((item->service_list & BT_SC_HFP_SERVICE_MASK) ||
			     (item->service_list & BT_SC_HSP_SERVICE_MASK)) {
				connected = _bt_is_profile_connected(BT_HEADSET_CONNECTED,
							ugd->conn, item->bd_addr);

				if (connected)
					return TRUE;
			}
		}
	}

	return connected;
}

gboolean _bt_main_is_stereo_headset_connected(bt_ug_data *ugd)
{
	gboolean connected = FALSE;
	Eina_List *l = NULL;
	bt_dev_t *item = NULL;

	retv_if(ugd == NULL, FALSE);

	EINA_LIST_FOREACH(ugd->paired_device, l, item) {
		if (!item)
			continue;

		if (item->service_list & BT_SC_A2DP_SERVICE_MASK) {
			connected = _bt_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
							ugd->conn, item->bd_addr);

			if (connected)
				return TRUE;
		}

	}

	return connected;
}

void _bt_main_connect_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	int headset_type = BT_AUDIO_PROFILE_TYPE_ALL;

	ret_if(ugd == NULL);
	ret_if(dev == NULL);

	if (ugd->connect_req == TRUE) {
		_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_FAILED);
		return;
	}

	if ((dev->service_list & BT_SC_HFP_SERVICE_MASK) ||
	     (dev->service_list & BT_SC_HSP_SERVICE_MASK)) {
		/* Connect the  Headset */

		if (_bt_main_is_headset_connected(ugd) == TRUE) {
			/* Check if A2DP is connected or not */
			if ((dev->service_list & BT_SC_A2DP_SERVICE_MASK) &&
				_bt_main_is_stereo_headset_connected(ugd) == TRUE) {
				_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_EXISTS);
				return;
			}

			headset_type = BT_AUDIO_PROFILE_TYPE_A2DP;
		} else {
			headset_type = BT_AUDIO_PROFILE_TYPE_ALL;
		}

		if (bt_audio_connect(dev->addr_str,
					headset_type) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_CONNECTING;
		} else {
			BT_DBG("Fail to connect Headset device");
		}
	} else if (dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
		/* Connect the  Stereo Headset */
		if (_bt_main_is_stereo_headset_connected(ugd) == TRUE) {
			_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_EXISTS);
			return;
		}

		if (bt_audio_connect(dev->addr_str,
				BT_AUDIO_PROFILE_TYPE_A2DP) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_CONNECTING;
		} else {
			BT_DBG("Fail to connect Headset device");
		}
	} else if (dev->service_list & BT_SC_HID_SERVICE_MASK) {
		BT_DBG("HID connect request\n");

		if (bt_hid_host_connect(dev->addr_str) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_CONNECTING;
		} else {
			BT_DBG("Fail to connect HID device");
		}
	}

	elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);

	FN_END;
}

void _bt_main_disconnect_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	ret_if(ugd == NULL);
	ret_if(dev == NULL);

	if (ugd->connect_req == TRUE) {
		_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_FAILED);
		return;
	}

	if (_bt_is_profile_connected(BT_HEADSET_CONNECTED, ugd->conn,
						dev->bd_addr) == TRUE) {
		BT_DBG("Disconnecting AG service \n");
		if (bt_audio_disconnect(dev->addr_str,
				BT_AUDIO_PROFILE_TYPE_ALL) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_DBG("Fail to connect Headset device");
		}
	} else if (_bt_is_profile_connected(BT_STEREO_HEADSET_CONNECTED, ugd->conn,
						dev->bd_addr) == TRUE) {
		BT_DBG("Disconnecting AV service \n");
		if (bt_audio_disconnect(dev->addr_str,
				BT_AUDIO_PROFILE_TYPE_A2DP) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_DBG("Fail to connect Headset device");
		}
	} else if (_bt_is_profile_connected(BT_HID_CONNECTED, ugd->conn,
						dev->bd_addr) == TRUE) {
		BT_DBG("Disconnecting HID service!!\n");

		if (bt_hid_host_disconnect(dev->addr_str) == BT_ERROR_NONE) {
			dev->status = BT_DISCONNECTING;
		} else {
			BT_DBG("Fail to disconnect HID device");
		}
		return;
	}

	elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);

	FN_END;
}

void _bt_main_change_rotate_mode(void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	BT_DBG("rotation: %d", ugd->rotation);

	FN_END;
}

void _bt_main_change_connection_status(bool connected, bt_ug_data *ugd,
				       bt_dev_t *dev)
{
	FN_START;

	retm_if(ugd == NULL, "Invalid argument: ugd is NULL\n");
	retm_if(dev == NULL, "Invalid argument: dev is NULL\n");
	retm_if(dev->genlist_item == NULL,
		"Invalid argument: genlist_item is NULL\n");

	BT_DBG("Connected: %d", connected);

	dev->status = BT_IDLE;
	dev->connected_mask = connected ? dev->connected_mask : 0x00;

	elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);

	FN_END;
	return;
}

void _bt_main_draw_popup_menu(Evas_Object *parent, bt_dev_t *dev,
			      bt_ug_data *ugd)
{
	FN_START;

	Elm_Object_Item *item = NULL;
	Evas_Object *menu_list = NULL;
	Evas_Object *popup_menu = NULL;
	Evas_Object *btn = NULL;
	char *name = NULL;

	retm_if(parent == NULL, "Invalid argument: parent is NULL\n");
	retm_if(dev == NULL, "Invalid argument: ugd is NULL\n");
	retm_if(ugd == NULL, "Invalid argument: ugd is NULL\n");
	retm_if(ugd->popup_menu != NULL, "Menu popup is displaying\n");

	BT_DBG("dev name: %s", dev->name);

	name = __bt_main_get_name(dev);

	/* create normal popup */
	popup_menu =
	    _bt_create_popup(parent, name, NULL, __bt_main_popup_menu_cb, ugd,
			     0);

	elm_object_style_set(popup_menu, "menustyle");
	btn = elm_button_add(ugd->popup);
	elm_object_text_set(btn, BT_STR_BLUETOOTH_CLOSE);
	elm_object_part_content_set(ugd->popup, "button1", btn);
	evas_object_smart_callback_add(btn, "clicked", __bt_main_popup_menu_cb, ugd);

	ugd->popup_menu = popup_menu;

	if (name)
		free(name);

	/* create the popup menu using genlist */
	menu_list = _bt_create_genlist(popup_menu);

	evas_object_data_set(menu_list, "dev_info", dev);

	/* create inner list for menu */
	ugd->popup_menu_itc.item_style = "1text";
	ugd->popup_menu_itc.func.text_get = __bt_main_popup_menu_label_get;
	ugd->popup_menu_itc.func.content_get = NULL;
	ugd->popup_menu_itc.func.state_get = NULL;
	ugd->popup_menu_itc.func.del = NULL;

	BT_DBG("service list: %x", dev->service_list);
	BT_DBG("major clase: %x", dev->major_class);

	item = elm_genlist_item_append(menu_list, &ugd->popup_menu_itc,
				    (void *)BT_CONNECT_MENU, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    __bt_main_select_menu_cb, ugd);

	/* set inner list to popup */
	elm_object_content_set(popup_menu, menu_list);
	ugd->back_cb = (bt_app_back_cb) __bt_main_popup_menu_cb;

	FN_END;
}

int _bt_main_request_pairing_with_effect(bt_ug_data *ugd,
					 Elm_Object_Item *seleted_item)
{
	FN_START;

	bt_dev_t *dev;

	retvm_if(ugd == NULL, BT_UG_FAIL, "Invalid argument: ugd is NULL\n");
	retvm_if(seleted_item == NULL, BT_UG_FAIL,
		 "Invalid argument: object is NULL\n");

	dev = _bt_main_get_dev_info(ugd->searched_device, seleted_item);
	retvm_if(dev == NULL, BT_UG_FAIL, "Invalid argument: dev is NULL\n");

	if (bt_device_create_bond(dev->addr_str) == BT_ERROR_NONE) {
		dev->status = BT_DEV_PAIRING;
		ugd->op_status = BT_PAIRING;

		elm_genlist_item_update(seleted_item);
		elm_object_item_disabled_set(ugd->scan_item, EINA_TRUE);
	} else {
		return BT_UG_FAIL;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

void _bt_main_retry_pairing(void *data, int response)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;
	dev = _bt_main_get_dev_info(ugd->searched_device, ugd->searched_item);
	retm_if(dev == NULL, "dev is NULL\n");

	if (response == 0) {
		/* Retry pairing with same device */
		dev->status = BT_DEV_PAIRING;
		elm_genlist_item_update(ugd->searched_item);

		if (ugd->op_status != BT_PAIRING) {
			if (_bt_main_request_pairing_with_effect
			    (ugd, ugd->searched_item) != BT_UG_ERROR_NONE)
				ugd->searched_item = NULL;
		} else
			ugd->searched_item = NULL;
	} else {
		dev->status = BT_IDLE;
		elm_genlist_item_update(ugd->searched_item);
	}

	FN_END;
}

void __bt_main_parse_service(bt_ug_data *ugd, service_h service)
{
	char *launch_type = NULL;
	char *operation = NULL;
	const char *file_url = NULL;
	const char *file_path = NULL;

	ret_if(ugd == NULL);
	ret_if(service == NULL);

	if (service_get_operation(service, &operation) < 0)
		BT_DBG("Get operation error");

	BT_DBG("operation: %s", operation);

	if (g_strcmp0(operation, SERVICE_OPERATION_SEND) == 0) {
		launch_type = strdup("send");

		if (service_get_uri(service, (char **)&file_url) < 0)
			BT_DBG("Get uri error");

		if (file_url == NULL)
			goto done;

		file_path = g_filename_from_uri(file_url, NULL, NULL);

		if (file_path == NULL) {
			BT_DBG("Not include URI info");
			file_path = strdup(file_url);
		}

		/* In now, we support only 1 file by AppControl */
		if (service_add_extra_data(service, "filecount", "1") < 0)
			BT_DBG("Fail to add extra data");

		if (service_add_extra_data(service, "files", file_path) < 0)
			BT_DBG("Fail to add extra data");
	} else if (service_get_extra_data(service, "launch-type",
						&launch_type) < 0) {
		BT_DBG("Get data error");
	}

done:
	if (launch_type) {
		BT_DBG("Launch with launch type [%s]\n", launch_type);
		_bt_util_set_value(launch_type, &ugd->search_type,
				   &ugd->bt_launch_mode);
	} else {
		BT_DBG("launch type is NULL");
	}

	if (file_url)
		free((void *)file_url);

	if (file_path)
		free((void *)file_path);

	if (launch_type)
		free((void *)launch_type);

}

void _bt_main_init_status(bt_ug_data *ugd, void *data)
{
	FN_START;

	service_h service = NULL;
	bool status = false;
	bt_adapter_state_e bt_state = BT_ADAPTER_DISABLED;
	bt_adapter_visibility_mode_e mode =
				BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;

	ret_if(ugd == NULL);

	service = data;

	if (service != NULL) {
		__bt_main_parse_service(ugd, service);
	} else {
		ugd->search_type = MISCELLANEOUS_MAJOR_DEVICE_MASK;
		ugd->bt_launch_mode = BT_LAUNCH_NORMAL;
	}

	ugd->conn = (void *)dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);

	if (bt_initialize() != BT_ERROR_NONE)
		BT_DBG("bt_initialize() failed");

	if (bt_audio_initialize() != BT_ERROR_NONE)
		BT_DBG("bt_initialize() failed");

	if (bt_adapter_get_state(&bt_state) != BT_ERROR_NONE)
		BT_DBG("bt_adapter_get_state() failed.");

	if (bt_state == BT_ADAPTER_DISABLED) {
		ugd->op_status = BT_DEACTIVATED;
	} else {
		if (bt_adapter_is_discovering(&status) != BT_ERROR_NONE)
			BT_DBG("bt_adapter_get_state() failed.");

		if (status == true)
			bt_adapter_stop_device_discovery();

		ugd->op_status = BT_ACTIVATED;

		if(bt_adapter_get_visibility(&mode) != BT_ERROR_NONE)
			BT_DBG("bt_adapter_get_visibility() failed.");

		if (mode == BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE) {
			ugd->visible = FALSE;
			ugd->visibility_timeout = 0;
		} else if (mode == BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE){
			ugd->visible = TRUE;
			ugd->visibility_timeout = -1;
		} else {
			/* BT_ADAPTER_VISIBILITY_MODE_LIMITED_DISCOVERABLE */
			/* Need to add the code for getting timeout */
			if (vconf_get_int(BT_VCONF_VISIBLE_TIME,
					&ugd->visibility_timeout)) {
				BT_DBG("Get the timeout value");
			}

			ugd->remain_time = _bt_get_remain_timeout(ugd->conn);

			if (ugd->remain_time > 0) {
				ugd->timeout_id = g_timeout_add_seconds(1,
						__bt_main_visible_timeout_cb, ugd);
			} else {
				ugd->visibility_timeout = 0;
			}
		}
	}

	/* Set event callbacks */
	bt_adapter_set_state_changed_cb(_bt_cb_state_changed, (void *)ugd);

	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY) {
		/* Don't need to register callback */
		return;
	}

	bt_audio_set_connection_state_changed_cb(
				_bt_cb_audio_state_changed,
				(void *)ugd);

	bt_adapter_set_device_discovery_state_changed_cb(
					_bt_cb_discovery_state_changed,
					(void *)ugd);

	bt_device_set_bond_created_cb(_bt_cb_bonding_created, (void *)ugd);

	bt_device_set_bond_destroyed_cb(_bt_cb_bonding_destroyed, (void *)ugd);

	bt_device_set_service_searched_cb(_bt_cb_service_searched, (void *)ugd);

	bt_hid_host_initialize(_bt_cb_hid_state_changed, (void *)ugd);

	FN_END;
}

bt_dev_t *_bt_main_create_paired_device_item(void *data)
{
	FN_START;

	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
	bt_dev_t *dev = NULL;
	bt_device_info_s *dev_info = NULL;

	retv_if(data == NULL, NULL);

	dev_info = (bt_device_info_s *)data;

	if (strlen(dev_info->remote_name) == 0)
		return NULL;

	dev = malloc(sizeof(bt_dev_t));
	retv_if(dev == NULL, NULL);

	memset(dev, 0, sizeof(bt_dev_t));
	strncpy(dev->name, dev_info->remote_name,
		BT_DEVICE_NAME_LENGTH_MAX);

	dev->major_class = dev_info->bt_class.major_device_class;
	dev->minor_class = dev_info->bt_class.minor_device_class;
	dev->service_class = dev_info->bt_class.major_service_class_mask;

	_bt_util_addr_string_to_addr_type(bd_addr, dev_info->remote_address);

	memcpy(dev->addr_str, dev_info->remote_address,
	       BT_ADDRESS_STR_LEN);

	memcpy(dev->bd_addr, bd_addr, BT_ADDRESS_LENGTH_MAX);

	_bt_util_get_service_mask_from_uuid_list(dev_info->service_uuid,
						 dev_info->service_count,
						 &dev->service_list);

	BT_DBG("device name [%s]", dev->name);
	BT_DBG("device major class [%x]", dev->major_class);
	BT_DBG("device minor class [%x]", dev->minor_class);
	BT_DBG("device service class [%x]", dev->service_class);
	BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
	       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
	       dev->bd_addr[4], dev->bd_addr[5]);

	FN_END;
	return dev;
}

bt_dev_t *_bt_main_create_searched_device_item(void *data)
{
	FN_START;

	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
	bt_dev_t *dev = NULL;
	bt_adapter_device_discovery_info_s *dev_info = NULL;

	retv_if(data == NULL, NULL);

	dev_info = (bt_adapter_device_discovery_info_s *)data;

	if (strlen(dev_info->remote_name) == 0)
		return NULL;

	dev = calloc(1, sizeof(bt_dev_t));
	retv_if(dev == NULL, NULL);

	strncpy(dev->name, dev_info->remote_name,
		BT_DEVICE_NAME_LENGTH_MAX);

	dev->major_class = dev_info->bt_class.major_device_class;
	dev->minor_class = dev_info->bt_class.minor_device_class;
	dev->service_class = dev_info->bt_class.major_service_class_mask;
	dev->rssi = dev_info->rssi;

	_bt_util_addr_string_to_addr_type(bd_addr, dev_info->remote_address);

	memcpy(dev->addr_str, dev_info->remote_address,
	       BT_ADDRESS_STR_LEN);

	memcpy(dev->bd_addr, bd_addr, BT_ADDRESS_LENGTH_MAX);

	BT_DBG("device name [%s]", dev->name);
	BT_DBG("device major class [%x]", dev->major_class);
	BT_DBG("device minor class [%x]", dev->minor_class);
	BT_DBG("device service class [%x]", dev->service_class);
	BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
	       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
	       dev->bd_addr[4], dev->bd_addr[5]);

	FN_END;
	return dev;
}

gboolean _bt_main_is_matched_profile(unsigned int search_type,
				 unsigned int major_class,
				 unsigned int service_class,
				 service_h service)
{
	FN_START;

	bt_device_major_mask_t major_mask = BT_DEVICE_MAJOR_MASK_MISC;

	if (search_type == 0x000000)
		return TRUE;

	BT_DBG("search_type: %x", search_type);
	BT_DBG("service_class: %x", service_class);

	/* Check the service_class */
	if (service_class & search_type) {
		if (search_type & OBJECT_TRANSFER_MAJOR_SERVICE_MASK &&
		     major_class == BT_MAJOR_DEV_CLS_IMAGING) {
			if (__bt_main_is_image_file(service))
				return TRUE;
		} else {
			return TRUE;
		}
	}

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

	if (search_type & major_mask)
		return TRUE;

	FN_END;
	return FALSE;
}

bt_dev_t *_bt_main_get_dev_info(Eina_List *list,
				Elm_Object_Item *genlist_item)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retvm_if(list == NULL, NULL, "Invalid argument: list is NULL\n");
	retvm_if(genlist_item == NULL, NULL, "Invalid argument: obj is NULL\n");

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (item->genlist_item == genlist_item)
				return item;
		}
	}

	FN_END;
	return NULL;
}

bt_dev_t *_bt_main_get_dev_info_by_address(Eina_List *list,
						char *address)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retvm_if(list == NULL, NULL, "Invalid argument: list is NULL\n");
	retvm_if(address == NULL, NULL, "Invalid argument: addr is NULL\n");

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (memcmp(item->addr_str, address, BT_ADDRESS_STR_LEN) == 0)
				return item;
		}
	}

	FN_END;
	return NULL;
}

int _bt_main_check_and_update_device(Eina_List *list, char *addr,
				     char *name)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retv_if(list == NULL, -1);
	retv_if(addr == NULL, -1);
	retv_if(name == NULL, -1);

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (memcmp(item->addr_str, addr, BT_ADDRESS_STR_LEN) == 0) {
				memset(item->name, 0x00,
				       BT_DEVICE_NAME_LENGTH_MAX);
				memcpy(item->name, name,
				       BT_DEVICE_NAME_LENGTH_MAX);
				return 0;
			}
		}
	}

	FN_END;

	return -1;
}

void _bt_main_launch_syspopup(void *data, char *event_type, char *title,
			      char *type)
{
	FN_START;

	int ret = 0;
	bt_ug_data *ugd = NULL;
	bundle *b = NULL;

	ret_if(event_type == NULL);
	ret_if(type == NULL);

	ugd = (bt_ug_data *)data;

	_bt_ipc_register_popup_event_signal(ugd->EDBusHandle, data);

	b = bundle_create();
	ret_if(b == NULL);

	bundle_add(b, "event-type", event_type);
	bundle_add(b, "title", title);
	bundle_add(b, "type", type);

	ret = syspopup_launch("bt-syspopup", b);
	if (0 > ret) {
		BT_DBG("Popup launch failed...retry %d \n", ret);
		ugd->popup_bundle = b;
		ugd->popup_timer = g_timeout_add(BT_UG_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS,
			      (GSourceFunc) __bt_main_system_popup_timer_cb, ugd);
	} else {
		bundle_free(b);
	}
	FN_END;
}

void _bt_main_create_information_popup(bt_ug_data *ugd, char *msg) {
	FN_START;
	ret_if(ugd == NULL);

	if (ugd->popup) {
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	ugd->popup = _bt_create_popup(ugd->win_main,
				BT_STR_INFORMATION,
				msg,
				_bt_main_popup_del_cb,
				ugd, 2);
	FN_END;
}

void _bt_main_add_searched_title(bt_ug_data *ugd)
{
	Elm_Object_Item *git;

	if (ugd->paired_padding) {
		elm_object_item_del(ugd->paired_padding);
		ugd->paired_padding = NULL;
	}

	git = elm_genlist_item_append(ugd->main_genlist,
			ugd->searched_title_itc,
			(void *)ugd, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	ugd->searched_title = git;

	git = elm_genlist_item_append(ugd->main_genlist,
				    ugd->end_itc,
				    (void *)ugd, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	ugd->searched_padding = git;
}


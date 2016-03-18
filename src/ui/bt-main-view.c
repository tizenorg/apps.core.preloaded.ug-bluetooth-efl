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

#include <Ecore.h>
#include <errno.h>
#include <eina_list.h>
#include <aul.h>
#include <bluetooth.h>
#include <syspopup_caller.h>
#include <dbus/dbus.h>
#include <vconf.h>
#include <app_control.h>
#include <notification.h>
#include <efl_extension.h>
#include <bundle.h>

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
#include "bt-net-connection.h"
#include "bluetooth_internal.h"
#include "syspopup_caller.h"

#define MULTI_SHARE_SERVICE_DATA_PATH "http://tizen.org/appcontrol/data/path"
#define APP_CONTROL_OPERATION_SHARE_CONTACT "http://tizen.org/appcontrol/operation/share_contact"
#define SERVICE_SHARE_CONTACT_MODE "http://tizen.org/appcontrol/data/social/namecard_share_mode"
#define SERVICE_SHARE_CONTACT_ITEM "http://tizen.org/appcontrol/data/social/item_type"
#define SHARE_CONTACT_DATA_PATH "/opt/usr/media/Downloads/.bluetooth"
#define SHARE_CONTACT_ITEM_ID_ARRAY "http://tizen.org/appcontrol/data/social/item_id"
#define SHARE_CONTACT_ITEM_SHARE_MODE "http://tizen.org/appcontrol/data/social/namecard_share_mode"
#define HELP_SETUP_BLUETOOTH_URI		"tizen-help://ug-bluetooth-efl/setupbluetooth"

/**********************************************************************
*                                      Static Functions declaration
***********************************************************************/

static void __bt_main_onoff_btn_cb(void *data, Evas_Object *obj, void *event_info);

static app_control_h __bt_main_get_bt_onoff_result(bt_ug_data *ugd,
						gboolean result);

static app_control_h __bt_main_get_visibility_result(bt_ug_data *ugd,
						 gboolean result);

static app_control_h __bt_main_get_pick_result(bt_ug_data *ugd, gboolean result);

static int __bt_main_request_to_send(bt_ug_data *ugd, bt_dev_t *dev);

#ifdef KIRAN_ACCESSIBILITY
static char *__bt_main_get_device_string(int major_class, int minor_class);
#endif
static Eina_Bool __bt_cb_register_net_state_cb(void *data);

/**********************************************************************
*                                               Static Functions
***********************************************************************/

static char *__bt_main_onoff_label_get(void *data, Evas_Object *obj,
                                        const char *part)
{
	FN_START;
	bt_ug_data *ugd = NULL;

	retv_if(data == NULL, NULL);
	ugd = (bt_ug_data *)data;

	if (!strcmp("elm.text", part)) {
		return g_strdup(BT_STR_BLUETOOTH);
	} else if (!strcmp("elm.text.multiline", part)) {
		char buf[1024] = {0,};
		if (ugd->op_status == BT_ACTIVATING) {
			snprintf(buf, sizeof(buf), "<font_size=30>%s</font_size>", BT_STR_TURNING_ON_BLUETOOTH);
			return g_strdup(buf);
		} else if (ugd->op_status == BT_DEACTIVATED) {
			snprintf(buf, sizeof(buf), "<font_size=30>%s</font_size>", BT_STR_TURN_ON_BLUETOOTH_TO_SEE_A_LIST_OF_AVAILABLE_DEVICES);
			return g_strdup(buf);
		}
	}

	FN_END;
	return NULL;
}

static Evas_Object *__bt_main_onoff_icon_get(void *data, Evas_Object *obj,
						const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	Evas_Object *btn = NULL;
	bool activated = false;

	retv_if(data == NULL, NULL);

	ugd = (bt_ug_data *)data;

	if (!strcmp("elm.swallow.end", part)) {
		if (ugd->op_status == BT_ACTIVATING
			|| ugd->op_status == BT_DEACTIVATING) {
			btn = elm_progressbar_add(obj);
			elm_object_style_set(btn, "process_medium");
			evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, 0.5);
			evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_progressbar_pulse(btn, TRUE);
		} else {
			activated = ((ugd->op_status == BT_DEACTIVATED) ||
				(ugd->op_status == BT_ACTIVATING)) ? false : true;
			btn = elm_check_add(obj);
			elm_object_style_set(btn, "on&off");
			evas_object_pass_events_set(btn, EINA_TRUE);
			evas_object_propagate_events_set(btn, EINA_FALSE);
			elm_check_state_set(btn, activated);

#ifdef KIRAN_ACCESSIBILITY
			elm_access_object_unregister(btn);
#endif

			/* add smart callback */
			evas_object_smart_callback_add(btn, "changed",
				__bt_main_onoff_btn_cb, ugd);

			ugd->onoff_btn = btn;
		}
		evas_object_show(btn);
	}

	FN_END;
	return btn;
}

static char *__bt_main_rename_desc_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	if (!strcmp("elm.text.multiline", part)) {
		char buf[1024] = {0,};
		snprintf(buf, sizeof(buf),"<font_size=30>%s</font_size>",
				BT_STR_RENAME_DEVICE_LABEL);
		return g_strdup(buf);
	}

	FN_END;
	return NULL;
}

static char *__bt_main_device_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
	char *dev_name = NULL;
	char *ptr = NULL;
#ifdef KIRAN_ACCESSIBILITY
	char str[BT_STR_ACCES_INFO_MAX_LEN] = { 0, };
	Evas_Object *ao = NULL;
#endif
	retv_if(data == NULL, NULL);

	ugd = (bt_ug_data *)data;
	BT_DBG("part : %s", part);

	if (!strcmp("elm.text", part)) {
		memset(ugd->phone_name, 0x00, BT_GLOBALIZATION_STR_LENGTH);

		_bt_util_get_phone_name(ugd->phone_name, sizeof(ugd->phone_name));

		if (strlen(ugd->phone_name) == 0) {
			if (bt_adapter_get_name(&dev_name) == BT_ERROR_NONE) {
				g_strlcpy(ugd->phone_name, dev_name,
					  BT_GLOBALIZATION_STR_LENGTH);
				g_free(dev_name);
			}
		}

		BT_DBG("ugd->phone_name : %s[%d]", ugd->phone_name, strlen(ugd->phone_name));
		/* Check the utf8 valitation & Fill the NULL in the invalid location */
		if (!g_utf8_validate(ugd->phone_name, -1, (const char **)&ptr))
			*ptr = '\0';

		dev_name = elm_entry_utf8_to_markup(ugd->phone_name);
		if (dev_name) {
			g_strlcpy(buf, dev_name, BT_GLOBALIZATION_STR_LENGTH);
			g_free(dev_name);
		} else
			g_strlcpy(buf, ugd->phone_name, BT_GLOBALIZATION_STR_LENGTH);

#ifdef KIRAN_ACCESSIBILITY
		snprintf(str, sizeof(str), "%s, %s",
			 BT_STR_DEVICE_NAME, ugd->phone_name);
		ao = elm_object_item_access_object_get(ugd->device_name_item);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		FN_END;
		BT_DBG("buf : %s[%d]", buf, strlen(buf));
		return strdup(buf);
	} else if (!strcmp("elm.text.sub", part)) {
		g_strlcpy(buf, BT_STR_MY_DEVICE,
			  BT_GLOBALIZATION_STR_LENGTH);
#ifdef KIRAN_ACCESSIBILITY
		ao = elm_object_item_access_object_get(ugd->paired_title);
		snprintf(str, sizeof(str), "%s, %s", BT_STR_PAIRED_DEVICES,
			 BT_ACC_STR_GROUP_INDEX);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		FN_END;
		return strdup(buf);
	} else {
		BT_ERR("This part name is not exist in style.");
		return NULL;
	}
}

char* __bt_convert_rgba_to_hex(int r, int g, int b, int a)
{
	int hexcolor = 0;
	char* string = NULL;

	string = g_try_malloc0(sizeof(char )* 255);
	/* Fix : NULL_RETURNS */
	if (string == NULL)
		return NULL;

	hexcolor = (r << 24) + (g << 16) + (b << 8) + a;
	sprintf(string, "%08x", hexcolor );

	return string;
}

static char *__bt_main_visible_label_get(void *data, Evas_Object *obj,
					 const char *part)
{
	FN_START;

	char *buf = NULL;
	char *text1 = NULL;
	char *text2 = NULL;
	char remain_time[BT_EXTRA_STR_LEN] = { 0 };
	bt_ug_data *ugd = NULL;
#ifdef KIRAN_ACCESSIBILITY
	Evas_Object *ao = NULL;
	Elm_Object_Item *item = NULL;
	char *text_visibility = NULL;
	Eina_Bool expanded = EINA_FALSE;
	char acc_str[BT_STR_ACCES_INFO_MAX_LEN] = { 0 };
	char formatted_time[BT_BUFFER_LEN] = { 0 };
#endif

	int r = 0, g = 0, b = 0, a = 0;

	if (data == NULL)
		return NULL;

	ugd = (bt_ug_data *)data;

	BT_DBG("%s", part);

	if (!strcmp("elm.text", part)) {
		buf = g_strdup(BT_STR_VISIBLE);
	} else if (!strcmp("elm.text.multiline", part)) {
		char *color_code = NULL;
		if (ugd->visibility_timeout <= 0) {
			text1 = _bt_util_get_timeout_string(ugd->visibility_timeout);

			r = 20, g = 107, b = 147, a = 255;
			color_code = __bt_convert_rgba_to_hex(r, g, b, a);

			if (ugd->visibility_timeout == 0) {
				text2 = strdup(BT_STR_ONLY_VISIBLE_TO_PAIRED_DEVICES);
			} else {
				text2 = strdup(BT_STR_VISIBLE_TO_ALL_NEARBY);
			}

			buf = g_strdup_printf("<font_size=30><color=#%s>%s</color><br>%s</font_size>",
					color_code, text1, text2);

#ifdef KIRAN_ACCESSIBILITY
			text_visibility = g_strdup_printf("<color=#%s>%s</color>",
					color_code, text1);
#endif
		} else {
			time_t current_time;
			int time_diff;
			int minute;
			int second;

			/* Get the current time */
			time(&current_time);

			/* Calculate time elapsed from remain_time */
			time_diff = difftime(current_time, ugd->start_time);

			/* Display remain timeout */
#ifdef KIRAN_ACCESSIBILITY
			_bt_util_convert_time_to_string((ugd->remain_time -
							time_diff),
							remain_time, formatted_time,
							sizeof(remain_time), sizeof(formatted_time));
#else
			_bt_util_convert_time_to_string((ugd->remain_time -
							time_diff),
							remain_time, NULL,
							sizeof(remain_time), 0);
#endif
			/* Get seconds */
			second = (ugd->remain_time - time_diff) % 60;
			/* Get minutes */
			minute = (ugd->remain_time - time_diff) / 60;

			text1 = g_strdup_printf("%d:%02d", minute, second);

			r = 20, g = 107, b = 147, a = 255;
			color_code = __bt_convert_rgba_to_hex(r, g, b, a);

			text2 = strdup(BT_STR_VISIBLE_TO_ALL_NEARBY);

			buf = g_strdup_printf("<font_size=30><color=#%s>%s</color><br>%s</font_size>",
					color_code, text1, text2);

			BT_DBG("buf : %s, rgba:%d,%d,%d,%d", buf,r,g,b,a);

#ifdef KIRAN_ACCESSIBILITY
			text_visibility = g_strdup_printf(BT_STR_PS_REMAINING,
							formatted_time);
#endif
		}
		g_free(color_code);

#ifdef KIRAN_ACCESSIBILITY
		item = ugd->visible_item;
		if (item != NULL) {
			expanded = elm_genlist_item_expanded_get(item);

			ao = elm_object_item_access_object_get(item);
			if (expanded == EINA_TRUE) {
					snprintf(acc_str, sizeof(acc_str), "%s, %s, %s", BT_STR_VISIBLE,
							text_visibility, BT_STR_EXP_LIST_CLOSE);
			} else {
				if (elm_object_item_disabled_get(item))
					snprintf(acc_str, sizeof(acc_str), "%s, %s, %s, %s",
							BT_STR_VISIBLE, text_visibility,
							BT_STR_EXP_LIST_OPEN, BT_STR_UNAVAILABLE);
				else
					snprintf(acc_str, sizeof(acc_str), "%s, %s, %s", BT_STR_VISIBLE,
							text_visibility, BT_STR_EXP_LIST_OPEN);
			}

			elm_access_info_set(ao, ELM_ACCESS_INFO, acc_str);
		}
#endif
	} else {
		BT_ERR("empty text for label");
		return NULL;
	}

	g_free(text1);
	g_free(text2);
#ifdef KIRAN_ACCESSIBILITY
	g_free(text_visibility);
#endif
	FN_END;
	return buf;
}

static char *__bt_main_timeout_value_label_get(void *data, Evas_Object *obj,
					       const char *part)
{
	FN_START;

	char *buf = NULL;
	int timeout = 0;
	bt_radio_item *item = NULL;

	retv_if(data == NULL, NULL);

	item = (bt_radio_item *)data;
	retv_if(item->ugd == NULL, NULL);

	if (!strcmp("elm.text", part)) {
		timeout = _bt_util_get_timeout_value(item->index);
		buf = _bt_util_get_timeout_string(timeout);
	} else {
		BT_ERR("empty text for label");
		return NULL;
	}

	FN_END;
	return buf;
}

int _bt_idle_destroy_ug(void *data)
{
	FN_START;

	bt_ug_data *ugd = data;
	app_control_h service = NULL;

	retv_if(ugd == NULL, BT_UG_FAIL);

	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY)
		service = __bt_main_get_visibility_result(ugd, TRUE);
	else if (ugd->bt_launch_mode == BT_LAUNCH_PICK)
		service = __bt_main_get_pick_result(ugd, TRUE);
	else if (ugd->bt_launch_mode == BT_LAUNCH_ONOFF)
		service = __bt_main_get_bt_onoff_result(ugd, TRUE);

	_bt_ug_destroy(data, (void *)service);

	if (service)
		app_control_destroy(service);

	FN_END;
	return BT_UG_ERROR_NONE;
}

static gboolean __bt_main_visible_timeout_cb(gpointer user_data)
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
	BT_INFO("Time difference in seconds %d", time_diff);

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

static void __bt_update_visibility_menu(bt_radio_item *item)
{
	FN_START;
	bt_ug_data *ugd = NULL;
	int ret;
	int timeout;

	ret_if(item == NULL);

	ugd = (bt_ug_data *)item->ugd;
	ret_if(item->ugd == NULL);

	timeout = _bt_util_get_timeout_value(item->index);

	if (timeout < 0) {
		ret = bt_adapter_set_visibility(BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE, 0);

		if (ugd->timeout_id) {
			g_source_remove(ugd->timeout_id);
			ugd->timeout_id = 0;
		}
	} else if (timeout == 0) {
		ret = bt_adapter_set_visibility(BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE,
						0);
		if (ugd->timeout_id) {
			g_source_remove(ugd->timeout_id);
			ugd->timeout_id = 0;
		}
	} else {
		ret = bt_adapter_set_visibility(BT_ADAPTER_VISIBILITY_MODE_LIMITED_DISCOVERABLE,
						timeout);

		if (ret == BT_ERROR_NONE) {
			if (ugd->timeout_id) {
				g_source_remove(ugd->timeout_id);
				ugd->timeout_id = 0;
			}
			/* Set current time snapshot */
			time(&(ugd->start_time));
			ugd->remain_time = timeout;
			ugd->timeout_id = g_timeout_add(BT_VISIBILITY_TIMEOUT,
							(GSourceFunc)
							__bt_main_visible_timeout_cb,
							ugd);
		}
	}

	if (ret != BT_ERROR_NONE) {
		BT_ERR("bt_adapter_set_visibility() failed");
		return;
	}

	ugd->selected_radio = item->index;
	ugd->visibility_timeout = timeout;

	_bt_update_genlist_item(ugd->visible_item);
	elm_radio_value_set(ugd->radio_main, ugd->selected_radio);

	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY)
		g_idle_add((GSourceFunc) _bt_idle_destroy_ug, ugd);

	FN_END;
}

static void __bt_main_timeout_value_item_sel(void *data, Evas_Object *obj,
					     void *event_info)
{
	FN_START;

	bt_radio_item *item = NULL;

	ret_if(data == NULL);

	item = (bt_radio_item *)data;

	if(event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
				      EINA_FALSE);

	__bt_update_visibility_menu(item);

	bt_ug_data *ugd = (bt_ug_data *)item->ugd;
	if (ugd && ugd->visibility_popup) {
		evas_object_del(ugd->visibility_popup);
		ugd->visibility_popup = NULL;
	}

	FN_END;
	return;
}

static Evas_Object *__bt_main_timeout_value_icon_get(void *data,
						     Evas_Object *obj,
						     const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_radio_item *item = NULL;
	Evas_Object *btn = NULL;
	Evas_Object *ly_radio = NULL;
	retv_if(data == NULL, NULL);

	item = (bt_radio_item *)data;
	retv_if(item->ugd == NULL, NULL);

	ugd = (bt_ug_data *)item->ugd;

	if (!strcmp("elm.swallow.end", part)) {
		ly_radio = elm_layout_add(obj);
		elm_layout_theme_set(ly_radio, "layout", "list/C/type.2", "default");
		btn = elm_radio_add(ly_radio);
		elm_radio_state_value_set(btn, item->index);
		elm_radio_group_add(btn, ugd->radio_main);
		elm_radio_value_set(btn, ugd->selected_radio);
		elm_object_style_set(btn, "list");
#ifdef KIRAN_ACCESSIBILITY
		elm_access_object_unregister(btn);
#endif
		evas_object_show(btn);
		elm_layout_content_set(ly_radio, "elm.swallow.content", btn);
	}
	FN_END;
	return ly_radio;
}
static void __bt_main_timeout_value_del(void *data, Evas_Object *obj)
{
	FN_START;

	bt_radio_item *item = (bt_radio_item *)data;
	if (item)
		free(item);

	FN_END;
}

gboolean _bt_main_is_connectable_device(bt_dev_t *dev)
{
	FN_START;

	bt_device_info_s *device_info = NULL;

	retv_if(dev == NULL, FALSE);

	if (dev->service_list == 0) {
		if (bt_adapter_get_bonded_device_info
		    ((const char *)dev->addr_str,
		     &device_info) != BT_ERROR_NONE) {
			if (device_info)
				bt_adapter_free_device_info(device_info);
			return FALSE;
		}
		bt_device_get_service_mask_from_uuid_list
		    (device_info->service_uuid, device_info->service_count,
		     &dev->service_list);

		bt_adapter_free_device_info(device_info);

		if (dev->service_list == 0) {
			BT_ERR("No service list");
			return FALSE;
		}
	}

	if ((dev->service_list & BT_SC_HFP_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_HSP_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_A2DP_SERVICE_MASK) ||
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	    (dev->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK) ||
#endif
	    (dev->service_list & BT_SC_HID_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_NAP_SERVICE_MASK)) {
		/* Connectable device */
		return TRUE;
	}

	FN_END;
	return FALSE;
}

static char *__bt_main_paired_device_label_get(void *data, Evas_Object *obj,
				      const char *part)
{
	FN_START;

	char *buf = NULL;
#ifdef KIRAN_ACCESSIBILITY
	char str[BT_STR_ACCES_INFO_MAX_LEN] = { 0, };
#endif
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	int r = 0, g = 0, b = 0, a = 0;

	retv_if(data == NULL, NULL);

	dev = (bt_dev_t *)data;
	retv_if(dev == NULL, NULL);

	ugd = (bt_ug_data *)dev->ugd;
	retv_if(ugd == NULL, NULL);

	if (!strcmp("elm.text", part)) {
		char *name = elm_entry_utf8_to_markup(dev->name);

		if (ugd->bt_launch_mode != BT_LAUNCH_PICK &&
			dev->is_connected > 0 && dev->highlighted == FALSE) {
			r = 20, g = 107, b = 147, a = 255;
			char *color_code = __bt_convert_rgba_to_hex(r, g, b, a);;
			if (name) {
				buf = g_strdup_printf("<color=#%s>%s</color>",
					color_code,
					name);
				free(name);
			} else {
				buf = g_strdup_printf("<color=#%s>%s</color>",
					color_code,
					dev->name);
			}
			g_free(color_code);
			return buf;
		}

		if (name) {
			buf = g_strdup_printf("%s",
				name);
			free(name);
		} else {
			buf = g_strdup_printf("%s",
				dev->name);
		}
	} else if (!strcmp("elm.text.sub", part) && ugd->bt_launch_mode != BT_LAUNCH_PICK) {
#ifdef KIRAN_ACCESSIBILITY
		char *double_tap_string = NULL;
		char *device_type = NULL;
		Evas_Object *ao = NULL;
#endif

		if (dev->status == BT_IDLE) {
			if (_bt_main_is_connectable_device(dev)) {
				if (dev->is_connected == 0) {
					buf = g_strdup(BT_STR_PAIRED);
#ifdef KIRAN_ACCESSIBILITY
					double_tap_string = NULL;
#endif
				} else if (dev->is_connected > 0) {
					buf = g_strdup(BT_STR_CONNECTED);
#ifdef KIRAN_ACCESSIBILITY
					double_tap_string =
					    BT_STR_DOUBLE_TAP_DISCONNECT_D;
#endif
				}
			} else {
				buf = g_strdup(BT_STR_PAIRED);
#ifdef KIRAN_ACCESSIBILITY
				double_tap_string = BT_STR_DOUBLE_TAP_CONNECT_D;
#endif
			}
		} else if (dev->status == BT_CONNECTING) {
			buf = g_strdup(BT_STR_CONNECTING);
		} else if (dev->status == BT_SERVICE_SEARCHING) {
			buf = g_strdup(BT_STR_SEARCHING_SERVICES);
		} else if (dev->status == BT_DISCONNECTING) {
			buf = g_strdup(BT_STR_DISCONNECTING);
		}
#ifdef KIRAN_ACCESSIBILITY
		device_type =
				__bt_main_get_device_string(dev->major_class,
						dev->minor_class);
		if (double_tap_string != NULL && buf != NULL)
			snprintf(str, sizeof(str), "%s, %s, %s, %s, %s",
				 dev->name, buf,
				 device_type, double_tap_string,
				 BT_STR_MORE_BUTTON);
		else if (double_tap_string == NULL && buf != NULL)
			snprintf(str, sizeof(str), "%s, %s, %s, %s",
				 dev->name, buf,
				 device_type, BT_STR_MORE_BUTTON);

		g_free(device_type);
		ao = elm_object_item_access_object_get(dev->genlist_item);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
	} else {		/* for empty item */

		BT_ERR("empty text for label");
		return NULL;
	}
	FN_END;
	return buf;
}

static void __bt_paired_device_profile_cb(void *data, Evas_Object *obj,
				     void *event_info)
{
	FN_START;
	int ret;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	ugd = dev->ugd;

	if(event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
						  EINA_FALSE);

	if (ugd->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if (ret != BT_ERROR_NONE)
			BT_ERR("Fail to stop discovery: %d", ret);
	}

	/* Create the profile view */
	_bt_profile_create_view(dev);
	FN_END;
}

static void __bt_rename_device_entry_changed_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	ret_if(obj == NULL);
	ret_if(data == NULL);
	bt_ug_data *ugd = NULL;

	ugd = (bt_ug_data *)data;

	const char *entry_text = NULL;
	char *input_str = NULL;

	if (ugd->rename_entry != obj)
		ugd->rename_entry = obj;
	entry_text = elm_entry_entry_get(obj);
	input_str = elm_entry_markup_to_utf8(entry_text);

	if (input_str == NULL || strlen(input_str) == 0 ||
				_bt_util_is_space_str(input_str)) {
		BT_DBG("");
		elm_object_disabled_set(ugd->rename_button, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(
				obj, EINA_TRUE);
	} else {
		BT_DBG("");
		if (elm_object_disabled_get(ugd->rename_button))
			elm_object_disabled_set(ugd->rename_button, EINA_FALSE);
		if (elm_entry_input_panel_return_key_disabled_get(obj))
			elm_entry_input_panel_return_key_disabled_set(obj, EINA_FALSE);
	}
	if(input_str != NULL) {
		free(input_str);
		input_str = NULL;
	}

	FN_END;
}

static void __bt_rename_device_cancel_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);
	ugd = (bt_ug_data *)data;

	if (ugd->rename_popup != NULL) {
		evas_object_del(ugd->rename_popup);
		ugd->rename_popup = NULL;
		ugd->rename_entry_item = NULL;
		ugd->rename_entry = NULL;
	}

	FN_END;
}

static void __bt_rename_device_ok_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	bt_ug_data *ugd = (bt_ug_data *) data;

	const char *entry_str = elm_entry_entry_get(ugd->rename_entry);
	char *device_name_str = NULL;
	device_name_str = elm_entry_markup_to_utf8(entry_str);
	ret_if(!device_name_str);
	BT_DBG("Device name:[%s]", device_name_str);

	if (0 != vconf_set_str(VCONFKEY_SETAPPL_DEVICE_NAME_STR, device_name_str)) {
		BT_ERR("Set vconf[%s] failed",VCONFKEY_SETAPPL_DEVICE_NAME_STR);
	}

	_bt_update_genlist_item(ugd->device_name_item);
	evas_object_del(ugd->rename_popup);
	ugd->rename_popup = NULL;
	ugd->rename_entry_item = NULL;
	ugd->rename_entry = NULL;
	g_free(device_name_str);
	FN_END;
}

static void __bt_rename_entry_changed_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	ret_if(!data);
	bt_ug_data *ugd = (bt_ug_data *) data;
	ret_if(!ugd->rename_entry_item);

	if (elm_object_part_content_get(obj, "elm.icon.eraser")) {
		if (elm_object_focus_get(obj)) {
			if (elm_entry_is_empty(obj)) {
				elm_object_item_signal_emit(ugd->rename_entry_item,
					"elm,state,eraser,hide", "");
			} else {
				elm_object_item_signal_emit(ugd->rename_entry_item,
					"elm,state,eraser,show", "");
			}
		}
	}
	__bt_rename_device_entry_changed_cb(data, obj, event_info);

	FN_END;
}

static void __bt_rename_entry_focused_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	if (elm_object_part_content_get(obj, "elm.swallow.clear")) {
		if (!elm_entry_is_empty(obj))
			elm_object_signal_emit(obj, "elm,state,clear,visible", "");
		else
			elm_object_signal_emit(obj, "elm,state,clear,hidden", "");
	}
	elm_object_signal_emit(obj, "elm,state,focus,on", "");
	FN_END;
}

static void __bt_rename_entry_keydown_cb(void *data, Evas *e, Evas_Object *obj,
							void *event_info)
{
	FN_START;
	Evas_Event_Key_Down *ev;
	Evas_Object *entry = obj;

	ret_if(data == NULL);
	ret_if(event_info == NULL);
	ret_if(entry == NULL);

	ev = (Evas_Event_Key_Down *)event_info;
	BT_INFO("ENTER ev->key:%s", ev->key);

	if (g_strcmp0(ev->key, "KP_Enter") == 0 ||
			g_strcmp0(ev->key, "Return") == 0) {

		Ecore_IMF_Context *imf_context;

		imf_context =
			(Ecore_IMF_Context*)elm_entry_imf_context_get(entry);
		if (imf_context)
			ecore_imf_context_input_panel_hide(imf_context);

		elm_object_focus_set(entry, EINA_FALSE);
	}
	FN_END;
}

static Evas_Object *__bt_main_rename_entry_icon_get(
				void *data, Evas_Object *obj, const char *part)
{
	FN_START;
	retv_if (obj == NULL || data == NULL, NULL);

	Evas_Object *entry = NULL;
	char *name_value = NULL;
	char *name_value_utf = NULL;

	static Elm_Entry_Filter_Limit_Size limit_filter_data;
	bt_ug_data *ugd = (bt_ug_data *)data;

	if (!strcmp(part, "elm.icon.entry")) {
		name_value_utf = vconf_get_str(VCONFKEY_SETAPPL_DEVICE_NAME_STR);
		retvm_if (!name_value_utf, NULL, "Get string is failed");

		name_value = elm_entry_utf8_to_markup(name_value_utf);

		entry = elm_entry_add(obj);
		elm_entry_single_line_set(entry, EINA_TRUE);
		elm_entry_scrollable_set(entry, EINA_TRUE);

		eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);
		elm_entry_scrollable_set(entry, EINA_TRUE);
		elm_object_signal_emit(entry, "elm,action,hide,search_icon", "");
		elm_object_part_text_set(entry, "elm.guide", BT_STR_DEVICE_NAME);
		elm_entry_entry_set(entry, name_value);
		elm_entry_cursor_end_set(entry);
		elm_entry_input_panel_imdata_set(entry, "action=disable_emoticons", 24);

		elm_entry_input_panel_return_key_type_set(entry, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
		limit_filter_data.max_char_count = DEVICE_NAME_MAX_CHARACTER;
		elm_entry_markup_filter_append(entry,
			elm_entry_filter_limit_size, &limit_filter_data);

		elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);

		evas_object_smart_callback_add(entry, "maxlength,reached",
				_bt_util_max_len_reached_cb, ugd);
		evas_object_smart_callback_add(entry, "changed",
				__bt_rename_entry_changed_cb, ugd);
		evas_object_smart_callback_add(entry, "preedit,changed",
				__bt_rename_entry_changed_cb, ugd);
		evas_object_smart_callback_add(entry, "focused",
				__bt_rename_entry_focused_cb, NULL);
		evas_object_event_callback_add(entry, EVAS_CALLBACK_KEY_DOWN,
				__bt_rename_entry_keydown_cb, ugd);
		evas_object_show(entry);

		elm_object_focus_set(entry, EINA_TRUE);
		ugd->rename_entry = entry;

		if (name_value_utf)
			free(name_value_utf);
		if (name_value)
			free(name_value);

		return entry;
	}

	return NULL;
}

static Evas_Object *__bt_main_paired_device_icon_get(void *data, Evas_Object *obj,
					    const char *part)
{
	FN_START;

	Evas_Object *btn = NULL;
	Evas_Object *icon = NULL;
	char *dev_icon_file = NULL;
	bt_dev_t *dev = NULL;

	retv_if(data == NULL, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp("elm.swallow.icon", part)) {
		if ((dev->major_class == BT_MAJOR_DEV_CLS_MISC)
				&& (dev->service_list != 0))
			_bt_util_update_class_of_device_by_service_list(dev->service_list,
					&dev->major_class, &dev->minor_class);

		dev_icon_file =
		    _bt_main_get_device_icon(dev->major_class,
					     dev->minor_class,
					     dev->is_connected,
					     dev->highlighted);
		icon = _bt_create_icon(obj, dev_icon_file);
		evas_object_propagate_events_set(icon, EINA_FALSE);
		evas_object_show(icon);
		dev->icon = icon;
		if (dev->highlighted || dev->is_connected)
			evas_object_color_set(dev->icon, 20, 107, 147, 255);
		else
			evas_object_color_set(dev->icon, 76, 76, 76, 255);
	} else if (!strcmp("elm.swallow.end", part)) {
		BT_INFO("status : %d", dev->status);
		if (dev->status == BT_IDLE) {
			elm_object_style_set(btn, "info_button");

			evas_object_propagate_events_set(btn, EINA_FALSE);
			evas_object_smart_callback_add(btn, "clicked",
						       __bt_paired_device_profile_cb,
						       (void *)dev);
			evas_object_show(btn);
		}
	}
	if (icon)
		evas_object_show(icon);
	FN_END;
	return icon;
}

static char *__bt_main_searched_label_get(void *data, Evas_Object *obj,
					  const char *part)
{
	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0 };
	bt_dev_t *dev = NULL;

	if (data == NULL)
		return NULL;

	dev = (bt_dev_t *)data;
	if (!strcmp("elm.text", part)) {
#ifdef KIRAN_ACCESSIBILITY
		char str[BT_STR_ACCES_INFO_MAX_LEN] = { 0, };
		Evas_Object *ao = NULL;
#endif
		char *dev_name_markup = elm_entry_utf8_to_markup(dev->name);

		if (dev_name_markup) {
			g_strlcpy(buf, dev_name_markup,
					BT_GLOBALIZATION_STR_LENGTH);
			free(dev_name_markup);
		} else {
			g_strlcpy(buf, dev->name, BT_GLOBALIZATION_STR_LENGTH);
		}

		BT_INFO("label : %s", buf);
#ifdef KIRAN_ACCESSIBILITY
		snprintf(str, sizeof(str), "%s, %s", dev->name,
			 BT_STR_DOUBLE_TAP_CONNECT);

		ao = elm_object_item_access_object_get(dev->genlist_item);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
	} else {
		BT_ERR("empty text for label");
		return NULL;
	}

	return strdup(buf);
}

static Evas_Object *__bt_main_searched_icon_get(void *data,
						Evas_Object *obj,
						const char *part)
{
	Evas_Object *icon = NULL;
	char *dev_icon_file = NULL;
	bt_dev_t *dev = NULL;

	retv_if(data == NULL, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp("elm.swallow.icon", part)) {
		dev_icon_file =
		    _bt_main_get_device_icon(dev->major_class,
					     dev->minor_class,
					     dev->is_connected,
					     dev->highlighted);
		icon = _bt_create_icon(obj, dev_icon_file);
		if (dev->highlighted || dev->is_connected)
			evas_object_color_set(icon, 20, 107, 147, 255);
		else
			evas_object_color_set(icon, 76, 76, 76, 255);
		evas_object_propagate_events_set(icon, EINA_FALSE);


	} else if (!strcmp("elm.swallow.end", part)) {
		if (dev->status != BT_IDLE) {
			icon = _bt_create_progressbar(obj, "process_medium");
			evas_object_color_set(icon, 76, 76, 76, 255);
		}

	}

	return icon;
}

static char *__bt_main_no_device_label_get(void *data, Evas_Object *obj,
					   const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0 };
#ifdef KIRAN_ACCESSIBILITY
	bt_ug_data *ugd = NULL;
	char str[BT_STR_ACCES_INFO_MAX_LEN] = { 0, };
	Evas_Object *ao = NULL;
#endif
	if (!strcmp("elm.text", part)) {
		g_strlcpy(buf, BT_STR_NO_DEVICE_FOUND,
			  BT_GLOBALIZATION_STR_LENGTH);
		snprintf(buf, sizeof(buf), "<align=center>%s</align>", BT_STR_NO_DEVICE_FOUND);

#ifdef KIRAN_ACCESSIBILITY
		retv_if(data == NULL, NULL);
		ugd = (bt_ug_data *)data;
		g_strlcpy(str, BT_STR_NO_DEVICE_FOUND,
			  BT_STR_ACCES_INFO_MAX_LEN);

		ao = elm_object_item_access_object_get(ugd->no_device_item);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
	} else {
		BT_ERR("empty text for label");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_main_paired_title_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
#ifdef KIRAN_ACCESSIBILITY
	bt_ug_data *ugd = NULL;
	char str[BT_STR_ACCES_INFO_MAX_LEN] = { 0, };
	Evas_Object *ao = NULL;
#endif
	if (!strcmp("elm.text", part)) {
		/*Label */
		g_strlcpy(buf, BT_STR_PAIRED_DEVICES,
			  BT_GLOBALIZATION_STR_LENGTH);
	} else
		return NULL;

#ifdef KIRAN_ACCESSIBILITY
	retv_if(data == NULL, NULL);
	ugd = (bt_ug_data *)data;
	ao = elm_object_item_access_object_get(ugd->paired_title);
	snprintf(str, sizeof(str), "%s, %s", BT_STR_PAIRED_DEVICES,
		 BT_ACC_STR_GROUP_INDEX);
	elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
	FN_END;
	return strdup(buf);
}

static char *__bt_main_searched_title_label_get(void *data, Evas_Object *obj,
						const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
	bt_ug_data *ugd = NULL;
#ifdef KIRAN_ACCESSIBILITY
	Evas_Object *ao = NULL;
	char str[BT_STR_ACCES_INFO_MAX_LEN] = { 0, };
#endif

	retv_if(data == NULL, NULL);

	ugd = (bt_ug_data *)data;
	if (!strcmp("elm.text", part)) {
		/* Label */
		if (ugd->searched_device == NULL ||
		    eina_list_count(ugd->searched_device) == 0) {
			if (ugd->op_status == BT_SEARCHING) {
				g_strlcpy(buf, BT_STR_SCANNING,
					  BT_GLOBALIZATION_STR_LENGTH);
			} else if (ugd->op_status == BT_ACTIVATED) {
				g_strlcpy(buf, BT_STR_BLUETOOTH_DEVICES,
					  BT_GLOBALIZATION_STR_LENGTH);
			}
		} else {
			g_strlcpy(buf, BT_STR_AVAILABLE_DEVICES,
				  BT_GLOBALIZATION_STR_LENGTH);
		}

#ifdef KIRAN_ACCESSIBILITY
		snprintf(str, sizeof(str), "%s, %s",
					buf, BT_ACC_STR_GROUP_INDEX);
		ao = elm_object_item_access_object_get(ugd->searched_title);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
	} else {
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static Evas_Object *__bt_main_searched_title_icon_get(void *data, Evas_Object *obj,
					     const char *part)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	Evas_Object *progressbar = NULL;

	retv_if(data == NULL, NULL);
	retv_if(obj == NULL, NULL);
	retv_if(part == NULL, NULL);

	ugd = (bt_ug_data *)data;

	if (!strcmp("elm.swallow.end", part) && ugd->op_status == BT_SEARCHING) {
		progressbar = _bt_create_progressbar(obj, "process_small");
	}

	FN_END;
	return progressbar;
}

static void __bt_popup_visibility_delete_cb(void *data, Evas_Object *obj, void *event_info)
{
	bt_ug_data *ugd = (bt_ug_data *)data;
	ret_if(!ugd);
	if (ugd->visibility_popup) {
		evas_object_del(ugd->visibility_popup);
		ugd->visibility_popup = NULL;
	}
}

static void __bt_main_visible_item_sel(void *data, Evas_Object *obj,
				       void *event_info)
{
	FN_START;

	ret_if(data == NULL);
	ret_if(event_info == NULL);

	bt_ug_data *ugd = (bt_ug_data *)data;
	if(event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
				      EINA_FALSE);

	ugd->visibility_changed_by_ug = TRUE;

	Evas_Object *popup = NULL;
	Evas_Object *box = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;
	bt_radio_item *item = NULL;
	int i = 0;

	popup = elm_popup_add(ugd->base);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, __bt_popup_visibility_delete_cb, ugd);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_smart_callback_add(popup, "block,clicked", __bt_popup_visibility_delete_cb, ugd);

	elm_object_part_text_set(popup, "title,text", BT_STR_VISIBLE);

	/* box */
	box = elm_box_add(popup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	/* genlist */
	genlist = elm_genlist_add(box);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_FALSE);
	elm_genlist_block_count_set(genlist, 3);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	/* Set item class for timeout value */
	ugd->timeout_value_itc = elm_genlist_item_class_new();
	ret_if(ugd->timeout_value_itc == NULL);
	ugd->timeout_value_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	ugd->timeout_value_itc->func.text_get =
	    __bt_main_timeout_value_label_get;
	ugd->timeout_value_itc->func.content_get =
	    __bt_main_timeout_value_icon_get;
	ugd->timeout_value_itc->func.state_get = NULL;
	ugd->timeout_value_itc->func.del = __bt_main_timeout_value_del;

	for (i = 1; i <= BT_MAX_TIMEOUT_ITEMS; i++) {
		item = calloc(1, sizeof(bt_radio_item));
		ret_if(item == NULL);

		item->index = i;
		item->ugd = ugd;

		git = elm_genlist_item_append(genlist, ugd->timeout_value_itc,
					      (void *)item, NULL,
					      ELM_GENLIST_ITEM_NONE,
					      __bt_main_timeout_value_item_sel,
					      (void *)item);
		if (!git)
			BT_DBG("git is NULL!");
		item->it = git;
		ugd->visible_exp_item[i] = git;
	}

	evas_object_show(genlist);

	elm_box_pack_end(box, genlist);

	evas_object_size_hint_min_set(box, -1, ELM_SCALE_SIZE(480));
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	elm_object_content_set(popup, box);
	evas_object_show(popup);
	ugd->visibility_popup = popup;
	FN_END;
}

static app_control_h __bt_main_get_bt_onoff_result(bt_ug_data *ugd,
							gboolean result)
{
	app_control_h service = NULL;
	const char *result_str;
	bt_adapter_state_e bt_state = BT_ADAPTER_DISABLED;

	retv_if(ugd == NULL, NULL);

	app_control_create(&service);

	retv_if(service == NULL, NULL);

	if (result == TRUE)
		result_str = BT_RESULT_SUCCESS;
	else
		result_str = BT_RESULT_FAIL;

	if (app_control_add_extra_data(service, "result", result_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (bt_adapter_get_state(&bt_state) == BT_ERROR_NONE) {
		if (bt_state == BT_ADAPTER_ENABLED) {
			if (app_control_add_extra_data(service, "bt_status", BT_ADAPTER_ON) < 0) {
				BT_ERR("Fail to add extra data");
			}
		} else {
			if (app_control_add_extra_data(service, "bt_status", BT_ADAPTER_OFF) < 0) {
				BT_ERR("Fail to add extra data");
			}
		}
	} else {
		BT_ERR("Fail to bt_adapter_get_state");
	}

	return service;
}

static app_control_h __bt_main_get_visibility_result(bt_ug_data *ugd,
						 gboolean result)
{
	app_control_h service = NULL;
	char mode_str[BT_RESULT_STR_MAX] = { 0 };
	const char *result_str;
	int visibility = BT_VISIBLE_OFF;

	retv_if(ugd == NULL, NULL);

	app_control_create(&service);

	retv_if(service == NULL, NULL);

	if (result == TRUE)
		result_str = BT_RESULT_SUCCESS;
	else
		result_str = BT_RESULT_FAIL;

	if (app_control_add_extra_data(service, "result", result_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	/* Original output fields will be removed */
	snprintf(mode_str, BT_RESULT_STR_MAX, "%d", (int)ugd->selected_radio);

	if (app_control_add_extra_data(service, "visibility",
				   (const char *)mode_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	/* Convert the radio value to result */
	switch (ugd->selected_radio) {
	case 0:
		visibility = BT_VISIBLE_OFF;
		break;
	case 1:
	case 2:
	case 3:
		visibility = BT_VISIBLE_TIME_LIMITED;
		break;
	case 4:
		visibility = BT_VISIBLE_ALWAYS;
		break;
	default:
		visibility = BT_VISIBLE_OFF;
		break;
	}

	memset(mode_str, 0x00, BT_RESULT_STR_MAX);
	snprintf(mode_str, BT_RESULT_STR_MAX, "%d", visibility);

	if (app_control_add_extra_data(service, BT_APPCONTROL_VISIBILITY,
				   (const char *)mode_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	return service;
}

static app_control_h __bt_main_get_pick_result(bt_ug_data *ugd, gboolean result)
{
	app_control_h service = NULL;
	const char *result_str;
	char address[BT_ADDRESS_STR_LEN] = { 0 };
	char value_str[BT_RESULT_STR_MAX] = { 0 };
	bt_dev_t *dev;

	retv_if(ugd == NULL, NULL);
	retv_if(ugd->pick_device == NULL, NULL);

	dev = ugd->pick_device;

	app_control_create(&service);

	retv_if(service == NULL, NULL);

	if (result == TRUE)
		result_str = BT_RESULT_SUCCESS;
	else
		result_str = BT_RESULT_FAIL;

	if (app_control_add_extra_data(service, "result", result_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	_bt_util_addr_type_to_addr_result_string(address, dev->bd_addr);

	/* Original output fields will be removed */
	if (app_control_add_extra_data(service, "address",
				   (const char *)address) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, BT_APPCONTROL_ADDRESS,
				   (const char *)address) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, "name",
				   (const char *)dev->name) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, BT_APPCONTROL_NAME,
				   (const char *)dev->name) < 0) {
		BT_ERR("Fail to add extra data");
	}

	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->rssi);

	if (app_control_add_extra_data(service, "rssi",
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, BT_APPCONTROL_RSSI,
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->is_bonded);

	if (app_control_add_extra_data(service, "is_bonded",
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, BT_APPCONTROL_IS_PAIRED,
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->major_class);

	if (app_control_add_extra_data(service, "major_class",
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, BT_APPCONTROL_MAJOR_CLASS,
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%d", dev->minor_class);

	if (app_control_add_extra_data(service, "minor_class",
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, BT_APPCONTROL_MINOR_CLASS,
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	memset(value_str, 0x00, sizeof(value_str));
	snprintf(value_str, BT_RESULT_STR_MAX, "%ld",
		 (long int)dev->service_class);

	if (app_control_add_extra_data(service, "service_class",
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data(service, BT_APPCONTROL_SERVICE_CLASS,
				   (const char *)value_str) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data_array(service, "uuids",
					 (const char **)dev->uuids,
					 dev->uuid_count) < 0) {
		BT_ERR("Fail to add extra data");
	}

	if (app_control_add_extra_data_array(service, BT_APPCONTROL_UUID_LIST,
					 (const char **)dev->uuids,
					 dev->uuid_count) < 0) {
		BT_ERR("Fail to add extra data");
	}

	return service;
}

static Eina_Bool __bt_main_quit_btn_cb(void *data, Elm_Object_Item *it)
{
	FN_START;
	app_control_h reply = NULL;
	app_control_h service = NULL;
	bt_ug_data *ugd = (bt_ug_data *)data;

	retv_if(ugd == NULL, EINA_FALSE);

	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY) {
		service = __bt_main_get_visibility_result(ugd, FALSE);
		app_control_create(&reply);
		if (app_control_add_extra_data(reply, "result",
						BT_RESULT_FAIL) < 0) {
			BT_ERR("Fail to add extra data");
		}
		BT_DBG("BT_LAUNCH_VISIBILITY reply to launch request");
		app_control_reply_to_launch_request(reply, service,
						APP_CONTROL_RESULT_FAILED);

		_bt_ug_destroy(data, (void *)service);

		if (service)
			app_control_destroy(service);
		if (reply)
			app_control_destroy(reply);
	} else if (ugd->bt_launch_mode == BT_LAUNCH_PICK) {
		app_control_create(&service);

		if (service == NULL) {
			_bt_ug_destroy(data, NULL);
			return EINA_FALSE;
		}

		if (app_control_add_extra_data(service, "result",
					   BT_RESULT_FAIL) < 0) {
			BT_ERR("Fail to add extra data");
		}

		_bt_ug_destroy(data, (void *)service);

		app_control_destroy(service);
	} else if (ugd->bt_launch_mode == BT_LAUNCH_ONOFF) {
		service = __bt_main_get_bt_onoff_result(ugd, FALSE);

		app_control_create(&reply);
		if (app_control_add_extra_data(reply, "result",
							BT_RESULT_FAIL) < 0) {
			BT_ERR("Fail to add extra data");
		}

		BT_DBG("BT_LAUNCH_ONOFF reply to launch request");
		app_control_reply_to_launch_request(reply, service,
						APP_CONTROL_RESULT_FAILED);

		_bt_ug_destroy(data, (void *)service);

		if (service)
			app_control_destroy(service);
		if (reply)
			app_control_destroy(reply);
	} else {
		_bt_ug_destroy(data, NULL);
	}

	FN_END;
	return EINA_FALSE;
}

int _bt_main_enable_bt(void *data)
{
	FN_START;
	int ret;
	retv_if(data == NULL, -1);
	bt_ug_data *ugd = (bt_ug_data *)data;

	if (_bt_util_is_battery_low() == TRUE) {
		/* Battery is critical low */
		_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
		return -1;
	}

	ret = bt_adapter_enable();
	if (ret != BT_ERROR_NONE) {
		BT_ERR("Failed to enable bluetooth [%d]", ret);
	} else {
		ugd->op_status = BT_ACTIVATING;
	}

	FN_END;
	return 0;
}

int _bt_main_disable_bt(void *data)
{
	FN_START;
	int ret;
	retv_if(data == NULL, -1);
	bt_ug_data *ugd = (bt_ug_data *)data;

	ret = bt_adapter_disable();
	if (ret != BT_ERROR_NONE) {
		BT_ERR("Failed to disable bluetooth [%d]", ret);
	} else {
		ugd->op_status = BT_DEACTIVATING;
		elm_object_disabled_set(ugd->scan_btn, EINA_TRUE);
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

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (ugd->bt_launch_mode == BT_LAUNCH_HELP) {
		Eina_Bool check_mode = elm_check_state_get(obj);
		elm_check_state_set(obj, !check_mode);

		ret = notification_status_message_post(BT_STR_INVALID_ACTION_TRY_AGAIN);
		if (ret != NOTIFICATION_ERROR_NONE)
			BT_ERR("notification_status_message_post() is failed : %d", ret);

		FN_END;
		return;
	}
	ret_if (ugd->op_status == BT_ACTIVATING ||
		ugd->op_status == BT_DEACTIVATING);

	elm_object_disabled_set(ugd->onoff_btn, EINA_TRUE);

	if (ugd->op_status == BT_DEACTIVATED) {
		ret = _bt_main_enable_bt(data);
		ugd->op_status = BT_ACTIVATING;
	} else if (ugd->op_status != BT_DEACTIVATING &&
		ugd->op_status != BT_ACTIVATING) {
		ret = bt_adapter_disable();
		if (ret != BT_ERROR_NONE) {
			BT_ERR("Failed to disable bluetooth [%d]", ret);
		} else {
			ugd->op_status = BT_DEACTIVATING;
			elm_object_disabled_set(ugd->scan_btn, EINA_TRUE);
		}
	}

	if (ugd->op_status == BT_ACTIVATING ||
		ugd->op_status == BT_DEACTIVATING) {
		elm_genlist_item_fields_update(ugd->onoff_item, "*",
						ELM_GENLIST_ITEM_FIELD_TEXT);
		elm_genlist_item_fields_update(ugd->onoff_item, "*",
						ELM_GENLIST_ITEM_FIELD_CONTENT);
	}
	FN_END;
}

static void __bt_main_controlbar_btn_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retm_if(data == NULL, "Invalid argument: bt_ug_data is NULL");

	ugd = (bt_ug_data *)data;

	if (ugd->op_status == BT_SEARCHING) {
		if (ugd->is_discovery_started) {
			if (bt_adapter_stop_device_discovery() == BT_ERROR_NONE) {
				elm_object_disabled_set(ugd->scan_btn, EINA_TRUE);
				elm_object_text_set(ugd->scan_btn, BT_STR_STOP);
			} else { /*the case in which stop discovery returns error from Bluez*/
				ugd->op_status = BT_ACTIVATED;
				elm_object_text_set(ugd->scan_btn, BT_STR_SCAN);

				if (ugd->searched_title == NULL)
					_bt_main_add_searched_title(ugd);
				_bt_update_genlist_item((Elm_Object_Item *)
						ugd->searched_title);
			}
		}
	} else { /*ugd->op_status != BT_SEARCHING */
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

	retm_if(data == NULL, "Invalid argument: data is NULL");

	dev = (bt_dev_t *)data;
	retm_if(dev->ugd == NULL, "ugd is NULL");

	ugd = dev->ugd;

	if (ugd->popup) {
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	_bt_main_disconnect_device(ugd, dev);

	FN_END;
}

static int __bt_main_request_to_send(bt_ug_data *ugd, bt_dev_t *dev)
{
	obex_ipc_param_t param;
	char *value = NULL;
	char **array_val = NULL;
	int cnt;
	int i = 0;
	int ret;

	BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
	       dev->bd_addr[0], dev->bd_addr[1],
	       dev->bd_addr[2], dev->bd_addr[3],
	       dev->bd_addr[4], dev->bd_addr[5]);

	memset(&param, 0x00, sizeof(obex_ipc_param_t));
	memcpy(param.addr, dev->bd_addr, BT_ADDRESS_LENGTH_MAX);

	ret = app_control_get_extra_data_array(ugd->service, "files",
					   &array_val, &cnt);
	if (ret != APP_CONTROL_ERROR_NONE) {
		BT_ERR("Get data error");
		goto fail;
	}

	if (array_val == NULL)
		goto fail;

	param.file_cnt = cnt;
	param.filepath = g_new0(char *, cnt + 1);
	for (i = 0; i < cnt; i++) {
		if (array_val[i]) {
			param.filepath[i] = g_strdup(array_val[i]);
			DBG_SECURE("%s", param.filepath[i]);
			free(array_val[i]);
			array_val[i] = NULL;
		}
	}
	free(array_val);
	array_val = NULL;

	param.dev_name = g_strdup(dev->name);

	if (app_control_get_extra_data(ugd->service, "type", &value) < 0)
		BT_ERR("Get data error");

	if (value == NULL)
		goto fail;

	param.type = g_strdup(value);
	free(value);

	if (_bt_ipc_send_obex_message(&param, ugd) != BT_UG_ERROR_NONE)
		goto fail;

	g_free(param.dev_name);
	g_free(param.type);
	if (param.filepath) {
		for (i = 0; param.filepath[i] != NULL; i++)
			g_free(param.filepath[i]);
		g_free(param.filepath);
	}

	return BT_UG_ERROR_NONE;

 fail:
	if (array_val) {
		for (i = 0; i < cnt; i++) {
			if (array_val[i]) {
				free(array_val[i]);
			}
		}
	}
	_bt_main_launch_syspopup(ugd, BT_SYSPOPUP_REQUEST_NAME,
				 BT_STR_UNABLE_TO_SEND,
				 BT_SYSPOPUP_ONE_BUTTON_TYPE);

	g_free(param.dev_name);
	g_free(param.type);
	if (param.filepath) {
		for (i = 0; param.filepath[i] != NULL; i++)
			g_free(param.filepath[i]);
		g_free(param.filepath);
	}

	return BT_UG_FAIL;
}

static void __bt_main_paired_item_sel_cb(void *data, Evas_Object *obj,
					 void *event_info)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;
	Evas_Object *btn = NULL;
	Evas_Object *popup_btn = NULL;
	int ret;

	if(event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	retm_if(data == NULL, "Invalid argument: bt_ug_data is NULL");

	ugd = (bt_ug_data *)data;
	item = (Elm_Object_Item *) event_info;

	ret_if(ugd->waiting_service_response == TRUE);
	ret_if(ugd->op_status == BT_PAIRING);

	if (ugd->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if (ret != BT_ERROR_NONE) {
			BT_ERR("Fail to stop discovery");
			return;
		}
	}

	dev = _bt_main_get_dev_info(ugd->paired_device, item);
	retm_if(dev == NULL, "Invalid argument: device info is NULL");
	retm_if(dev->status != BT_IDLE,
		"Connecting / Disconnecting is in progress");

	if ((ugd->waiting_service_response) && (dev->service_list == 0)) {
		ugd->paired_item = item;

		_bt_main_popup_del_cb(ugd, NULL, NULL);

		ugd->popup_data.type = BT_POPUP_GETTING_SERVICE_LIST;
		ugd->popup = _bt_create_popup(ugd, _bt_main_popup_del_cb,
						ugd, 2);

		retm_if(ugd->popup == NULL, "fail to create popup!");
		btn = elm_button_add(ugd->popup);
		elm_object_style_set(btn, "popup");
		elm_object_domain_translatable_text_set(
			btn ,
			PKGNAME, "IDS_BT_BUTTON_OK");
		elm_object_part_content_set(ugd->popup, "button1", btn);
		evas_object_smart_callback_add(btn, "clicked", (Evas_Smart_Cb)
					       _bt_main_popup_del_cb, ugd);

		eext_object_event_callback_add(ugd->popup, EEXT_CALLBACK_BACK,
				_bt_main_popup_del_cb, ugd);

		evas_object_show(ugd->popup);
		return;
	}

	if (ugd->bt_launch_mode == BT_LAUNCH_NORMAL ||
	    ugd->bt_launch_mode == BT_LAUNCH_CONNECT_HEADSET ||
	    ugd->bt_launch_mode == BT_LAUNCH_USE_NFC ||
	    ugd->bt_launch_mode == BT_LAUNCH_HELP) {

		ugd->paired_item = item;

		if (dev->service_list == 0) {
			if (bt_device_start_service_search
			    ((const char *)dev->addr_str) == BT_ERROR_NONE) {

				dev->status = BT_SERVICE_SEARCHING;
				ugd->waiting_service_response = TRUE;
				ugd->request_timer =
				    ecore_timer_add(BT_SEARCH_SERVICE_TIMEOUT,
						    (Ecore_Task_Cb)
						    _bt_main_service_request_cb,
						    ugd);

				_bt_update_genlist_item(ugd->paired_item);
				return;
			} else {
				BT_ERR("service search error");
				return;
			}
		}

			if (dev->is_connected == 0) {
				/* Not connected case */
				_bt_main_connect_device(ugd, dev);
			} else {

			_bt_main_popup_del_cb(ugd, NULL, NULL);

			ugd->popup_data.type = BT_POPUP_DISCONNECT;
			ugd->popup_data.data = g_strdup(dev->name);
			ugd->popup = _bt_create_popup(ugd, NULL, NULL, 0);
			retm_if(!ugd->popup , "fail to create popup!");

			popup_btn = elm_button_add(ugd->popup);
			if (popup_btn) {
				elm_object_style_set(popup_btn, "popup");
				elm_object_domain_translatable_text_set(
					popup_btn ,
					PKGNAME, "IDS_BR_SK_CANCEL");
				elm_object_part_content_set(ugd->popup, "button1",
							    popup_btn);
				evas_object_smart_callback_add(popup_btn, "clicked",
							       _bt_main_popup_del_cb,
							       ugd);
			}

			popup_btn = elm_button_add(ugd->popup);
			if (popup_btn) {
				elm_object_style_set(popup_btn, "popup");
				elm_object_domain_translatable_text_set(
					popup_btn ,
					PKGNAME, "IDS_BT_SK_DISCONNECT");
				elm_object_part_content_set(ugd->popup, "button2",
							    popup_btn);
				evas_object_smart_callback_add(popup_btn, "clicked",
							       __bt_main_disconnect_cb,
							       dev);
				eext_object_event_callback_add(ugd->popup, EEXT_CALLBACK_BACK,
						eext_popup_back_cb, NULL);
			}
			evas_object_show(ugd->popup);

		}
	} else if (ugd->bt_launch_mode == BT_LAUNCH_SEND_FILE) {
		if (_bt_util_is_battery_low() == TRUE) {
			/* Battery is critical low */
			_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
			return;
		}

		if (__bt_main_request_to_send(ugd, dev) == BT_UG_ERROR_NONE)
			BT_DBG("Request file sending");

		_bt_ug_destroy(ugd, NULL);
	} else if (ugd->bt_launch_mode == BT_LAUNCH_PICK) {
		ugd->pick_device = dev;
		g_idle_add((GSourceFunc) _bt_idle_destroy_ug, ugd);
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

	if(event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	retm_if(data == NULL, "Invalid argument: bt_ug_data is NULL");

	ugd = (bt_ug_data *)data;

	ret_if(ugd->op_status == BT_PAIRING);

	item = (Elm_Object_Item *) event_info;

	dev = _bt_main_get_dev_info(ugd->searched_device,
				    (Elm_Object_Item *) event_info);
	retm_if(dev == NULL, "Invalid argument: device info is NULL");

	if (ugd->bt_launch_mode == BT_LAUNCH_USE_NFC) {
		char address[18] = { 0 };
		app_control_h service = NULL;

		app_control_create(&service);

		ret_if(service == NULL);

		BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
		       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
		       dev->bd_addr[4], dev->bd_addr[5]);

		_bt_util_addr_type_to_addr_string(address, dev->bd_addr);

		if (app_control_add_extra_data(service, "device_address",
					   (const char *)address) < 0) {
			BT_ERR("Fail to add extra data");
		}

		if (app_control_add_extra_data(service, "device_name",
					   (const char *)dev->name) < 0) {
			BT_ERR("Fail to add extra data");
		}

		_bt_ug_destroy(ugd, (void *)service);

		app_control_destroy(service);

		return;
	} else if (ugd->bt_launch_mode == BT_LAUNCH_PICK) {
		ugd->pick_device = dev;
		g_idle_add((GSourceFunc) _bt_idle_destroy_ug, ugd);
		return;
	}

	ugd->searched_item = item;

	if (_bt_util_is_battery_low() == TRUE) {
		/* Battery is critical low */
		_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
		return;
	}

	if (ugd->op_status == BT_SEARCHING) {
		ret = bt_adapter_stop_device_discovery();
		if (ret != BT_ERROR_NONE)
			BT_ERR("Fail to stop discovery");
	}

	if (ugd->bt_launch_mode == BT_LAUNCH_SEND_FILE) {

		if (__bt_main_request_to_send(ugd, dev) == BT_UG_ERROR_NONE) {
			BT_DBG("Request file sending");
		}

		_bt_ug_destroy(ugd, NULL);
		return;
	}

	if (_bt_main_request_pairing_with_effect(ugd, item) != BT_UG_ERROR_NONE) {
		ugd->searched_item = NULL;
	}

	FN_END;
}

static void __bt_main_gl_highlighted(void *data, Evas_Object *obj,
				     void *event_info)
{
	FN_START;

	bt_ug_data *ugd;
	bt_dev_t *dev;
	Elm_Object_Item *item = (Elm_Object_Item *) event_info;

	ret_if(item == NULL);

	ugd = (bt_ug_data *)data;
	ret_if(ugd == NULL);

	dev = _bt_main_get_dev_info(ugd->paired_device, item);
	if (dev == NULL)
		dev = _bt_main_get_dev_info(ugd->searched_device, item);

	ret_if(dev == NULL);

	dev->highlighted = TRUE;

	/* Update text */
	elm_genlist_item_fields_update(item, "*", ELM_GENLIST_ITEM_FIELD_TEXT);

	FN_END;
}

static void __bt_main_gl_unhighlighted(void *data, Evas_Object *obj,
				       void *event_info)
{
	FN_START;

	bt_ug_data *ugd;
	bt_dev_t *dev;
	Elm_Object_Item *item = (Elm_Object_Item *) event_info;

	ret_if(item == NULL);

	ugd = (bt_ug_data *)data;
	ret_if(ugd == NULL);

	dev = _bt_main_get_dev_info(ugd->paired_device, item);
	if (dev == NULL)
		dev = _bt_main_get_dev_info(ugd->searched_device, item);

	ret_if(dev == NULL);

	dev->highlighted = FALSE;

	/* Update text */
	elm_genlist_item_fields_update(item, "*", ELM_GENLIST_ITEM_FIELD_TEXT);

	FN_END;
}

void _bt_main_add_device_name_item(bt_ug_data *ugd, Evas_Object *genlist)
{
	FN_START;
	retm_if(ugd->op_status == BT_DEACTIVATED, "BT is turned off");

	Elm_Object_Item *git = NULL;
	/* Device name */
	git = elm_genlist_item_insert_after(genlist, ugd->device_name_itc, ugd, NULL,
					ugd->onoff_item, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	ugd->device_name_item = git;

	FN_END;
}

void _bt_main_add_visible_item(bt_ug_data *ugd, Evas_Object *genlist)
{
	FN_START;
	retm_if(ugd->op_status == BT_DEACTIVATED, "BT is turned off");

	Elm_Object_Item *git = NULL;

	if (ugd->bt_launch_mode == BT_LAUNCH_NORMAL ||
	     ugd->bt_launch_mode == BT_LAUNCH_HELP ||
	      ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY) {
		/* visibility */
		git = elm_genlist_item_insert_after(genlist, ugd->visible_itc, ugd,
					      NULL, ugd->device_name_item,
					      ELM_GENLIST_ITEM_NONE,
					      __bt_main_visible_item_sel, ugd);
		ugd->visible_item = git;
	}
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
	retv_if(!genlist, NULL);
	/* We shoud set the mode to compress
	   for using dialogue/2text.2icon.3.tb */
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_FALSE);
	elm_genlist_block_count_set(genlist, 3);

	evas_object_smart_callback_add(genlist, "highlighted",
				       __bt_main_gl_highlighted, ugd);

	evas_object_smart_callback_add(genlist, "unhighlighted",
				       __bt_main_gl_unhighlighted, ugd);

	ugd->radio_main = elm_radio_add(genlist);
	elm_radio_state_value_set(ugd->radio_main, 0);
	elm_radio_value_set(ugd->radio_main, 0);

	ugd->selected_radio =
	    _bt_util_get_timeout_index(ugd->visibility_timeout);

	ugd->on_itc = elm_genlist_item_class_new();
	retv_if(ugd->on_itc == NULL, NULL);

	ugd->on_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	ugd->on_itc->func.text_get = __bt_main_onoff_label_get;
	ugd->on_itc->func.content_get = __bt_main_onoff_icon_get;
	ugd->on_itc->func.state_get = NULL;
	ugd->on_itc->func.del = NULL;

	/* Set item class for dialogue seperator */
	ugd->off_itc = elm_genlist_item_class_new();
	retv_if(ugd->off_itc == NULL, NULL);

	ugd->off_itc->item_style = BT_GENLIST_MULTILINE_TEXT_STYLE;
	ugd->off_itc->func.text_get = __bt_main_onoff_label_get;
	ugd->off_itc->func.content_get = __bt_main_onoff_icon_get;
	ugd->off_itc->func.state_get = NULL;
	ugd->off_itc->func.del = NULL;

	/* Set item class for paired dialogue title */
	ugd->device_name_itc = elm_genlist_item_class_new();
	retv_if(ugd->device_name_itc == NULL, NULL);

	ugd->device_name_itc->item_style = BT_GENLIST_2LINE_BOTTOM_TEXT_STYLE;
	ugd->device_name_itc->func.text_get = __bt_main_device_label_get;
	ugd->device_name_itc->func.content_get = NULL;
	ugd->device_name_itc->func.state_get = NULL;
	ugd->device_name_itc->func.del = NULL;

	if (ugd->bt_launch_mode == BT_LAUNCH_NORMAL ||
		ugd->bt_launch_mode == BT_LAUNCH_HELP) {
		/* Set item class for visibility */
		ugd->visible_itc = elm_genlist_item_class_new();
		retv_if(ugd->visible_itc == NULL, NULL);

		ugd->visible_itc->item_style = BT_GENLIST_MULTILINE_TEXT_STYLE;
		ugd->visible_itc->func.text_get = __bt_main_visible_label_get;
		ugd->visible_itc->func.content_get = NULL;
		ugd->visible_itc->func.state_get = NULL;
		ugd->visible_itc->func.del = NULL;
	}

	/* Set item class for paired dialogue title */
	ugd->paired_title_itc = elm_genlist_item_class_new();
	retv_if(ugd->paired_title_itc == NULL, NULL);

	ugd->paired_title_itc->item_style = BT_GENLIST_GROUP_INDEX_STYLE;
	ugd->paired_title_itc->func.text_get = __bt_main_paired_title_label_get;
	ugd->paired_title_itc->func.content_get = NULL;
	ugd->paired_title_itc->func.state_get = NULL;
	ugd->paired_title_itc->func.del = NULL;

	/* Set item class for searched dialogue title */
	ugd->searched_title_itc = elm_genlist_item_class_new();
	retv_if(ugd->searched_title_itc == NULL, NULL);

	ugd->searched_title_itc->item_style = BT_GENLIST_GROUP_INDEX_STYLE;
	ugd->searched_title_itc->func.text_get = __bt_main_searched_title_label_get;
	ugd->searched_title_itc->func.content_get = __bt_main_searched_title_icon_get;
	ugd->searched_title_itc->func.state_get = NULL;
	ugd->searched_title_itc->func.del = NULL;

	/* Set item class for paired device */
	ugd->paired_device_itc = elm_genlist_item_class_new();
	retv_if(ugd->paired_device_itc == NULL, NULL);

	ugd->paired_device_itc->item_style = BT_GENLIST_2LINE_TOP_TEXT_ICON_STYLE;
	ugd->paired_device_itc->func.text_get = __bt_main_paired_device_label_get;
	ugd->paired_device_itc->func.content_get = __bt_main_paired_device_icon_get;
	ugd->paired_device_itc->func.state_get = NULL;
	ugd->paired_device_itc->func.del = NULL;

	/* Set item class for searched device */
	ugd->searched_device_itc = elm_genlist_item_class_new();
	retv_if(ugd->searched_device_itc == NULL, NULL);

	ugd->searched_device_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	ugd->searched_device_itc->func.text_get = __bt_main_searched_label_get;
	ugd->searched_device_itc->func.content_get = __bt_main_searched_icon_get;
	ugd->searched_device_itc->func.state_get = NULL;
	ugd->searched_device_itc->func.del = NULL;

	/* Set item class for no device */
	ugd->no_device_itc = elm_genlist_item_class_new();
	retv_if(ugd->no_device_itc == NULL, NULL);

	ugd->no_device_itc->item_style = BT_GENLIST_1LINE_TEXT_STYLE;
	ugd->no_device_itc->func.text_get = __bt_main_no_device_label_get;
	ugd->no_device_itc->func.content_get = NULL;
	ugd->no_device_itc->func.state_get = NULL;
	ugd->no_device_itc->func.del = NULL;

	/* Set item class for timeout value */
	ugd->timeout_value_itc = elm_genlist_item_class_new();
	retv_if(ugd->timeout_value_itc == NULL, NULL);

	ugd->timeout_value_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	ugd->timeout_value_itc->func.text_get =
	    __bt_main_timeout_value_label_get;
	ugd->timeout_value_itc->func.content_get =
	    __bt_main_timeout_value_icon_get;
	ugd->timeout_value_itc->func.state_get = NULL;
	ugd->timeout_value_itc->func.del = __bt_main_timeout_value_del;

	if (ugd->op_status == BT_DEACTIVATED ||
		ugd->op_status == BT_ACTIVATING)
		git = elm_genlist_item_append(genlist, ugd->off_itc, ugd, NULL,
	                                      ELM_GENLIST_ITEM_NONE,
	                                      __bt_main_onoff_btn_cb, ugd);
	else
		git = elm_genlist_item_append(genlist, ugd->on_itc, ugd, NULL,
	                                      ELM_GENLIST_ITEM_NONE,
	                                      __bt_main_onoff_btn_cb, ugd);

	ugd->onoff_item = git;

	_bt_main_add_device_name_item(ugd, genlist);
	_bt_main_add_visible_item(ugd, genlist);

	evas_object_show(genlist);

	FN_END;
	return genlist;
}

static Evas_Object *__bt_main_add_visibility_dialogue(Evas_Object * parent,
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
	elm_genlist_homogeneous_set(genlist, EINA_FALSE);
	elm_genlist_block_count_set(genlist, 3);

	evas_object_smart_callback_add(genlist, "highlighted",
				       __bt_main_gl_highlighted, ugd);

	evas_object_smart_callback_add(genlist, "unhighlighted",
				       __bt_main_gl_unhighlighted, ugd);

	ugd->radio_main = elm_radio_add(genlist);
	elm_radio_state_value_set(ugd->radio_main, 0);
	elm_radio_value_set(ugd->radio_main, 0);

	ugd->selected_radio =
	    _bt_util_get_timeout_index(ugd->visibility_timeout);

	ugd->on_itc = elm_genlist_item_class_new();
	retv_if(ugd->on_itc == NULL, NULL);

	ugd->on_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	ugd->on_itc->func.text_get = __bt_main_onoff_label_get;
	ugd->on_itc->func.content_get = __bt_main_onoff_icon_get;
	ugd->on_itc->func.state_get = NULL;
	ugd->on_itc->func.del = NULL;

	/* Set item class for dialogue seperator */
	ugd->off_itc = elm_genlist_item_class_new();
	retv_if(ugd->off_itc == NULL, NULL);

	ugd->off_itc->item_style = BT_GENLIST_MULTILINE_TEXT_STYLE;
	ugd->off_itc->func.text_get = __bt_main_onoff_label_get;
	ugd->off_itc->func.content_get = __bt_main_onoff_icon_get;
	ugd->off_itc->func.state_get = NULL;
	ugd->off_itc->func.del = NULL;

	/* Set item class for paired dialogue title */
	ugd->device_name_itc = elm_genlist_item_class_new();
	retv_if(ugd->device_name_itc == NULL, NULL);

	ugd->device_name_itc->item_style = BT_GENLIST_2LINE_BOTTOM_TEXT_STYLE;
	ugd->device_name_itc->func.text_get = __bt_main_device_label_get;
	ugd->device_name_itc->func.content_get = NULL;
	ugd->device_name_itc->func.state_get = NULL;
	ugd->device_name_itc->func.del = NULL;

	/* Set item class for visibility */
	ugd->visible_itc = elm_genlist_item_class_new();
	retv_if(ugd->visible_itc == NULL, NULL);

	ugd->visible_itc->item_style = BT_GENLIST_MULTILINE_TEXT_STYLE;
	ugd->visible_itc->func.text_get = __bt_main_visible_label_get;
	ugd->visible_itc->func.content_get = NULL;
	ugd->visible_itc->func.state_get = NULL;
	ugd->visible_itc->func.del = NULL;

	/* Set item class for timeout value */
	ugd->timeout_value_itc = elm_genlist_item_class_new();
	retv_if(ugd->timeout_value_itc == NULL, NULL);

	ugd->timeout_value_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	ugd->timeout_value_itc->func.text_get =
	    __bt_main_timeout_value_label_get;
	ugd->timeout_value_itc->func.content_get =
	    __bt_main_timeout_value_icon_get;
	ugd->timeout_value_itc->func.state_get = NULL;
	ugd->timeout_value_itc->func.del = __bt_main_timeout_value_del;


	if (ugd->op_status == BT_DEACTIVATED ||
		ugd->op_status == BT_ACTIVATING)
		git = elm_genlist_item_append(genlist, ugd->off_itc, ugd, NULL,
	                                      ELM_GENLIST_ITEM_NONE,
	                                      __bt_main_onoff_btn_cb, ugd);
	else
		git = elm_genlist_item_append(genlist, ugd->on_itc, ugd, NULL,
	                                      ELM_GENLIST_ITEM_NONE,
	                                      __bt_main_onoff_btn_cb, ugd);
	ugd->onoff_item = git;

	_bt_main_add_device_name_item(ugd, genlist);
	_bt_main_add_visible_item(ugd, genlist);
	evas_object_show(genlist);

	FN_END;
	return genlist;
}

static Evas_Object *__bt_main_add_onoff_dialogue(Evas_Object * parent,
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
	elm_genlist_homogeneous_set(genlist, EINA_FALSE);
	elm_genlist_block_count_set(genlist, 3);

	evas_object_smart_callback_add(genlist, "highlighted",
				       __bt_main_gl_highlighted, ugd);

	evas_object_smart_callback_add(genlist, "unhighlighted",
				       __bt_main_gl_unhighlighted, ugd);

	ugd->on_itc = elm_genlist_item_class_new();
	retv_if(ugd->on_itc == NULL, NULL);

	ugd->on_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	ugd->on_itc->func.text_get = __bt_main_onoff_label_get;
	ugd->on_itc->func.content_get = __bt_main_onoff_icon_get;
	ugd->on_itc->func.state_get = NULL;
	ugd->on_itc->func.del = NULL;

	/* Set item class for dialogue seperator */
	ugd->off_itc = elm_genlist_item_class_new();
	retv_if(ugd->off_itc == NULL, NULL);

	ugd->off_itc->item_style = BT_GENLIST_MULTILINE_TEXT_STYLE;
	ugd->off_itc->func.text_get = __bt_main_onoff_label_get;
	ugd->off_itc->func.content_get = __bt_main_onoff_icon_get;
	ugd->off_itc->func.state_get = NULL;
	ugd->off_itc->func.del = NULL;

	/* Set item class for paired dialogue title */
	ugd->device_name_itc = elm_genlist_item_class_new();
	retv_if(ugd->device_name_itc == NULL, NULL);

	ugd->device_name_itc->item_style = BT_GENLIST_2LINE_BOTTOM_TEXT_STYLE;
	ugd->device_name_itc->func.text_get = __bt_main_device_label_get;
	ugd->device_name_itc->func.content_get = NULL;
	ugd->device_name_itc->func.state_get = NULL;
	ugd->device_name_itc->func.del = NULL;


	if (ugd->op_status == BT_DEACTIVATED ||
		ugd->op_status == BT_ACTIVATING)
		git = elm_genlist_item_append(genlist, ugd->off_itc, ugd, NULL,
	                                      ELM_GENLIST_ITEM_NONE,
	                                      __bt_main_onoff_btn_cb, ugd);
	else
		git = elm_genlist_item_append(genlist, ugd->on_itc, ugd, NULL,
	                                      ELM_GENLIST_ITEM_NONE,
	                                      __bt_main_onoff_btn_cb, ugd);
	ugd->onoff_item = git;

	_bt_main_add_device_name_item(ugd, genlist);
	evas_object_show(genlist);

	FN_END;
	return genlist;
}

static gboolean __bt_main_system_popup_timer_cb(gpointer user_data)
{
	FN_START;

	int ret;
	bt_ug_data *ugd;
	bundle *b;

	retv_if(user_data == NULL, FALSE);

	ugd = (bt_ug_data *)user_data;

	b = ugd->popup_bundle;

	if (NULL == b) {
		BT_ERR("bundle is NULL");
		return FALSE;
	}

	ret = syspopup_launch("bt-syspopup", b);
	if (ret < 0) {
		BT_ERR("Sorry cannot launch popup");
	} else {
		BT_DBG("Finally Popup launched");
		bundle_free(b);
	}

	FN_END;
	return (ret < 0) ? TRUE : FALSE;
}

static bool __bt_main_get_mime_type(char *file)
{
	FN_START;

	char mime_type[BT_FILE_NAME_LEN_MAX] = { 0, };
	int len = strlen("image");

	retv_if(file == NULL, FALSE);

	if (aul_get_mime_from_file(file, mime_type,
				   BT_FILE_NAME_LEN_MAX) == AUL_R_OK) {
		BT_INFO("mime type=[%s]", mime_type);
		if (0 != strncmp(mime_type, "image", len))
			return FALSE;
	} else {
		BT_ERR("Error getting mime type");
		return FALSE;
	}

	FN_END;
	return TRUE;
}

static bool __bt_main_is_image_file(app_control_h service)
{
	FN_START;

	char *value = NULL;
	int number_of_files = 0;
	char *token = NULL;
	char *param = NULL;
	int i = 0;

	retvm_if(service == NULL, FALSE, "Invalid data bundle");

	if (app_control_get_extra_data(service, "filecount", &value) < 0)
		BT_ERR("Get data error");

	retv_if(value == NULL, FALSE);

	number_of_files = atoi(value);
	BT_INFO("[%d] files", number_of_files);
	free(value);
	value = NULL;

	if (number_of_files <= 0) {
		BT_ERR("No File");
		return FALSE;
	}

	if (app_control_get_extra_data(service, "files", &value) < 0)
		BT_ERR("Get data error");

	retv_if(value == NULL, FALSE);

	param = value;
	while (((token = strstr(param, "?")) != NULL) && i < number_of_files) {
		*token = '\0';
		if (!__bt_main_get_mime_type(param)) {
			*token = '?';
			free(value);
			return FALSE;
		}
		BT_INFO("File [%d] [%s]", i, param);
		*token = '?';
		param = token + 1;
		i++;
	}
	if (i == (number_of_files - 1)) {
		if (!__bt_main_get_mime_type(param)) {
			free(value);
			return FALSE;
		}
		BT_INFO("File [%d] [%s]", i, param);
	} else {
		BT_ERR("Count not match : [%d] / [%d]", number_of_files, i);
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

	if (ugd->op_status != BT_DEACTIVATED && ugd->op_status != BT_ACTIVATED) {
		BT_INFO("current bluetooth status [%d]", ugd->op_status);
		return;
	}

	if (_bt_util_is_battery_low() == TRUE) {
		/* Battery is critical low */
		_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
		return;
	}

	if (ugd->op_status == BT_DEACTIVATED) {
		ret = _bt_main_enable_bt((void *)ugd);
		if (!ret) {
			elm_object_disabled_set(ugd->scan_btn, EINA_TRUE);
		}
	} else {
		ret = bt_adapter_start_device_discovery();
		if (!ret) {
			/* Disable the Scan button till the BLUETOOTH_EVENT_DISCOVERY_STARTED is received */
			elm_object_disabled_set(ugd->scan_btn, EINA_TRUE);
			_bt_main_remove_all_searched_devices(ugd);
			ugd->op_status = BT_SEARCHING;
			elm_object_text_set(ugd->scan_btn, BT_STR_STOP);

			if (ugd->searched_title == NULL)
				_bt_main_add_searched_title(ugd);
		} else {
			BT_ERR("Operation failed : Error Cause[%d]", ret);
		}
	}

	FN_END;
}

int _bt_main_service_request_cb(void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retvm_if(data == NULL, BT_UG_FAIL,
		 "Invalid argument: bt_ug_data is NULL");

	ugd = (bt_ug_data *)data;

	if (ugd->request_timer) {
		ecore_timer_del(ugd->request_timer);
		ugd->request_timer = NULL;
	}

	/* Need to modify API: Address parameter */
	if (ugd->waiting_service_response == TRUE) {
		bt_dev_t *dev = NULL;

		ugd->waiting_service_response = FALSE;
		bt_device_cancel_service_search();

		dev =
		    _bt_main_get_dev_info(ugd->paired_device, ugd->paired_item);
		retvm_if(dev == NULL, BT_UG_FAIL, "dev is NULL");

		dev->status = BT_IDLE;
		_bt_update_genlist_item(ugd->paired_item);
		_bt_main_connect_device(ugd, dev);
	} else {
		ugd->paired_item = NULL;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

#ifdef KIRAN_ACCESSIBILITY
char *__bt_main_get_device_string(int major_class, int minor_class)
{
	FN_START;
	char *device_type = NULL;

	BT_DBG("major_class: 0x%x, minor_class: 0x%x", major_class,
	       minor_class);

	switch (major_class) {
	case BT_MAJOR_DEV_CLS_COMPUTER:
		device_type = g_strdup(BT_STR_COMPUTER);
		break;
	case BT_MAJOR_DEV_CLS_PHONE:
		device_type = g_strdup(BT_STR_PHONE);
		break;
	case BT_MAJOR_DEV_CLS_AUDIO:
		if (minor_class == BTAPP_MIN_DEV_CLS_HEADPHONES)
			device_type = g_strdup(BT_STR_SVC_STEREO);
		else
			device_type = g_strdup(BT_STR_SVC_HEADSET);
		break;
	case BT_MAJOR_DEV_CLS_LAN_ACCESS_POINT:
		device_type = g_strdup(BT_STR_LAN_ACCESS_POINT);
		break;
	case BT_MAJOR_DEV_CLS_IMAGING:
		if (minor_class == BTAPP_MIN_DEV_CLS_PRINTER)
			device_type = g_strdup(BT_STR_PRINTER);
		else if (minor_class == BTAPP_MIN_DEV_CLS_CAMERA)
			device_type = g_strdup(BT_STR_CAMERA);
		else if (minor_class == BTAPP_MIN_DEV_CLS_DISPLAY)
			device_type = g_strdup(BT_STR_DISPLAY);
		break;
	case BT_MAJOR_DEV_CLS_PERIPHERAL:
		if (minor_class == BTAPP_MIN_DEV_CLS_KEY_BOARD)
			device_type = g_strdup(BT_STR_KEYBOARD);
		else if (minor_class == BTAPP_MIN_DEV_CLS_POINTING_DEVICE)
			device_type = g_strdup(BT_STR_MOUSE);
		else if (minor_class == BTAPP_MIN_DEV_CLS_GAME_PAD)
			device_type = g_strdup(BT_STR_GAMING_DEVICE);
		else
			device_type = g_strdup(BT_STR_MOUSE);
		break;
	case BT_MAJOR_DEV_CLS_HEALTH:
		device_type = g_strdup(BT_STR_HEALTH_DEVICE);
		break;
	default:
		device_type = g_strdup(BT_STR_UNKNOWN);
		break;
	}

	if (device_type == NULL)
		device_type = g_strdup(BT_STR_UNKNOWN);

	FN_END;
	return device_type;
}
#endif

char *_bt_main_get_device_icon(int major_class, int minor_class,
			       int connected, gboolean highlighted)
{
	char *icon = BT_ICON_UNKNOWN;

	switch (major_class) {
	case BT_MAJOR_DEV_CLS_COMPUTER:
		icon = BT_ICON_PC;
		break;
	case BT_MAJOR_DEV_CLS_PHONE:
		icon = BT_ICON_PHONE;
		break;
	case BT_MAJOR_DEV_CLS_AUDIO:
		if (minor_class == BTAPP_MIN_DEV_CLS_HEADPHONES)
			icon = BT_ICON_HEADPHONE;
		else if (minor_class == BTAPP_MIN_DEV_CLS_VIDEO_DISPLAY_AND_LOUD_SPEAKER)
			icon = BT_ICON_DISPLAY;
		else
			icon = BT_ICON_HEADSET;
		break;
	case BT_MAJOR_DEV_CLS_LAN_ACCESS_POINT:
		icon = BT_ICON_NETWORK;
		break;
	case BT_MAJOR_DEV_CLS_IMAGING:
		if (minor_class == BTAPP_MIN_DEV_CLS_PRINTER)
			icon = BT_ICON_PRINTER;
		else if (minor_class == BTAPP_MIN_DEV_CLS_CAMERA)
			icon = BT_ICON_CAMERA;
		else if (minor_class == BTAPP_MIN_DEV_CLS_DISPLAY)
			icon = BT_ICON_DISPLAY;
		break;
	case BT_MAJOR_DEV_CLS_PERIPHERAL:
		if (minor_class == BTAPP_MIN_DEV_CLS_KEY_BOARD)
			icon = BT_ICON_KEYBOARD;
		else if (minor_class == BTAPP_MIN_DEV_CLS_POINTING_DEVICE)
			icon = BT_ICON_MOUSE;
		else if (minor_class == BTAPP_MIN_DEV_CLS_GAME_PAD)
			icon = BT_ICON_GAMING;
		else
			icon = BT_ICON_MOUSE;
		break;
	case BT_MAJOR_DEV_CLS_HEALTH:
		icon = BT_ICON_HEALTH;
		break;

	case BT_MAJOR_DEV_CLS_WEARABLE:
		if (minor_class == BTAPP_MIN_DEV_CLS_WRIST_WATCH)
			icon = BT_ICON_WATCH;
		else
			icon = BT_ICON_UNKNOWN;
		break;
	default:
		icon = BT_ICON_UNKNOWN;
		break;
	}

	return icon;
}

void _bt_main_popup_del_cb(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;
	retm_if(data == NULL, "Invalid argument: struct bt_appdata is NULL");

	bt_ug_data *ugd = (bt_ug_data *)data;

	if (ugd->popup) {
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}
	if (ugd->popup_data.data)
		g_free(ugd->popup_data.data);
	ugd->popup_data.data = NULL;

	if (ugd->visibility_popup) {
		evas_object_del(ugd->visibility_popup);
		ugd->visibility_popup = NULL;
	}

	ugd->back_cb = NULL;

	FN_END;
}

void _bt_back_btn_popup_del_cb(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;
	retm_if(data == NULL, "Invalid argument: struct bt_appdata is NULL");

	bt_ug_data *ugd = (bt_ug_data *)data;

	if (ugd->popup) {
		BT_DBG("Deleting popup");
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	ugd->op_status = BT_ACTIVATED;

	FN_END;
}

Elm_Object_Item *_bt_main_add_paired_device_on_bond(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git = NULL;

	retvm_if(ugd == NULL, NULL, "Invalid argument: ugd is NULL");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL");

	/* Paired device Title */
	if (ugd->paired_title == NULL) {
		if (ugd->searched_title == NULL) {
			git = elm_genlist_item_append(ugd->main_genlist,
								ugd->paired_title_itc,
								(void *)ugd, NULL,
								ELM_GENLIST_ITEM_NONE,
								NULL, NULL);
			elm_genlist_item_select_mode_set(git,
							 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		} else {
			git = elm_genlist_item_insert_before(ugd->main_genlist,
								ugd->paired_title_itc,
								(void *)ugd, NULL,
								ugd->searched_title,
								ELM_GENLIST_ITEM_NONE,
								NULL, NULL);
			elm_genlist_item_select_mode_set(git,
							 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		}
		ugd->paired_title = git;
	}

	dev->ugd = (void *)ugd;
	dev->is_bonded = TRUE;
	dev->status = BT_IDLE;

	/* Add the device item in the list */
	if (ugd->paired_device == NULL) {
		git = elm_genlist_item_insert_after(ugd->main_genlist,
					ugd->paired_device_itc, dev, NULL,
					ugd->paired_title,
					ELM_GENLIST_ITEM_NONE,
					__bt_main_paired_item_sel_cb,
					ugd);;
	} else {
		bt_dev_t *item_dev = NULL;
		Elm_Object_Item *item = NULL;
		Elm_Object_Item *next = NULL;

		item = elm_genlist_item_next_get(ugd->paired_title);

		while (item != NULL || item != ugd->searched_title) {
			item_dev = _bt_main_get_dev_info(ugd->paired_device, item);

			if (item_dev && item_dev->is_connected > 0) {
				next = elm_genlist_item_next_get(item);
				if (next == NULL || next == ugd->searched_title) {
					git = elm_genlist_item_insert_after(ugd->main_genlist,
							ugd->paired_device_itc, dev, NULL, item,
							ELM_GENLIST_ITEM_NONE,
							__bt_main_paired_item_sel_cb,
							ugd);
					break;
				}
				item = next;
			} else {
				git = elm_genlist_item_insert_before(ugd->main_genlist,
						ugd->paired_device_itc, dev,
						NULL, item, ELM_GENLIST_ITEM_NONE,
						__bt_main_paired_item_sel_cb, ugd);
				break;
			}
		}
	}
	dev->genlist_item = git;

	FN_END;
	return dev->genlist_item;
}

void _bt_sort_paired_device_list(bt_ug_data *ugd, bt_dev_t *dev, int connected)
{
	FN_START;

	bt_dev_t *item = NULL;
	Elm_Object_Item *git = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	retm_if(ugd == NULL, "Invalid argument: ugd is NULL");
	retm_if(dev == NULL, "Invalid argument: dev is NULL");

	EINA_LIST_FOREACH_SAFE(ugd->paired_device, l, l_next, item) {
		if (item && (item == dev)) {
			if (connected) {
				elm_object_item_del(item->genlist_item);
				git = elm_genlist_item_insert_after(ugd->main_genlist,
						ugd->paired_device_itc, dev, NULL,
						ugd->paired_title,
						ELM_GENLIST_ITEM_NONE,
						__bt_main_paired_item_sel_cb,
						ugd);
				dev->genlist_item = git;
				break;
			} else {
				if (ugd->searched_title) {
					elm_object_item_del(item->genlist_item);
					git = elm_genlist_item_insert_before(ugd->main_genlist,
							ugd->paired_device_itc, dev, NULL,
							ugd->searched_title,
							ELM_GENLIST_ITEM_NONE,
							__bt_main_paired_item_sel_cb,
							ugd);
					dev->genlist_item = git;
					break;
				} else {
					elm_object_item_del(item->genlist_item);
					git = elm_genlist_item_append(ugd->main_genlist,
							ugd->paired_device_itc, dev, NULL,
							ELM_GENLIST_ITEM_NONE,
							__bt_main_paired_item_sel_cb,
							ugd);
					dev->genlist_item = git;
					break;

				}
			}
		}
	}

	FN_END;
	return;
}

Elm_Object_Item *_bt_main_add_paired_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git = NULL;

	retvm_if(ugd == NULL, NULL, "Invalid argument: ugd is NULL");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL");

	/* Paired device Title */
	if (ugd->paired_title == NULL) {
		if (ugd->searched_title == NULL) {
			git = elm_genlist_item_append(ugd->main_genlist,
							ugd->paired_title_itc,
							(void *)ugd, NULL,
							ELM_GENLIST_ITEM_NONE,
							NULL, NULL);
			elm_genlist_item_select_mode_set(git,
							 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		} else {
			git = elm_genlist_item_insert_before(ugd->main_genlist,
					ugd->paired_title_itc,
							(void *)ugd, NULL,
							ugd->searched_title,
							ELM_GENLIST_ITEM_NONE,
							NULL, NULL);
			elm_genlist_item_select_mode_set(git,
							 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

		}
		ugd->paired_title = git;
	}

	dev->ugd = (void *)ugd;
	dev->is_bonded = TRUE;
	dev->status = BT_IDLE;

	/* Add the device item in the list */
	if (ugd->paired_device == NULL) {
		git = elm_genlist_item_insert_after(ugd->main_genlist,
							ugd->paired_device_itc, dev, NULL,
							ugd->paired_title,
							ELM_GENLIST_ITEM_NONE,
							__bt_main_paired_item_sel_cb,
							ugd);
	} else {
		if (dev->is_connected > 0) {
			git = elm_genlist_item_insert_after(ugd->main_genlist,
							ugd->paired_device_itc, dev, NULL,
							ugd->paired_title,
							ELM_GENLIST_ITEM_NONE,
							__bt_main_paired_item_sel_cb,
							ugd);
		} else {
			Elm_Object_Item *item = NULL;
			Elm_Object_Item *next = NULL;

			item = elm_genlist_item_next_get(ugd->paired_title);

			while (item != NULL || item != ugd->searched_title) {
				next = elm_genlist_item_next_get(item);
				if (next == NULL || next == ugd->searched_title) {
					git = elm_genlist_item_insert_after(ugd->main_genlist,
							ugd->paired_device_itc, dev, NULL, item,
							ELM_GENLIST_ITEM_NONE,
							__bt_main_paired_item_sel_cb,
							ugd);
					break;
				}
				item = next;
			}
		}
	}
	dev->genlist_item = git;

	FN_END;
	return dev->genlist_item;
}

Elm_Object_Item *_bt_main_add_searched_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	Elm_Object_Item *git = NULL;

	retvm_if(ugd == NULL, NULL, "Invalid argument: ugd is NULL");
	retvm_if(dev == NULL, NULL, "Invalid argument: dev is NULL");

	if (ugd->searched_title == NULL)
		_bt_main_add_searched_title(ugd);

	retvm_if(ugd->searched_title == NULL, NULL,
		 "Fail to add searched title genlist item");

	/* Searched device Item */
	if (ugd->searched_device == NULL) {
		git =
		    elm_genlist_item_insert_after(ugd->main_genlist,
						  ugd->searched_device_itc, dev, NULL,
						  ugd->searched_title,
						  ELM_GENLIST_ITEM_NONE,
						  __bt_main_searched_item_sel_cb,
						  ugd);
	} else {
		bt_dev_t *item_dev = NULL;
		Elm_Object_Item *item = NULL;
		Elm_Object_Item *next = NULL;

		item = elm_genlist_item_next_get(ugd->searched_title);

		/* check the RSSI value of searched device list add arrange its order */
		while (item != NULL) {
			item_dev =
			    _bt_main_get_dev_info(ugd->searched_device, item);
			retv_if(item_dev == NULL, NULL);

			if (item_dev->rssi > dev->rssi) {
				next = elm_genlist_item_next_get(item);
				if (next == NULL) {
					git =
					    elm_genlist_item_insert_after
					    (ugd->main_genlist,
					     ugd->searched_device_itc, dev, NULL, item,
					     ELM_GENLIST_ITEM_NONE,
					     __bt_main_searched_item_sel_cb,
					     ugd);
					break;
				}
				item = next;
			} else {
				git =
				    elm_genlist_item_insert_before
				    (ugd->main_genlist, ugd->searched_device_itc, dev,
				     NULL, item, ELM_GENLIST_ITEM_NONE,
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

	retvm_if(ugd == NULL, NULL, "Invalid argument: ugd is NULL");

	if (ugd->searched_title == NULL)
		_bt_main_add_searched_title(ugd);

	/* No device found Item */
	git =
	    elm_genlist_item_insert_after(ugd->main_genlist, ugd->no_device_itc,
	    			ugd, NULL, ugd->searched_title,
	    			ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_select_mode_set(git,
					 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	FN_END;
	return git;
}

static void __bt_move_help_ctxpopup(Evas_Object *ctx, bt_ug_data *ugd)
{
	FN_START;
	Evas_Coord w, h;
	int pos = -1;

	ret_if(ugd == NULL);
	ret_if(ugd->win_main == NULL);

	elm_win_screen_size_get(ugd->win_main, NULL, NULL, &w, &h);
	pos = elm_win_rotation_get(ugd->win_main);
	switch (pos) {
	case 0:
	case 180:
	case 360:
		evas_object_move(ctx, (w/2), h);
		break;
	case 90:
	case 270:
		evas_object_move(ctx, (h/2), w);
		break;
	default:
		evas_object_move(ctx, (w/2), h);
		break;
	}
	FN_END;
}

static void __bt_ctxpopup_rotate_cb(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;
	bt_ug_data *ugd = NULL;
	ret_if(data == NULL);
	ugd = (bt_ug_data *)data;

	__bt_move_help_ctxpopup(ugd->help_more_popup, ugd);
	evas_object_show(ugd->win_main);

	FN_END;
}

static void __bt_more_popup_del_cb(void *data)
{
	FN_START;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);
	ugd = (bt_ug_data *)data;

	if (ugd->help_more_popup != NULL) {
		evas_object_del(ugd->help_more_popup);
		ugd->help_more_popup = NULL;
	}

	FN_END;
}

static void __bt_more_popup_more_cb(void *data,
				Evas_Object *obj, void *event_info)
{
	FN_START;
	__bt_more_popup_del_cb((bt_ug_data *)data);
	FN_END;
}

static void __bt_more_popup_back_cb(void *data,
				Evas_Object *obj, void *event_info)
{
	FN_START;
	__bt_more_popup_del_cb((bt_ug_data *)data);
	FN_END;
}

static void __bt_more_popup_dismiss_cb(void *data, Evas_Object *obj,
						void *event)
{
	FN_START;
	bt_ug_data *ugd;

	ugd = (bt_ug_data *)data;
	ret_if(ugd == NULL);
	ret_if(ugd->help_more_popup == NULL);

	evas_object_del(ugd->help_more_popup);
	ugd->help_more_popup = NULL;

	FN_END;
}

static void __bt_more_popup_delete_cb(void *data, Evas *e,
		Evas_Object *obj,	void *event_info)
{
	FN_START;
	Evas_Object *navi = (Evas_Object *)data;
	Evas_Object *ctx = obj;

	ret_if (navi == NULL);

	evas_object_smart_callback_del(ctx, "dismissed",
			__bt_more_popup_dismiss_cb);
	evas_object_event_callback_del_full(ctx, EVAS_CALLBACK_DEL,
			__bt_more_popup_delete_cb, navi);
	FN_END;
}

#ifdef TIZEN_REDWOOD
static void __bt_ug_layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	FN_START;
	Evas_Object *base;

	ret_if (!ug);

	base = ug_get_layout(ug);
	if (!base) {
		BT_ERR("ug_get_layout() return NULL");
		ug_destroy(ug);
		return;
	}

	evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(base);
	FN_END;
}

static void __bt_ug_destroy_cb(ui_gadget_h ug, void *data)
{
	FN_START;
	ret_if(NULL == ug);
	ug_destroy(ug);

	FN_END;
}
#endif

static void __bt_more_popup_rename_device_item_sel_cb(void *data,
				Evas_Object *obj, void *event_inf)
{
	FN_START;
	ret_if(data == NULL);
	ret_if(event_inf == NULL);
	bt_ug_data *ugd = (bt_ug_data *)data;

	if(event_inf)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_inf,
						EINA_FALSE);

	ret_if(ugd == NULL);
	ret_if(ugd->help_more_popup == NULL);
	evas_object_del(ugd->help_more_popup);
	ugd->help_more_popup = NULL;
	Evas_Object *popup = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *button = NULL;
	Elm_Object_Item *git = NULL;


	popup = elm_popup_add(ugd->base);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
			__bt_rename_device_cancel_cb, ugd);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_object_domain_translatable_part_text_set(popup,
						"title,text",
						PKGNAME,
						"IDS_ST_HEADER_RENAME_DEVICE");

	genlist = elm_genlist_add(popup);
	evas_object_size_hint_weight_set(genlist,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	/* Entry genlist item */
	ugd->rename_entry_itc = elm_genlist_item_class_new();
	/* Fix : NULL_RETURNS */
	if (ugd->rename_entry_itc) {
		ugd->rename_entry_itc->item_style = "entry";
		ugd->rename_entry_itc->func.text_get = NULL;
		ugd->rename_entry_itc->func.content_get = __bt_main_rename_entry_icon_get;
		ugd->rename_entry_itc->func.state_get = NULL;
		ugd->rename_entry_itc->func.del = NULL;

		ugd->rename_entry_item = elm_genlist_item_append(genlist,
				ugd->rename_entry_itc, ugd,
				NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	}

	ugd->rename_desc_itc = elm_genlist_item_class_new();
	/* Fix : NULL_RETURNS */
	if (ugd->rename_desc_itc) {
		ugd->rename_desc_itc->item_style = BT_GENLIST_MULTILINE_TEXT_STYLE;
		ugd->rename_desc_itc->func.text_get = __bt_main_rename_desc_label_get;
		ugd->rename_desc_itc->func.content_get = NULL;
		ugd->rename_desc_itc->func.state_get = NULL;
		ugd->rename_desc_itc->func.del = NULL;

		git = elm_genlist_item_append(genlist, ugd->rename_desc_itc, NULL, NULL,
				ELM_GENLIST_ITEM_NONE, NULL, NULL);
	}
	elm_genlist_item_select_mode_set(git,
					 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	button = elm_button_add(popup);
	elm_object_style_set(button, "popup");
	elm_object_domain_translatable_text_set(button,
						PKGNAME,
						"IDS_BR_SK_CANCEL");
	elm_object_part_content_set(popup, "button1", button);
	evas_object_smart_callback_add(button, "clicked",
			__bt_rename_device_cancel_cb, ugd);

	button = elm_button_add(popup);
	ugd->rename_button = button;
	elm_object_style_set(button, "popup");
	elm_object_domain_translatable_text_set(button,
						PKGNAME,
						"IDS_BT_OPT_RENAME");
	elm_object_part_content_set(popup, "button2", button);
	evas_object_smart_callback_add(button, "clicked",
			__bt_rename_device_ok_cb, ugd);

	elm_genlist_realization_mode_set(genlist, EINA_TRUE);
	evas_object_show(genlist);

	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	elm_object_content_set(popup, genlist);
	evas_object_show(popup);

	if (ugd->rename_entry)
		elm_object_focus_set(ugd->rename_entry, EINA_TRUE);

	ugd->rename_popup = popup;

	FN_END;

}

#ifdef TIZEN_REDWOOD
static void __bt_more_popup_help_item_sel_cb(void *data,
				Evas_Object *obj, void *event_info)
{
	FN_START;
	ret_if(!data);

	bt_ug_data *ugd = (bt_ug_data *)data;
	app_control_h service = NULL;
	ui_gadget_h ug_h = NULL;
	struct ug_cbs cbs = {0};

	if (ugd->popup) {
		BT_DBG("delete popup");
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	app_control_create(&service);
	app_control_add_extra_data(service, "page","help_settings_bluetooth");

	cbs.layout_cb = __bt_ug_layout_cb;
	cbs.result_cb = NULL;
	cbs.destroy_cb = __bt_ug_destroy_cb;
	cbs.priv = NULL;

	ug_h = ug_create(NULL, "help-efl", UG_MODE_FULLVIEW, service, &cbs);
	if(!ug_h) {
		BT_ERR("ug_create() Failed");
	}
	app_control_destroy(service);
	__bt_more_popup_del_cb((bt_ug_data *)data);
	FN_END;
}
#endif

static void __bt_more_menu_cb(void *data,
				Evas_Object *obj, void *event_info)
{
	FN_START;

	Evas_Object *more_ctxpopup = NULL;
	bt_ug_data *ugd;

	ugd = (bt_ug_data *)data;
	ret_if(ugd == NULL);
	ret_if(ugd->profile_vd != NULL);
	ret_if(ugd->op_status == BT_ACTIVATING ||
		ugd->op_status == BT_DEACTIVATED ||
		ugd->op_status == BT_DEACTIVATING);
	ret_if(ugd->bt_launch_mode != BT_LAUNCH_NORMAL);

	_bt_main_popup_del_cb(ugd, NULL, NULL);

	more_ctxpopup = elm_ctxpopup_add(ugd->win_main);
	ugd->help_more_popup = more_ctxpopup;
	eext_object_event_callback_add(more_ctxpopup,
			EEXT_CALLBACK_BACK, __bt_more_popup_back_cb, ugd);
	eext_object_event_callback_add(more_ctxpopup,
			EEXT_CALLBACK_MORE, __bt_more_popup_more_cb, ugd);
	elm_object_style_set(more_ctxpopup, "more/default");
	elm_ctxpopup_auto_hide_disabled_set(more_ctxpopup, EINA_TRUE);

	elm_ctxpopup_item_append(more_ctxpopup, BT_STR_RENAME_DEVICE_TITLE,
			NULL, __bt_more_popup_rename_device_item_sel_cb, ugd);

	evas_object_smart_callback_add(more_ctxpopup, "dismissed",
			__bt_more_popup_dismiss_cb, ugd);
	evas_object_event_callback_add(more_ctxpopup, EVAS_CALLBACK_DEL,
			__bt_more_popup_delete_cb, ugd->navi_bar);
	evas_object_smart_callback_add(elm_object_top_widget_get(more_ctxpopup), "rotation,changed",
			__bt_ctxpopup_rotate_cb, ugd);

	elm_ctxpopup_direction_priority_set(more_ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
			ELM_CTXPOPUP_DIRECTION_DOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	__bt_move_help_ctxpopup(more_ctxpopup, ugd);
	evas_object_show(more_ctxpopup);

	FN_END;
}

Evas_Object * _bt_main_create_scan_button(bt_ug_data *ugd)
{
	Evas_Object *scan_button = NULL;

	scan_button = elm_button_add(ugd->navi_bar);

	/* Use "bottom" style button */
	elm_object_style_set(scan_button, "bottom");

	elm_object_text_set(scan_button, BT_STR_SCAN);

	evas_object_smart_callback_add(scan_button, "clicked",
			__bt_main_controlbar_btn_cb, ugd);

	/* Set button into "toolbar" swallow part */
	elm_object_item_part_content_set(ugd->navi_it, "toolbar", scan_button);

	return scan_button;
}

static void __bt_main_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	__bt_main_quit_btn_cb(data, NULL);
}

int _bt_main_draw_list_view(bt_ug_data *ugd)
{
	FN_START;

	Evas_Object *navi = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *navi_it = NULL;
	Evas_Object *back_button = NULL;

	retv_if(ugd == NULL, BT_UG_FAIL);

	navi = _bt_create_naviframe(ugd->base);
	elm_naviframe_prev_btn_auto_pushed_set(navi, EINA_FALSE);
	ugd->navi_bar = navi;

	eext_object_event_callback_add(navi, EEXT_CALLBACK_BACK,
				     eext_naviframe_back_cb, NULL);

	if (ugd->bt_launch_mode != BT_LAUNCH_HELP)
		eext_object_event_callback_add(navi, EEXT_CALLBACK_MORE,
						__bt_more_menu_cb, ugd);
	genlist = __bt_main_add_genlist_dialogue(navi, ugd);
	ugd->main_genlist = genlist;

	back_button = elm_button_add(navi);
	elm_object_style_set(back_button, "naviframe/end_btn/default");

	navi_it = elm_naviframe_item_push(navi, BT_STR_BLUETOOTH, back_button, NULL,
					  genlist, NULL);
	elm_naviframe_item_pop_cb_set(navi_it, __bt_main_quit_btn_cb, ugd);
	ugd->navi_it = navi_it;
	evas_object_smart_callback_add(back_button, "clicked", __bt_main_back_cb, ugd);

	if (ugd->op_status == BT_ACTIVATED) {
		ugd->scan_btn = _bt_main_create_scan_button(ugd);
	}

	ugd->confirm_req = BT_NONE_REQ;

	if (ugd->op_status == BT_ACTIVATED || ugd->op_status == BT_SEARCHING) {
		if (ugd->visible_item)
			elm_object_item_disabled_set(ugd->visible_item,
						     EINA_FALSE);
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_main_draw_visibility_view(bt_ug_data *ugd)
{
	FN_START;

	Evas_Object *navi = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *navi_it;

	retv_if(ugd == NULL, BT_UG_FAIL);

	navi = _bt_create_naviframe(ugd->base);

	elm_naviframe_prev_btn_auto_pushed_set(navi, EINA_FALSE);

	eext_object_event_callback_add(navi, EEXT_CALLBACK_BACK,
				     eext_naviframe_back_cb, NULL);

	genlist = __bt_main_add_visibility_dialogue(navi, ugd);

	navi_it =
	    elm_naviframe_item_push(navi, BT_STR_VISIBLE, NULL, NULL, genlist,
				    NULL);
	elm_naviframe_item_pop_cb_set(navi_it, __bt_main_quit_btn_cb, ugd);
	ugd->navi_it = navi_it;
	ugd->navi_bar = navi;
	ugd->main_genlist = genlist;
	ugd->confirm_req = BT_NONE_REQ;

	if (ugd->op_status == BT_ACTIVATED && ugd->visible_item)
		elm_object_item_disabled_set(ugd->visible_item, EINA_FALSE);

	FN_END;
	return BT_UG_ERROR_NONE;
}

int _bt_main_draw_onoff_view(bt_ug_data *ugd)
{
	FN_START;

	Evas_Object *navi = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *navi_it;

	retv_if(ugd == NULL, BT_UG_FAIL);

	navi = _bt_create_naviframe(ugd->base);

	elm_naviframe_prev_btn_auto_pushed_set(navi, EINA_FALSE);

	eext_object_event_callback_add(navi, EEXT_CALLBACK_BACK,
				     eext_naviframe_back_cb, NULL);

	genlist = __bt_main_add_onoff_dialogue(navi, ugd);

	navi_it =
	    elm_naviframe_item_push(navi, BT_STR_BLUETOOTH, NULL, NULL, genlist,
				    NULL);
	elm_naviframe_item_pop_cb_set(navi_it, __bt_main_quit_btn_cb, ugd);
	ugd->navi_it = navi_it;
	ugd->navi_bar = navi;
	ugd->main_genlist = genlist;
	ugd->confirm_req = BT_NONE_REQ;

	FN_END;
	return BT_UG_ERROR_NONE;
}

static Eina_Bool __bt_cb_register_net_state_cb(void *data)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Eina_List *l = NULL;
	Eina_List *l2 = NULL;
	bt_net_profile_t *profile_data = NULL;
	void *profile = NULL;
	char net_address[BT_ADDRESS_STR_LEN + 1] = { 0 };

	bt_ug_data *ugd = (bt_ug_data *)data;
	if (ugd->network_timer) {
		ecore_timer_del(ugd->network_timer);
		ugd->network_timer = NULL;
	}
	retv_if(ugd == NULL, ECORE_CALLBACK_CANCEL);
	retv_if(ugd->connection == NULL, ECORE_CALLBACK_CANCEL);

	if (ugd->paired_device) {
		EINA_LIST_FOREACH(ugd->paired_device, l, dev) {
			if ( dev &&
				(dev->service_list & BT_SC_PANU_SERVICE_MASK ||
				dev->service_list & BT_SC_NAP_SERVICE_MASK) &&
				dev->is_connected && !dev->net_profile) {
				if (!ugd->net_profile_list)
					ugd->net_profile_list =
					_bt_get_registered_net_profile_list(ugd->connection);
				retvm_if (!ugd->net_profile_list, ECORE_CALLBACK_CANCEL,
						"net_profile_list is NULL!");

				EINA_LIST_FOREACH(ugd->net_profile_list, l2, profile_data) {
					if (profile_data &&
					profile_data->profile_h &&
					profile_data->address) {
						_bt_util_addr_type_to_addr_net_string(net_address, dev->bd_addr);
						if (!g_strcmp0((const char *)profile_data->address, net_address)) {
							profile = _bt_get_registered_net_profile(ugd->connection,
								   dev->bd_addr);
							_bt_set_profile_state_changed_cb(profile, dev);
						}
					}
				}
			}
		}

		if (ugd->net_profile_list) {
			_bt_free_net_profile_list(ugd->net_profile_list);
			ugd->net_profile_list = NULL;
		}

	}

	FN_END;
	return ECORE_CALLBACK_CANCEL;
}

static bool __bt_cb_adapter_bonded_device(bt_device_info_s *device_info,
					  void *user_data)
{
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	unsigned int service_class;

	retv_if(!user_data, false);
	retv_if(!device_info->remote_name || strlen(device_info->remote_name) == 0, false);
	retv_if(!device_info->remote_address || strlen(device_info->remote_address) == 0, false);

	ugd = (bt_ug_data *)user_data;

	if (_bt_main_check_and_update_device(ugd->paired_device,
					     device_info->remote_address,
					     device_info->remote_name) >= 0) {
		/* Update all realized items */
		elm_genlist_realized_items_update(ugd->main_genlist);

		return true;
	}

	dev = _bt_main_create_paired_device_item(device_info);

	retv_if(!dev, false);

	dev->ugd = (void *)ugd;
	if (device_info->is_connected)
		dev->is_connected = _bt_util_check_any_profile_connected(dev);
	else
		dev->is_connected = device_info->is_connected;

	BT_INFO("device state(%d) overriding with profile state(%d)",
		device_info->is_connected, dev->is_connected);

	service_class = dev->service_class;

	/* WORKAROUND for a PLM issue.
	For some devices, service class is 0 even if it supports OPP server. So it is
	not showing in the device list when user try to share files via bluetooth.
	So override service_class with BT_COD_SC_OBJECT_TRANSFER in that case
	*/
	BT_DBG("overriding");
	if (service_class == 0) {
		if (dev->service_list & BT_SC_OPP_SERVICE_MASK)
			service_class = BT_COD_SC_OBJECT_TRANSFER;
	}

	if (_bt_main_is_matched_profile(ugd->search_type,
					dev->major_class,
					service_class,
					ugd->service,
					dev->minor_class) == TRUE) {
		BT_INFO("Device count [%d]",
		       eina_list_count(ugd->paired_device));

		if (_bt_main_add_paired_device(ugd, dev) != NULL) {
			ugd->paired_device =
			    eina_list_append(ugd->paired_device, dev);

			if (dev->service_list == 0) {
				if (bt_device_start_service_search
				    ((const char *)dev->addr_str) == BT_ERROR_NONE) {

					ugd->paired_item = dev->genlist_item;
					dev->status = BT_SERVICE_SEARCHING;
					ugd->waiting_service_response = TRUE;
					ugd->request_timer =
					    ecore_timer_add(BT_SEARCH_SERVICE_TIMEOUT,
							    (Ecore_Task_Cb)
							    _bt_main_service_request_cb,
							    ugd);
				} else {
					BT_ERR("service search error");
				}
			}
		}
	} else {
		BT_ERR("Device class and search type do not match");
		free(dev);
	}
	return true;
}

void _bt_main_draw_paired_devices(bt_ug_data *ugd)
{
	FN_START;

	ret_if(ugd == NULL);

	if (bt_adapter_foreach_bonded_device(__bt_cb_adapter_bonded_device,
	                                     (void *)ugd) != BT_ERROR_NONE) {
	        BT_ERR("bt_adapter_foreach_bonded_device() failed");
	        return;
	}

	ugd->network_timer = ecore_timer_add(1,__bt_cb_register_net_state_cb, ugd);

	if (!ugd->network_timer)
		BT_ERR("network_timer can not be added");
	FN_END;
	return;
}

void _bt_main_remove_paired_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	retm_if(ugd == NULL, "Invalid argument: ugd is NULL");
	retm_if(dev == NULL, "Invalid argument: dev is NULL");

	EINA_LIST_FOREACH_SAFE(ugd->paired_device, l, l_next, item) {
		if (item && (item == dev)) {
			elm_object_item_del(item->genlist_item);
			ugd->paired_device =
			    eina_list_remove_list(ugd->paired_device, l);
		}
	}

	if (ugd->paired_device == NULL ||
	    eina_list_count(ugd->paired_device) == 0) {
		elm_object_item_del(ugd->paired_title);
		ugd->paired_title = NULL;
	}
	FN_END;
	return;
}

void _bt_main_remove_all_paired_devices(bt_ug_data *ugd)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
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
	}

	EINA_LIST_FOREACH_SAFE(ugd->paired_device, l, l_next, dev) {
		ugd->paired_device =
		    eina_list_remove_list(ugd->paired_device, l);
		_bt_util_free_device_item(dev);
	}

	ugd->paired_device = NULL;

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
			if (dev->genlist_item) {
				elm_object_item_del(dev->genlist_item);
				dev->genlist_item = NULL;
			}
			ugd->searched_device =
			    eina_list_remove_list(ugd->searched_device, l);
			_bt_util_free_device_item(item);
		}
	}

	if (ugd->searched_device == NULL ||
			eina_list_count(ugd->searched_device) == 0) {
		elm_object_item_del(ugd->searched_title);
		ugd->searched_title = NULL;
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
		ugd->no_device_item = NULL;
	}

	EINA_LIST_FOREACH_SAFE(ugd->searched_device, l, l_next, dev) {
		ugd->searched_device =
		    eina_list_remove_list(ugd->searched_device, l);
		_bt_util_free_device_item(dev);
	}

	ugd->searched_device = NULL;

	FN_END;
	return;
}

void _bt_main_connect_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	int headset_type = BT_AUDIO_PROFILE_TYPE_ALL;

	ret_if(ugd == NULL);
	ret_if(dev == NULL);

#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	if (dev->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK) {
		if (bt_audio_connect(dev->addr_str,
				     BT_AUDIO_PROFILE_TYPE_A2DP_SINK) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_CONNECTING;
		} else {
			BT_ERR("Fail to connect BT_AUDIO_PROFILE_TYPE_A2DP_SINK");
		}
#else
#ifndef TELEPHONY_DISABLED
	if ((dev->service_list & BT_SC_HFP_SERVICE_MASK) ||
	    (dev->service_list & BT_SC_HSP_SERVICE_MASK)) {
		/* Connect the  Headset */
		if (dev->service_list & BT_SC_A2DP_SERVICE_MASK)
			headset_type = BT_AUDIO_PROFILE_TYPE_ALL;
		else
			headset_type = BT_AUDIO_PROFILE_TYPE_HSP_HFP;

		BT_INFO("Connection type = %d", headset_type);

		if (bt_audio_connect(dev->addr_str,
				     headset_type) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_CONNECTING;
		} else {
			BT_ERR("Fail to connect Headset device");
		}
	} else if (dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
#else
	if (dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
#endif
#endif
		if (bt_audio_connect(dev->addr_str,
				     BT_AUDIO_PROFILE_TYPE_A2DP) ==
		    BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_CONNECTING;
		} else {
			BT_ERR("Fail to connect Headset device");
		}
	} else if (dev->service_list & BT_SC_HID_SERVICE_MASK) {
#ifdef TIZEN_HID
		BT_INFO("HID connect request");

		if (bt_hid_host_connect(dev->addr_str) == BT_ERROR_NONE) {
			ugd->connect_req = TRUE;
			dev->status = BT_CONNECTING;
		} else {
			BT_ERR("Fail to connect HID device");
		}
#endif
	} else if (dev->service_list & BT_SC_NAP_SERVICE_MASK) {
		void *net_profile;
			if (dev->net_profile) {
				if (_bt_connect_net_profile(ugd->connection,
							    dev->net_profile,
							    dev) == BT_UG_ERROR_NONE) {
					ugd->connect_req = TRUE;
					dev->status = BT_CONNECTING;
				} else {
					BT_ERR("Fail to connect the net profile");
				}
			} else {
				net_profile = _bt_get_registered_net_profile(ugd->connection,
								   dev->bd_addr);
				if (net_profile) {
					_bt_set_profile_state_changed_cb(net_profile,
									 dev);
					if (_bt_connect_net_profile(ugd->connection,
								dev->net_profile,
								dev) == BT_UG_ERROR_NONE) {
						ugd->connect_req = TRUE;
						dev->status = BT_CONNECTING;
					} else {
						BT_ERR("Fail to connect the net profile");
					}
				}
			}
	}

	_bt_update_genlist_item((Elm_Object_Item *) dev->genlist_item);

	FN_END;
}

void _bt_main_disconnect_device(bt_ug_data *ugd, bt_dev_t *dev)
{
	FN_START;

	ret_if(ugd == NULL);
	ret_if(dev == NULL);

#ifndef TELEPHONY_DISABLED
	if (_bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
				     dev->bd_addr) == TRUE) {
		BT_INFO("Disconnecting AG service");
		if (bt_audio_disconnect(dev->addr_str,
					BT_AUDIO_PROFILE_TYPE_ALL) ==
		    BT_ERROR_NONE) {
			ugd->disconn_req = true;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_ERR("Fail to connect Headset device");
		}
	} else if (_bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
		 dev->bd_addr) == TRUE) {
#else
	if (_bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
		 dev->bd_addr) == TRUE) {
#endif
		BT_INFO("Disconnecting AV service");
		if (bt_audio_disconnect(dev->addr_str,
					BT_AUDIO_PROFILE_TYPE_A2DP) ==
		    BT_ERROR_NONE) {
			ugd->disconn_req = true;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_ERR("Fail to connect Headset device");
		}
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	} else if (_bt_util_is_profile_connected(BT_MUSIC_PLAYER_CONNECTED,
				dev->bd_addr) == TRUE) {
		BT_INFO("Disconnecting AV service");

		if (bt_audio_disconnect(dev->addr_str,
					BT_AUDIO_PROFILE_TYPE_A2DP_SINK) ==
		    BT_ERROR_NONE) {
			ugd->disconn_req = true;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_ERR("Fail to connect Headset device");
		}
#endif
	} else if (_bt_util_is_profile_connected(BT_HID_CONNECTED,
					    dev->bd_addr) == TRUE) {
		BT_INFO("Disconnecting HID service!!");

		if (bt_hid_host_disconnect(dev->addr_str) == BT_ERROR_NONE) {
			dev->status = BT_DISCONNECTING;
		} else {
			BT_ERR("Fail to disconnect HID device");
		}
		return;
	 } else if (_bt_util_is_profile_connected(BT_NETWORK_SERVER_CONNECTED,
					dev->bd_addr) == TRUE) {
		/* Need to check NAP server */
		if (bt_nap_disconnect(dev->addr_str) == BT_UG_ERROR_NONE) {
			ugd->connect_req = true;
			dev->status = BT_DISCONNECTING;
		} else {
			BT_ERR("Failed to disconnect pan server");
		}
	} else {
		void *net_profile;
		gboolean connected = FALSE;

		connected = _bt_util_is_profile_connected(BT_NETWORK_CONNECTED,
						dev->bd_addr);
		if (connected) {
			if (dev->net_profile) {
				if (_bt_disconnect_net_profile(ugd->connection,
						dev->net_profile, dev) ==
							BT_UG_ERROR_NONE) {
					ugd->connect_req = true;
					dev->status = BT_DISCONNECTING;
				} else {
					BT_ERR("Fail to disconnect the net profile");
				}
			} else {
				net_profile =
					_bt_get_registered_net_profile(
							ugd->connection,
							dev->bd_addr);
				if (net_profile) {
					if (_bt_disconnect_net_profile(
							ugd->connection,
							net_profile, dev) ==
								BT_UG_ERROR_NONE) {
						ugd->connect_req = true;
						dev->status = BT_DISCONNECTING;
					} else {
						BT_ERR("Fail to disconnect the net profile");
					}
				}
			}
		}
	}

	_bt_update_genlist_item((Elm_Object_Item *) dev->genlist_item);

	FN_END;
}

int _bt_main_request_pairing_with_effect(bt_ug_data *ugd,
					 Elm_Object_Item *seleted_item)
{
	FN_START;

	bt_dev_t *dev;

	retvm_if(ugd == NULL, BT_UG_FAIL, "Invalid argument: ugd is NULL");
	retvm_if(seleted_item == NULL, BT_UG_FAIL,
		 "Invalid argument: object is NULL");

	dev = _bt_main_get_dev_info(ugd->searched_device, seleted_item);
	retvm_if(dev == NULL, BT_UG_FAIL, "Invalid argument: dev is NULL");

	if (bt_device_create_bond(dev->addr_str) == BT_ERROR_NONE) {
		dev->status = BT_DEV_PAIRING;
		ugd->op_status = BT_PAIRING;

		elm_object_disabled_set(ugd->scan_btn, EINA_TRUE);
		_bt_update_genlist_item(seleted_item);
	} else {
		return BT_UG_FAIL;
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

void __bt_main_parse_service(bt_ug_data *ugd, app_control_h service)
{
	char *uri_scheme = NULL;;
	char *body_text = NULL;
	char *launch_type = NULL;
	char *operation = NULL;
	char *mime = NULL;
	const char *uri = NULL;
	const char *file_path = NULL;

	ret_if(ugd == NULL);
	ret_if(service == NULL);

	if (app_control_get_operation(service, &operation) < 0)
		BT_ERR("Get operation error");

	BT_INFO("operation: %s", operation);

	if (g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE) == 0) {
		launch_type = strdup("send");

		if (app_control_get_uri(service, (char **)&uri) < 0)
			BT_ERR("Get uri error");

		if (uri) {
			uri_scheme = g_uri_parse_scheme(uri);
			DBG_SECURE("uri_scheme: %s", uri_scheme);

			if (uri_scheme == NULL) {
				/* File transfer */
				file_path = g_filename_from_uri(uri, NULL, NULL);
				if (app_control_add_extra_data(service, "type", "file") < 0)
					BT_ERR("Fail to add extra data");
			} else if (g_strcmp0(uri_scheme, "file") == 0) {
				/* File transfer */
				file_path = g_filename_from_uri(uri, NULL, NULL);
				if (file_path == NULL) {
					file_path = strdup(uri + 7);	/* file:// */
				}
				if (app_control_add_extra_data(service, "type", "file") < 0)
					BT_ERR("Fail to add extra data");
			} else {
				if (app_control_add_extra_data(service, "type", "text") < 0)
					BT_ERR("Fail to add extra data");
			}

			if (file_path == NULL) {
				BT_ERR("Not include URI info");
				file_path = strdup(uri);
			}

			g_free(uri_scheme);
		} else {
			char *value = NULL;

			BT_INFO("url is not set");
			if (app_control_get_extra_data(service,
					MULTI_SHARE_SERVICE_DATA_PATH, &value) < 0)
					BT_ERR("Fail to get extra data");

			if (value) {
				file_path = g_strdup(value);
				free(value);

				DBG_SECURE("file_path: %s", file_path);

				if (app_control_add_extra_data(service, "type", "file") < 0)
					BT_ERR("Fail to add extra data");

			} else {
				BT_ERR("Not include path info");
				goto done;
			}
		}

		if (app_control_add_extra_data_array
		    (service, "files", &file_path, 1) < 0)
			BT_ERR("Fail to add extra data");
	} else if (g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE_TEXT) == 0) {
		BT_DBG("APP_CONTROL_OPERATION_SHARE_TEXT");

		launch_type = strdup("send");

		if (app_control_get_uri(service, (char **)&uri) < 0)
			BT_ERR("Get uri error");

		if (uri == NULL) {
			if (app_control_get_extra_data(service,
						   APP_CONTROL_DATA_TEXT,
						   &body_text) !=
			    APP_CONTROL_ERROR_NONE) {
				BT_ERR("Get uri error");
			}

			if (body_text == NULL) {
				goto done;
			} else {
				file_path = g_strdup(body_text);
				free(body_text);
			}
		} else {
			file_path = g_strdup(uri);
		}

		if (file_path == NULL) {
			BT_ERR("file path is NULL");
			goto done;
		}

		if (app_control_add_extra_data(service, "type", "text") < 0)
			BT_ERR("Fail to add extra data");

		if (app_control_add_extra_data_array
		    (service, "files", &file_path, 1) < 0)
			BT_ERR("Fail to add extra data");

	} else if (g_strcmp0(operation, APP_CONTROL_OPERATION_MULTI_SHARE) == 0) {
		launch_type = strdup("send");

		char **array_value = NULL;
		int array_length;
		int ret, i;

		ret = app_control_get_extra_data_array(service,
						   MULTI_SHARE_SERVICE_DATA_PATH,
						   &array_value, &array_length);
		if (ret != APP_CONTROL_ERROR_NONE) {
			BT_ERR("Get data error");
			if (array_value) {
				for (i = 0; i < array_length; i++) {
					if (array_value[i]) {
						free(array_value[i]);
					}
				}
				free(array_value);
			}
			goto done;
		}

		if (app_control_add_extra_data_array
		    (service, "files", (const char **)array_value,
		     array_length) < 0)
			BT_ERR("Fail to add extra data");

		if (app_control_add_extra_data(service, "type", "file") < 0)
			BT_ERR("Fail to add extra data");
		if (array_value) {
			for (i = 0; i < array_length; i++) {
				if (array_value[i]) {
					free(array_value[i]);
				}
			}
			free(array_value);
		}
	} else if (g_strcmp0(operation, BT_APPCONTROL_PICK_OPERATION) == 0) {
		BT_DBG("Pick Operation");
		launch_type = strdup("pick");
	} else if (g_strcmp0(operation, BT_APPCONTROL_VISIBILITY_OPERATION) == 0 ||
				 g_strcmp0(operation, APP_CONTROL_OPERATION_SETTING_BT_VISIBILITY) == 0 ) {
		BT_DBG("Visibility Operation");
		launch_type = strdup("visibility");
	} else if (g_strcmp0(operation, BT_APPCONTROL_ONOFF_OPERATION) == 0 ||
				 g_strcmp0(operation, APP_CONTROL_OPERATION_SETTING_BT_ENABLE) == 0 ) {
		BT_DBG("onoff Operation");
		launch_type = strdup("onoff");
	} else if (g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE_CONTACT) == 0) {
		BT_DBG("Share Contact Operation");
		launch_type = strdup("contact");
	} else if (g_strcmp0(operation, BT_APPCONTROL_EDIT_OPERATION ) == 0) {
		BT_DBG("Edit Operation");
		if (app_control_get_mime(service, &mime) < 0)
			BT_ERR("Get mime error");
		if (g_strcmp0(mime, BT_APPCONTROL_ONOFF_MIME) == 0) {
			launch_type = strdup("onoff");
		} else if(g_strcmp0(mime, BT_APPCONTROL_VISIBILITY_MIME) == 0) {
			launch_type = strdup("visibility");
		}
	} else if (app_control_get_extra_data(service, "launch-type",
					  &launch_type) == APP_CONTROL_ERROR_NONE) {
		if (g_strcmp0(launch_type, "call") != 0) {
			BT_DBG("launch-type : except call");
			if (app_control_add_extra_data(service, "type", "file") < 0)
				BT_ERR("Fail to add extra data");
		} else {
			BT_DBG("launch-type : call");
		}
	} else if (app_control_get_uri(service, (char **)&uri) == APP_CONTROL_ERROR_NONE &&
			g_strcmp0(uri, HELP_SETUP_BLUETOOTH_URI) == 0) {
		BT_DBG("Help mode");
		launch_type = strdup("help");
	}

 done:
	if (launch_type) {
		BT_INFO("Launch with launch type [%s]", launch_type);
		_bt_util_set_value(launch_type, &ugd->search_type,
				   &ugd->bt_launch_mode);
	} else {
		BT_DBG("launch type is NULL");
	}

	if (uri)
		free((void *)uri);

	if (file_path)
		free((void *)file_path);

	if (launch_type)
		free((void *)launch_type);

	if (operation)
		free(operation);

	if(mime)
		free(mime);
}

void _bt_main_init_status(bt_ug_data *ugd, void *data)
{
	FN_START;

	app_control_h service = NULL;
	int remain_time = 0;
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

	/* currently this is the workaround for relaunching bt-ug by s-finder.
	bt-ug is not properly working because of the previous initialization.
	If there was no bt-ug then this deinit code doesn't affect at all.*/
	bt_deinitialize();

	if (bt_initialize() != BT_ERROR_NONE)
		BT_ERR("bt_initialize() failed");

	if (bt_adapter_get_state(&bt_state) != BT_ERROR_NONE)
		BT_ERR("bt_adapter_get_state() failed");

	if (bt_state == BT_ADAPTER_DISABLED) {
		ugd->op_status = BT_DEACTIVATED;
		mode = BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;
	} else {
		if (bt_adapter_is_discovering(&status) != BT_ERROR_NONE)
		BT_ERR("bt_adapter_is_discovering() failed");

		if (status == true)
			bt_adapter_stop_device_discovery();

		ugd->op_status = BT_ACTIVATED;

		if (bt_adapter_get_visibility(&mode, &remain_time) != BT_ERROR_NONE)
			BT_ERR("bt_adapter_get_visibility() failed");
	}

	if (mode == BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE) {
		ugd->visible = FALSE;
		ugd->visibility_timeout = 0;
	} else if (mode == BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE) {
		ugd->visible = TRUE;
		ugd->visibility_timeout = -1;
	} else {
		/* BT_ADAPTER_VISIBILITY_MODE_LIMITED_DISCOVERABLE */
		/* Need to add the code for getting timeout */
		if (vconf_get_int(BT_FILE_VISIBLE_TIME,
				  &ugd->visibility_timeout)) {
			BT_DBG("Get the timeout value");
		}

		ugd->remain_time = remain_time;

		if (ugd->remain_time > 0) {
			/* Set current time snapshot */
			time(&(ugd->start_time));
			ugd->timeout_id = g_timeout_add(BT_VISIBILITY_TIMEOUT,
							(GSourceFunc)
							__bt_main_visible_timeout_cb,
							ugd);
		} else {
			ugd->visibility_timeout = 0;
		}
	}

	FN_END;
}

bt_dev_t *_bt_main_create_paired_device_item(void *data)
{
	FN_START;

	int i;
	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
	bt_dev_t *dev = NULL;
	bt_device_info_s *dev_info = NULL;

	retv_if(data == NULL, NULL);

	dev_info = (bt_device_info_s *)data;

	retv_if(!dev_info->remote_name || strlen(dev_info->remote_name) == 0, NULL);
	retv_if(!dev_info->remote_address || strlen(dev_info->remote_address) == 0, NULL);

	dev = malloc(sizeof(bt_dev_t));
	retv_if(dev == NULL, NULL);

	memset(dev, 0, sizeof(bt_dev_t));
	g_strlcpy(dev->name, dev_info->remote_name,
		  DEVICE_NAME_MAX_LEN + 1);

	dev->major_class = dev_info->bt_class.major_device_class;
	dev->minor_class = dev_info->bt_class.minor_device_class;
	dev->service_class = dev_info->bt_class.major_service_class_mask;

	if (dev_info->service_uuid != NULL && dev_info->service_count > 0) {
		dev->uuids = g_new0(char *, dev_info->service_count + 1);

		for (i = 0; i < dev_info->service_count; i++) {
			dev->uuids[i] = g_strdup(dev_info->service_uuid[i]);
		}

		dev->uuid_count = dev_info->service_count;
	}

	_bt_util_addr_string_to_addr_type(bd_addr, dev_info->remote_address);

	memcpy(dev->addr_str, dev_info->remote_address, BT_ADDRESS_STR_LEN);

	memcpy(dev->bd_addr, bd_addr, BT_ADDRESS_LENGTH_MAX);

	bt_device_get_service_mask_from_uuid_list(dev_info->service_uuid,
						 dev_info->service_count,
						 &dev->service_list);

	BT_DBG("device name [%s]", dev->name);
	BT_DBG("device major class [%x]", dev->major_class);
	BT_DBG("device minor class [%x]", dev->minor_class);
	BT_DBG("device service class [%x]", dev->service_class);
	BT_DBG("device service list %x", dev->service_list);
	BT_DBG("%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X", dev->bd_addr[0],
	       dev->bd_addr[1], dev->bd_addr[2], dev->bd_addr[3],
	       dev->bd_addr[4], dev->bd_addr[5]);

	if (dev->major_class == BT_MAJOR_DEV_CLS_MISC &&
		dev->service_list != BT_SC_NONE) {
		_bt_util_update_class_of_device_by_service_list(dev->service_list,
						 &dev->major_class, &dev->minor_class);
	}

	FN_END;
	return dev;
}

bt_dev_t *_bt_main_create_searched_device_item(void *data)
{
	FN_START;

	int i;
	unsigned char bd_addr[BT_ADDRESS_LENGTH_MAX];
	bt_dev_t *dev = NULL;
	bt_adapter_device_discovery_info_s *dev_info = NULL;

	retv_if(data == NULL, NULL);

	dev_info = (bt_adapter_device_discovery_info_s *) data;

	if (strlen(dev_info->remote_name) == 0)
		return NULL;

	dev = calloc(1, sizeof(bt_dev_t));
	retv_if(dev == NULL, NULL);

	strncpy(dev->name, dev_info->remote_name, DEVICE_NAME_MAX_LEN);

	dev->major_class = dev_info->bt_class.major_device_class;
	dev->minor_class = dev_info->bt_class.minor_device_class;
	dev->service_class = dev_info->bt_class.major_service_class_mask;
	dev->rssi = dev_info->rssi;

	if (dev_info->service_uuid != NULL && dev_info->service_count > 0) {
		dev->uuids = g_new0(char *, dev_info->service_count + 1);

		for (i = 0; i < dev_info->service_count; i++) {
			dev->uuids[i] = g_strdup(dev_info->service_uuid[i]);
		}

		dev->uuid_count = dev_info->service_count;
	}

	_bt_util_addr_string_to_addr_type(bd_addr, dev_info->remote_address);

	memcpy(dev->addr_str, dev_info->remote_address, BT_ADDRESS_STR_LEN);

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
static gboolean _bt_check_minor_class(unsigned int minor_class)
{
	switch(minor_class) {
		case BTAPP_MIN_DEV_CLS_VIDEO_DISPLAY_AND_LOUD_SPEAKER:
			return FALSE;
		case BTAPP_MIN_DEV_CLS_VIDEO_CONFERENCING:
			return FALSE;
		case BTAPP_MIN_DEV_CLS_VIDEO_CAMERA:
			return FALSE;
		case BTAPP_MIN_DEV_CLS_SET_TOP_BOX:
			return FALSE;
		case BTAPP_MIN_DEV_CLS_VCR:
			return FALSE;
		case BTAPP_MIN_DEV_CLS_CAM_CORDER:
			return FALSE;
		default:
			return TRUE;
	}
}

gboolean _bt_main_is_matched_profile(unsigned int search_type,
				     unsigned int major_class,
				     unsigned int service_class,
				     app_control_h service,
				     unsigned int minor_class)
{
	FN_START;

	bt_device_major_mask_t major_mask = BT_DEVICE_MAJOR_MASK_MISC;

#ifndef TIZEN_HID
	/* P141010-03829 : Kiran doesn't support HID device */
	BT_DBG("Kiran doesn't support HID device");
	retv_if(major_class == BT_MAJOR_DEV_CLS_PERIPHERAL, FALSE);
#endif

	if (search_type == 0x000000)
		return TRUE;

	BT_INFO("search_type: %x", search_type);
	BT_INFO("service_class: %x", service_class);

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
	BT_INFO("major_class %x", major_class);
	BT_INFO("minor_class %x", minor_class);
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
		if (_bt_check_minor_class(minor_class) == TRUE)
			major_mask = BT_DEVICE_MAJOR_MASK_AUDIO;
		else
			major_mask = BT_DEVICE_MAJOR_MASK_MISC;
		break;
#ifdef TIZEN_HID
	case BT_MAJOR_DEV_CLS_PERIPHERAL:
		major_mask = BT_DEVICE_MAJOR_MASK_PERIPHERAL;
		break;
#endif
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

	BT_INFO("major_mask: %x", major_mask);

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

	retvm_if(list == NULL, NULL, "Invalid argument: list is NULL");
	retvm_if(genlist_item == NULL, NULL, "Invalid argument: obj is NULL");

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (item->genlist_item == genlist_item)
				return item;
		}
	}

	FN_END;
	return NULL;
}

bt_dev_t *_bt_main_get_dev_info_by_address(Eina_List *list, char *address)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retvm_if(list == NULL, NULL, "Invalid argument: list is NULL");
	retvm_if(address == NULL, NULL, "Invalid argument: addr is NULL");

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (memcmp(item->addr_str, address, BT_ADDRESS_STR_LEN)
			    == 0)
				return item;
		}
	}

	FN_END;
	return NULL;
}

int _bt_main_check_and_update_device(Eina_List *list, char *addr, char *name)
{
	FN_START;

	bt_dev_t *item = NULL;
	Eina_List *l = NULL;

	retv_if(list == NULL, -1);
	retv_if(addr == NULL, -1);
	retv_if(name == NULL, -1);

	EINA_LIST_FOREACH(list, l, item) {
		if (item) {
			if (memcmp(item->addr_str, addr, BT_ADDRESS_STR_LEN) ==
			    0) {
				memset(item->name, 0x00,
				       DEVICE_NAME_MAX_LEN+1);
				g_strlcpy(item->name, name,
					  DEVICE_NAME_MAX_LEN);
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

	b = bundle_create();
	ret_if(b == NULL);

	if (event_type)
		bundle_add_str(b, "event-type", event_type);
	if (title)
		bundle_add_str(b, "title", title);
	if (type)
		bundle_add_str(b, "type", type);

	ret = syspopup_launch("bt-syspopup", b);
	if (0 > ret) {
		BT_ERR("Popup launch failed...retry %d", ret);
		ugd->popup_bundle = b;
		ugd->popup_timer =
		    g_timeout_add(BT_UG_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS,
				  (GSourceFunc) __bt_main_system_popup_timer_cb,
				  ugd);
	} else {
		bundle_free(b);
	}
	FN_END;
}

void _bt_main_create_information_popup(void *data, char *msg)
{
	FN_START;
	ret_if(data == NULL);
	bt_ug_data *ugd = (bt_ug_data *)data;

	_bt_main_popup_del_cb(data, NULL, NULL);

	ugd->popup_data.type = BT_POPUP_LOW_BATTERY;
	ugd->popup = _bt_create_popup(ugd,
		_bt_main_popup_del_cb, ugd, 2);
	retm_if(!ugd->popup , "fail to create popup!");

	eext_object_event_callback_add(ugd->popup, EEXT_CALLBACK_BACK,
			_bt_main_popup_del_cb, ugd);

	evas_object_show(ugd->popup);
	FN_END;
}

#ifdef TIZEN_REDWOOD
static gboolean __bt_main_close_help_popup_cb(gpointer data)
{
	FN_START;
	retv_if(!data, FALSE);
	bt_ug_data *ugd = (bt_ug_data *)data;

	if (ugd->popup) {
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
	}

	_bt_ug_destroy(data, NULL);

	FN_END;
	return FALSE;
}
#endif

void _bt_main_add_searched_title(bt_ug_data *ugd)
{
	FN_START;
	Elm_Object_Item *git = NULL;

	git = elm_genlist_item_append(ugd->main_genlist,
					ugd->searched_title_itc,
					(void *)ugd, NULL,
					ELM_GENLIST_ITEM_NONE, NULL, NULL);
	if (git) {
		elm_genlist_item_select_mode_set(git,
					 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		ugd->searched_title = git;
	} else {
		BT_ERR("fail to append genlist item!");
	}

	FN_END;
}

void _bt_update_device_list(bt_ug_data *ugd)
{
	Eina_List *l = NULL;
	bt_dev_t *dev = NULL;

	ret_if(ugd == NULL);

	EINA_LIST_FOREACH(ugd->paired_device, l, dev) {
		if (dev)
			_bt_update_genlist_item((Elm_Object_Item *)
						dev->genlist_item);
	}

	EINA_LIST_FOREACH(ugd->searched_device, l, dev) {
		if (dev)
			_bt_update_genlist_item((Elm_Object_Item *)
						dev->genlist_item);
	}
}

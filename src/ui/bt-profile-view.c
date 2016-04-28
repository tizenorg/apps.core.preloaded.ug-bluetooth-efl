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

#include <glib.h>
#include <bluetooth.h>
#include <Elementary.h>
#include <Ecore_IMF.h>
#include <efl_extension.h>
#include <notification.h>

#include "bt-main-ug.h"
#include "bt-main-view.h"
#include "bt-debug.h"
#include "bt-widget.h"
#include "bt-type-define.h"
#include "bt-string-define.h"
#include "bt-profile-view.h"
#include "bt-ipc-handler.h"
#include "bt-util.h"
#include "bt-net-connection.h"
#include "bluetooth_internal.h"

/**********************************************************************
*                                      Static Functions declaration
***********************************************************************/
int __bt_profile_disconnect_option(bt_ug_data *ugd, bt_dev_t *dev,
					bt_device_type type);

int __bt_profile_connect_option(bt_ug_data *ugd, bt_dev_t *dev,
				bt_device_type type);

/**********************************************************************
*                                               Static Functions
***********************************************************************/
static void __bt_toast_popup_timeout_cb(void *data,
				Evas_Object *obj, void *event_info)
{
	bt_ug_data *ugd = (bt_ug_data *)data;
	evas_object_del(obj);
	if (ugd->rename_entry)
		elm_object_focus_set(ugd->rename_entry, EINA_TRUE);
}

void __bt_profile_input_panel_state_cb(void *data, Ecore_IMF_Context *ctx, int value)
{
	FN_START;

	bt_dev_t *dev;
	bt_ug_data *ugd;
	bt_profile_view_data *vd;

	dev = (bt_dev_t *)data;
	ret_if(dev == NULL);
	ret_if(dev->ugd == NULL);

	ugd = dev->ugd;
	ret_if(ugd->profile_vd == NULL);

	vd = ugd->profile_vd;
	ret_if(vd->navi_it == NULL);

	FN_END;
}

static char *__bt_profile_name_label_get(void *data, Evas_Object *obj, const char *part)
{
	FN_START;

	char *name = NULL;
	bt_dev_t *dev = (bt_dev_t *)data;

	if (!strcmp("elm.text.sub", part))
		return strdup(BT_STR_DEVICE_NAME);
	else if (!strcmp("elm.text", part)) {
		name = elm_entry_utf8_to_markup(dev->name);
		if (name)
			return name;
		else
			return strdup(dev->name);
	}

	FN_END;
	return NULL;
}

static void __bt_profile_rename_device_entry_changed_cb(void *data, Evas_Object *obj,
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
		elm_object_disabled_set(ugd->rename_button, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(obj,
								EINA_TRUE);
	} else {
		if (elm_object_disabled_get(ugd->rename_button))
			elm_object_disabled_set(ugd->rename_button, EINA_FALSE);
		if (elm_entry_input_panel_return_key_disabled_get(obj))
			elm_entry_input_panel_return_key_disabled_set(
								obj, EINA_FALSE);
	}

	if(input_str != NULL) {
		free(input_str);
		input_str = NULL;
	}

	FN_END;
}

static void __bt_profile_rename_device_cancel_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);
	ugd = (bt_ug_data *)data;

	if (ugd->rename_entry) {
		elm_entry_input_panel_hide(ugd->rename_entry);
		elm_object_focus_set(ugd->rename_entry, EINA_FALSE);
	}

	if (ugd->rename_popup != NULL) {
		evas_object_del(ugd->rename_popup);
		ugd->rename_popup = NULL;
		ugd->rename_entry = NULL;
	}

	FN_END;
}

static void __bt_profile_rename_device_ok_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	FN_START;
	bt_dev_t *dev = (bt_dev_t *)data;
	ret_if(!dev);
	bt_ug_data *ugd = (bt_ug_data *)dev->ugd;
	ret_if(!ugd);
	char *str = NULL;
	bt_dev_t *temp = NULL;
	Eina_List *l = NULL;
	bool flag = true;
	char msg[BT_STR_ACCES_INFO_MAX_LEN] = {0, };
	Evas_Object *popup = NULL;

	const char *entry_str = elm_entry_entry_get(ugd->rename_entry);
	char *device_name_str = NULL;

	if (ugd->rename_entry) {
		elm_entry_input_panel_hide(ugd->rename_entry);
		elm_object_focus_set(ugd->rename_entry, EINA_FALSE);
	}

	device_name_str = elm_entry_markup_to_utf8(entry_str);
	ret_if(!device_name_str);

	EINA_LIST_FOREACH(ugd->paired_device, l, temp) {
		if (temp)
			if (g_strcmp0(temp->name, device_name_str) == 0) {
				if(g_strcmp0(dev->name, device_name_str) != 0) {
					flag = false;
					break;
				}
			}
	}

	if (flag) {
		EINA_LIST_FOREACH(ugd->searched_device, l, temp) {
			if (temp)
				if (g_strcmp0(temp->name, device_name_str) == 0) {
					flag = false;
					break;
				}
			}
	}

	if (!flag){
		snprintf(msg, sizeof(msg), BT_STR_NAME_IN_USE);

		popup = elm_popup_add(ugd->win_main);
		elm_object_style_set(popup, "toast");
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
		elm_object_text_set(popup, msg);
		elm_popup_timeout_set(popup, 2.0);
		evas_object_smart_callback_add(popup, "timeout",
				__bt_toast_popup_timeout_cb, ugd);

		elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
		evas_object_show(popup);
	} else {
		BT_DBG("Device name:[%s]", device_name_str);

		str = g_strndup(device_name_str, DEVICE_NAME_MAX_LEN);
		if (BT_ERROR_NONE == bt_device_set_alias(dev->addr_str, str)) {
			memset(dev->name, 0, DEVICE_NAME_MAX_LEN + 1);
			g_strlcpy(dev->name, str, DEVICE_NAME_MAX_LEN + 1);
			free(str);

			if (ugd->profile_vd)
				_bt_update_genlist_item((Elm_Object_Item *)ugd->profile_vd->name_item);
			_bt_update_genlist_item((Elm_Object_Item *)dev->genlist_item);
		}

		evas_object_del(ugd->rename_popup);
		ugd->rename_popup = NULL;
		ugd->rename_entry = NULL;
	}
	g_free(device_name_str);
	FN_END;
}

static void __bt_profile_rename_entry_changed_cb(void *data, Evas_Object *obj,
				void *event_info)
{
	if (elm_object_part_content_get(obj, "elm.swallow.clear")) {
		if (elm_object_focus_get(obj)) {
			if (elm_entry_is_empty(obj))
				elm_object_signal_emit(obj, "elm,state,clear,hidden", "");
			else
				elm_object_signal_emit(obj, "elm,state,clear,visible", "");
		}
	}
	__bt_profile_rename_device_entry_changed_cb(data, obj, event_info);
}

static void __bt_profile_entry_edit_mode_show_cb(void *data, Evas *e, Evas_Object *obj,
                void *event_info)
{
	evas_object_event_callback_del(obj, EVAS_CALLBACK_SHOW,
							__bt_profile_entry_edit_mode_show_cb);

	elm_object_focus_set(obj, EINA_TRUE);
	elm_entry_cursor_end_set(obj);
}

static void __bt_profile_popup_entry_activated_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!obj)
		return;

	elm_object_focus_set(obj, EINA_FALSE);
}

static Evas_Object *__bt_profile_rename_entry_icon_get(
				void *data, Evas_Object *obj, const char *part)
{
	FN_START;
	retv_if (obj == NULL || data == NULL, NULL);

	bt_dev_t *dev = data;
	Evas_Object *entry = NULL;
	char *name_value = NULL;
	bt_ug_data *ugd;
	bt_profile_view_data *vd;
	static Elm_Entry_Filter_Limit_Size limit_filter_data;

	dev = (bt_dev_t *)data;
	retv_if(dev == NULL, NULL);

	ugd = (bt_ug_data *)dev->ugd;
	retv_if(ugd == NULL, NULL);

	vd = ugd->profile_vd;
	retv_if(vd == NULL, NULL);

	if (!strcmp(part, "elm.swallow.content")) {
		Evas_Object *layout = NULL;

		layout = elm_layout_add(obj);
		elm_layout_theme_set(layout, "layout", "editfield", "singleline");
		evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, 0.0);
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, 0.0);

		name_value = elm_entry_utf8_to_markup(dev->name);

		entry = elm_entry_add(layout);
		elm_entry_single_line_set(entry, EINA_TRUE);
		elm_entry_scrollable_set(entry, EINA_TRUE);

		evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.0);
		evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0.0);

		eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);
		elm_object_signal_emit(entry, "elm,action,hide,search_icon", "");
		elm_object_part_text_set(entry, "guide", BT_STR_DEVICE_NAME);
		elm_object_text_set(entry, name_value);
		elm_entry_input_panel_imdata_set(entry, "action=disable_emoticons", 24);

		elm_entry_input_panel_return_key_type_set(entry, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
		limit_filter_data.max_char_count = DEVICE_NAME_MAX_CHARACTER;
		elm_entry_markup_filter_append(entry,
			elm_entry_filter_limit_size, &limit_filter_data);

		elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);

		evas_object_smart_callback_add(entry, "maxlength,reached",
				_bt_util_max_len_reached_cb, ugd);
		evas_object_smart_callback_add(entry, "changed",
				__bt_profile_rename_entry_changed_cb, ugd);
		evas_object_smart_callback_add(entry, "preedit,changed",
				__bt_profile_rename_entry_changed_cb, ugd);
		evas_object_smart_callback_add(entry, "activated",
				__bt_profile_popup_entry_activated_cb, NULL);

		evas_object_event_callback_add(entry, EVAS_CALLBACK_SHOW,
				__bt_profile_entry_edit_mode_show_cb, ugd);

		elm_entry_input_panel_show(entry);
		elm_object_part_content_set(layout, "elm.swallow.content", entry);

		ugd->rename_entry = entry;

		if (name_value)
			free(name_value);

		return layout;
	}

	return NULL;
}

static void __bt_profile_name_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	bt_profile_view_data *vd;

	if (event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
					      EINA_FALSE);
	bt_ug_data *ugd = (bt_ug_data *)data;
	ret_if(ugd == NULL);

	vd = ugd->profile_vd;
	ret_if(vd == NULL);

	bt_dev_t *dev = elm_object_item_data_get((Elm_Object_Item *)event_info);
	ret_if(dev == NULL);
	Evas_Object *popup = NULL;
	Evas_Object *button = NULL;
	Evas_Object *genlist = NULL;

/*
	char *name_value = NULL;

	name_value = elm_entry_utf8_to_markup(dev->name);
*/
	popup = elm_popup_add(ugd->base);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
/*
	layout = elm_layout_add(popup);

	elm_layout_file_set(layout, BT_GENLIST_EDJ, "profile_rename_device_ly");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
*/
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
	vd->rename_entry_itc = elm_genlist_item_class_new();
	if (vd->rename_entry_itc) {
		vd->rename_entry_itc->item_style = BT_GENLIST_FULL_CONTENT_STYLE;
		vd->rename_entry_itc->func.text_get = NULL;
		vd->rename_entry_itc->func.content_get = __bt_profile_rename_entry_icon_get;
		vd->rename_entry_itc->func.state_get = NULL;
		vd->rename_entry_itc->func.del = NULL;

		vd->rename_entry_item = elm_genlist_item_append(genlist,
				vd->rename_entry_itc, dev,
				NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	}
/*
	entry = elm_entry_add(obj);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);

	eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_object_signal_emit(entry, "elm,action,hide,search_icon", "");
	elm_object_part_text_set(entry, "elm.guide", BT_STR_DEVICE_NAME);
	elm_entry_input_panel_imdata_set(entry, "action=disable_emoticons", 24);

	limit_filter.max_char_count = DEVICE_NAME_MAX_CHARACTER;
	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
			&limit_filter);

	elm_entry_input_panel_return_key_type_set(entry,
			ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE);

	if (name_value)
		elm_entry_entry_set(entry, name_value);

	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);

	evas_object_smart_callback_add(entry, "maxlength,reached",
			_bt_util_max_len_reached_cb, ugd);
	evas_object_smart_callback_add(entry, "changed",
			__bt_profile_rename_entry_changed_cb, ugd);
	evas_object_smart_callback_add(entry, "preedit,changed",
			__bt_profile_rename_entry_changed_cb, ugd);
	evas_object_smart_callback_add(entry, "focused",
			__bt_profile_rename_entry_focused_cb, NULL);
	evas_object_event_callback_add(entry, EVAS_CALLBACK_KEY_DOWN,
			__bt_profile_rename_entry_keydown_cb, ugd);

	evas_object_show(entry);
	elm_object_part_content_set(layout, "elm.swallow.layout", entry);

	elm_object_content_set(popup, layout);
*/
	button = elm_button_add(popup);
	elm_object_style_set(button, "popup");
	elm_object_domain_translatable_text_set(button,
						PKGNAME,
						"IDS_BR_SK_CANCEL");
	elm_object_part_content_set(popup, "button1", button);
	evas_object_smart_callback_add(button, "clicked",
			__bt_profile_rename_device_cancel_cb, ugd);

	button = elm_button_add(popup);
	ugd->rename_button = button;
	elm_object_style_set(button, "popup");
	elm_object_domain_translatable_text_set(button,
						PKGNAME,
						"IDS_BT_OPT_RENAME");
	elm_object_part_content_set(popup, "button2", button);
	evas_object_smart_callback_add(button, "clicked",
			__bt_profile_rename_device_ok_cb, dev);

	evas_object_show(genlist);

	elm_object_content_set(popup, genlist);
	evas_object_show(popup);

	if (ugd->rename_entry)
		elm_object_focus_set(ugd->rename_entry, EINA_TRUE);

	ugd->rename_popup = popup;

	FN_END;
}

static void __bt_profile_unpair_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd;

	ret_if(NULL == data);

	dev = (bt_dev_t *)data;

	ugd = dev->ugd;
	ret_if(NULL == ugd);

	if (event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
					      EINA_FALSE);

	/* If rename entry is focused, then unfocus and return */
	if (ugd->rename_entry && elm_object_focus_get(ugd->rename_entry)) {
		BT_DBG("Unfocus Rename Entry");
		elm_object_focus_set(ugd->rename_entry, EINA_FALSE);
		return;
	}

	if (dev->status == BT_DEV_UNPAIRING)
		return;

	dev->status = BT_DEV_UNPAIRING;
	if (bt_device_destroy_bond(dev->addr_str) != BT_ERROR_NONE) {
		BT_ERR("Fail to unpair");
		dev->status = BT_IDLE;
		return;
	}

	FN_END;
	return;
}

static char *__bt_profile_unpair_label_get(void *data, Evas_Object *obj,
					    const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	if (!strcmp("elm.text", part)) {
		g_strlcpy(buf, BT_STR_UNPAIR, BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_ERR("empty text for label");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_proflie_title_label_get(void *data, Evas_Object *obj,
						const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	if (!strcmp("elm.text", part)) {
		/*Label */
		g_strlcpy(buf, BT_STR_CONNECTION_OPTIONS,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_ERR("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

#ifndef TELEPHONY_DISABLED
static char *__bt_proflie_call_option_label_get(void *data, Evas_Object *obj,
						const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
#ifdef KIRAN_ACCESSIBILITY
	char str[BT_STR_ACCES_INFO_MAX_LEN] = {0, };
	Evas_Object *ao;
#endif
	bt_dev_t *dev_info = NULL;
	bt_ug_data *ugd = NULL;

	retv_if(NULL == data, NULL);
	dev_info = (bt_dev_t *)data;

	ugd = (bt_ug_data *)(dev_info->ugd);
	retv_if(NULL == ugd, NULL);

	if (!strcmp("elm.text", part)) {
		g_strlcpy(buf, BT_STR_CALL_AUDIO,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_ERR("This part name is not exist in style");
		return NULL;
	}

#ifdef KIRAN_ACCESSIBILITY
	if (ugd->profile_vd && ugd->profile_vd->call_item) {
		ao = elm_object_item_access_object_get(
				ugd->profile_vd->call_item);
		if (dev_info->status == BT_IDLE) {
			if (dev_info->call_checked)
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_CALL_AUDIO,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_SELECTED);
			else
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_CALL_AUDIO,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_UNSELECTED);
		}
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
	}
#endif
	FN_END;
	return strdup(buf);
}
#endif

static char *__bt_proflie_media_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
#ifdef KIRAN_ACCESSIBILITY
	char str[BT_STR_ACCES_INFO_MAX_LEN] = {0, };
	Evas_Object *ao;
#endif
	bt_dev_t *dev_info = NULL;
	bt_ug_data *ugd = NULL;

	retv_if(NULL == data, NULL);
	dev_info = (bt_dev_t *)data;

	ugd = (bt_ug_data *)(dev_info->ugd);
	retv_if(NULL == ugd, NULL);

	if (!strcmp("elm.text", part)) {
		g_strlcpy(buf, BT_STR_MEDIA_AUDIO,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_ERR("This part name is not exist in style");
		return NULL;
	}

#ifdef KIRAN_ACCESSIBILITY
	if (ugd->profile_vd && ugd->profile_vd->media_item) {
		ao = elm_object_item_access_object_get(
				ugd->profile_vd->media_item);
		if (dev_info->status == BT_IDLE) {
			if (dev_info->media_checked)
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_MEDIA_AUDIO,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_SELECTED);
			else
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_MEDIA_AUDIO,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_UNSELECTED);
		}
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
	}
#endif
	FN_END;
	return strdup(buf);
}

static char *__bt_proflie_hid_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
#ifdef KIRAN_ACCESSIBILITY
	char str[BT_STR_ACCES_INFO_MAX_LEN] = {0, };
	Evas_Object *ao;
#endif
	bt_dev_t *dev_info = NULL;
	bt_ug_data *ugd = NULL;

	retv_if(NULL == data, NULL);
	dev_info = (bt_dev_t *)data;

	ugd = (bt_ug_data *)(dev_info->ugd);
	retv_if(NULL == ugd, NULL);

	if (!strcmp("elm.text", part)) {
		g_strlcpy(buf, BT_STR_INPUT_DEVICE,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_ERR("This part name is not exist in style");
		return NULL;
	}
#ifdef KIRAN_ACCESSIBILITY
	if (ugd->profile_vd && ugd->profile_vd->hid_item) {
		ao = elm_object_item_access_object_get(
				ugd->profile_vd->hid_item);
		if (dev_info->status == BT_IDLE) {
			if (dev_info->hid_checked)
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_INPUT_DEVICE,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_SELECTED);
			else
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_INPUT_DEVICE,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_UNSELECTED);
		}
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
	}
#endif
	FN_END;
	return strdup(buf);
}

static char *__bt_proflie_nap_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };
#ifdef KIRAN_ACCESSIBILITY
	char str[BT_STR_ACCES_INFO_MAX_LEN] = {0, };
	Evas_Object *ao;
#endif
	bt_dev_t *dev_info = NULL;
	bt_ug_data *ugd = NULL;

	retv_if(NULL == data, NULL);
	dev_info = (bt_dev_t *)data;

	ugd = (bt_ug_data *)(dev_info->ugd);
	retv_if(NULL == ugd, NULL);

	if (!strcmp("elm.text", part)) {
		g_strlcpy(buf, BT_STR_INTERNET_ACCESS,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_ERR("This part name is not exist in style");
		return NULL;
	}

#ifdef KIRAN_ACCESSIBILITY
	if (ugd->profile_vd && ugd->profile_vd->network_item) {
		ao = elm_object_item_access_object_get(
				ugd->profile_vd->network_item);
		if (dev_info->status == BT_IDLE) {
			if (dev_info->network_checked)
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_INTERNET_ACCESS,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_SELECTED);
			else
				snprintf(str, sizeof(str), "%s, %s, %s",
						BT_STR_INTERNET_ACCESS,
						BT_STR_RADIO_BUTTON,
						BT_STR_RADIO_UNSELECTED);
		}
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
	}
#endif
	FN_END;
	return strdup(buf);
}

#ifndef TELEPHONY_DISABLED
static void __bt_profile_call_option_checkbox_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;
	bt_dev_t *dev = NULL;
	int ret;

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;

	if (dev->connected_mask & BT_HEADSET_CONNECTED) {
		ret = __bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_HEADSET_DEVICE);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_HEADSET_DEVICE);
	}

	if (ret == BT_UG_FAIL)
		elm_check_state_set(obj, dev->call_checked);

	FN_END;
}
#endif

static void __bt_profile_media_option_checkbox_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;
	bt_dev_t *dev = NULL;
	int ret;

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;

#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	if (dev->connected_mask & BT_MUSIC_PLAYER_CONNECTED) {
		ret = __bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_MUSIC_PLAYER_DEVICE);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
					dev, BT_MUSIC_PLAYER_DEVICE);
	}
#else
	if (dev->connected_mask & BT_STEREO_HEADSET_CONNECTED) {
		ret = __bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_STEREO_HEADSET_DEVICE);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
					dev, BT_STEREO_HEADSET_DEVICE);
	}
#endif

	if (ret == BT_UG_FAIL)
		elm_check_state_set(obj, dev->media_checked);

	FN_END;
}


static void __bt_profile_hid_option_checkbox_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;
	bt_dev_t *dev = NULL;
	int ret;

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;

	if (dev->connected_mask & BT_HID_CONNECTED) {
		ret = __bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_HID_DEVICE);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_HID_DEVICE);
	}

	if (ret == BT_UG_FAIL)
		elm_check_state_set(obj, dev->hid_checked);

	FN_END;
}

static void __bt_profile_nap_option_checkbox_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;
	bt_dev_t *dev = NULL;
	int ret;

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;

	if (dev->connected_mask & BT_NETWORK_CONNECTED) {
		ret = __bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_NETWORK_DEVICE);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_NETWORK_DEVICE);
	}

	if (ret == BT_UG_FAIL)
		elm_check_state_set(obj, dev->network_checked);

	FN_END;

}

#ifndef TELEPHONY_DISABLED
static Evas_Object *__bt_profile_call_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp("elm.swallow.end", part)) {
		check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");

		dev->call_checked = dev->connected_mask & \
					BT_HEADSET_CONNECTED;
		elm_check_state_set(check, dev->call_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);

		evas_object_smart_callback_add(check, "changed", __bt_profile_call_option_checkbox_sel, dev);

		evas_object_propagate_events_set(check, EINA_FALSE);
		evas_object_show(check);
	}
	FN_END;
	return check;
}
#endif

static Evas_Object *__bt_profile_media_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp("elm.swallow.end", part)) {
		check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
		dev->media_checked = dev->connected_mask & \
					BT_MUSIC_PLAYER_CONNECTED;
#else
		dev->media_checked = dev->connected_mask & \
					BT_STEREO_HEADSET_CONNECTED;
#endif
		elm_check_state_set(check, dev->media_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);

		evas_object_smart_callback_add(check, "changed", __bt_profile_media_option_checkbox_sel, dev);

		evas_object_propagate_events_set(check, EINA_FALSE);
		evas_object_show(check);
	}
	FN_END;
	return check;
}

static Evas_Object *__bt_profile_hid_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp("elm.swallow.end", part)) {
		check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");

		dev->hid_checked = dev->connected_mask & \
					BT_HID_CONNECTED;
		elm_check_state_set(check, dev->hid_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);

		evas_object_smart_callback_add(check, "changed", __bt_profile_hid_option_checkbox_sel, dev);

		evas_object_propagate_events_set(check, EINA_FALSE);
		evas_object_show(check);
	}
	FN_END;
	return check;
}

static Evas_Object *__bt_profile_nap_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp("elm.swallow.end", part)) {
		check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");

		dev->network_checked = dev->connected_mask & \
					BT_NETWORK_CONNECTED;
		elm_check_state_set(check, dev->network_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);

		evas_object_smart_callback_add(check, "changed", __bt_profile_nap_option_checkbox_sel, dev);

		evas_object_propagate_events_set(check, EINA_FALSE);
		evas_object_show(check);
	}
	FN_END;
	return check;
}

int __bt_profile_connect_option(bt_ug_data *ugd, bt_dev_t *dev,
				bt_device_type type)
{
	FN_START;

	int audio_profile;

	retv_if(ugd == NULL, BT_UG_FAIL);
	retv_if(dev == NULL, BT_UG_FAIL);

	if (type == BT_HEADSET_DEVICE || type == BT_STEREO_HEADSET_DEVICE ||
			type == BT_MUSIC_PLAYER_DEVICE) {
		if (type == BT_STEREO_HEADSET_DEVICE)
			audio_profile = BT_AUDIO_PROFILE_TYPE_A2DP;
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
		else if (type == BT_MUSIC_PLAYER_DEVICE)
			audio_profile = BT_AUDIO_PROFILE_TYPE_A2DP_SINK;
#endif
		else
			audio_profile = BT_AUDIO_PROFILE_TYPE_HSP_HFP;

		if (bt_audio_connect(dev->addr_str,
				audio_profile) != BT_ERROR_NONE) {
			BT_ERR("Fail to connect Headset device");
			return BT_UG_FAIL;
		}
	} else if (type == BT_HID_DEVICE) {
#ifdef TIZEN_HID

		BT_DBG("HID connect request");

		if (bt_hid_host_connect(dev->addr_str) != BT_ERROR_NONE) {
			return BT_UG_FAIL;
		}
#endif
	} else if (type == BT_NETWORK_DEVICE){
		BT_DBG("Network connect request");

		void *net_profile;

		if (dev->net_profile) {
			if (_bt_connect_net_profile(ugd->connection,
						dev->net_profile,
						dev) != BT_UG_ERROR_NONE) {
				BT_ERR("Fail to connect the net profile");
				return BT_UG_FAIL;
			}
		} else {
			net_profile =
				_bt_get_registered_net_profile(ugd->connection,
								dev->bd_addr);
			if (net_profile) {
				_bt_set_profile_state_changed_cb(net_profile, dev);

				if (_bt_connect_net_profile(ugd->connection,
							net_profile,
							dev) != BT_UG_ERROR_NONE) {
					BT_ERR("Fail to connect the net profile");
					return BT_UG_FAIL;
				}
			} else {
				BT_ERR("No registered net profile");
				return BT_UG_FAIL;
			}
		}
	} else {
		BT_ERR("Unknown type");
		return BT_UG_FAIL;
	}

	ugd->connect_req = TRUE;
	dev->status = BT_CONNECTING;
	_bt_update_genlist_item((Elm_Object_Item *)dev->genlist_item);

	retv_if(ugd->profile_vd == NULL, BT_UG_FAIL);

	if (ugd->profile_vd->genlist) {
		_bt_util_set_list_disabled(ugd->profile_vd->genlist,
					EINA_TRUE);
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

int __bt_profile_disconnect_option(bt_ug_data *ugd, bt_dev_t *dev,
					bt_device_type type)
{
	FN_START;

	int audio_profile;
	bt_ug_ipc_param_t param;
	gboolean connected = FALSE;
	gboolean connected_nap_profile = FALSE;

	retv_if(ugd == NULL, BT_UG_FAIL);
	retv_if(dev == NULL, BT_UG_FAIL);

	memset(&param, 0x00, sizeof(bt_ug_ipc_param_t));
	memcpy(param.param2, dev->bd_addr, BT_ADDRESS_LENGTH_MAX);

	if (type == BT_HEADSET_DEVICE) {
		connected = _bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
						dev->bd_addr);
	} else if (type == BT_STEREO_HEADSET_DEVICE) {
		connected = _bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
						dev->bd_addr);
	} else if (type == BT_MUSIC_PLAYER_DEVICE) {
		connected = _bt_util_is_profile_connected(BT_MUSIC_PLAYER_CONNECTED,
						dev->bd_addr);
	} else if (type == BT_HID_DEVICE) {
		connected = _bt_util_is_profile_connected(BT_HID_CONNECTED,
						dev->bd_addr);
	} else if (type == BT_NETWORK_DEVICE) {
		connected = _bt_util_is_profile_connected(BT_NETWORK_CONNECTED,
						dev->bd_addr);
		/* Need to check */
		if (!connected) {
			connected = _bt_util_is_profile_connected(BT_NETWORK_SERVER_CONNECTED,
						dev->bd_addr);
			connected_nap_profile = connected;
		}
	}

	if (connected == FALSE) {
		BT_ERR("Not connected");
		return BT_UG_FAIL;
	}

	if (type == BT_HEADSET_DEVICE || type == BT_STEREO_HEADSET_DEVICE ||
			type == BT_MUSIC_PLAYER_DEVICE) {
		if (type == BT_STEREO_HEADSET_DEVICE)
			audio_profile = BT_AUDIO_PROFILE_TYPE_A2DP;
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
		else if (type == BT_MUSIC_PLAYER_DEVICE)
			audio_profile = BT_AUDIO_PROFILE_TYPE_A2DP_SINK;
#endif
		else
			audio_profile = BT_AUDIO_PROFILE_TYPE_HSP_HFP;

		if (bt_audio_disconnect(dev->addr_str,
				audio_profile) != BT_ERROR_NONE) {
			BT_ERR("Fail to connect Headset device");
			return BT_UG_FAIL;
		}
	} else if (type == BT_HID_DEVICE) {
		BT_DBG("Disconnecting HID service!!");

		if (bt_hid_host_disconnect(dev->addr_str) != BT_ERROR_NONE) {
			BT_ERR("Fail to disconnect HID device");
			return BT_UG_FAIL;
		}
	} else if (type == BT_NETWORK_DEVICE) {

		void *net_profile;

		if (connected_nap_profile == FALSE) {
			BT_DBG("Disconnecting network service!! [PANU]");
			if (dev->net_profile) {
				if (_bt_disconnect_net_profile(ugd->connection,
							dev->net_profile,
							dev) != BT_UG_ERROR_NONE) {
					BT_ERR("Fail to disconnect the net profile");
					return BT_UG_FAIL;
				}
			} else {
				net_profile =
					_bt_get_registered_net_profile(ugd->connection,
									dev->bd_addr);
				if (net_profile) {
					_bt_set_profile_state_changed_cb(net_profile, dev);
					if (_bt_disconnect_net_profile(ugd->connection,
								dev->net_profile,
								dev) != BT_UG_ERROR_NONE) {
						BT_ERR("Fail to disconnect the net profile");
						return BT_UG_FAIL;
					}
				} else {
					BT_ERR("No registered net profile");
					return BT_UG_FAIL;
				}
			}
		} else {
			BT_DBG("Disconnecting network service!! [NAP]");
			if (bt_nap_disconnect(dev->addr_str) == BT_UG_ERROR_NONE)
				BT_ERR("Failed to disconnect pan server");
		}

	} else {
		BT_ERR("Unknown type");
		return BT_UG_FAIL;
	}

	ugd->connect_req = TRUE;
	dev->status = BT_DISCONNECTING;
	_bt_update_genlist_item((Elm_Object_Item *)dev->genlist_item);

	if (ugd->profile_vd->genlist) {
		_bt_util_set_list_disabled(ugd->profile_vd->genlist,
					EINA_TRUE);
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

#ifndef TELEPHONY_DISABLED
static void __bt_profile_call_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;

	ret_if(event_info == NULL);

	if(event_info) {
		item = (Elm_Object_Item *)event_info;
		elm_genlist_item_selected_set(item, EINA_FALSE);
	}

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;

	if (dev->connected_mask & BT_HEADSET_CONNECTED) {
		__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_HEADSET_DEVICE);
	} else {
		__bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_HEADSET_DEVICE);
	}
	_bt_update_genlist_item(item);

	FN_END;
}
#endif

static void __bt_profile_media_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;

	ret_if(event_info == NULL);

	if(event_info) {
		item = (Elm_Object_Item *)event_info;
		elm_genlist_item_selected_set(item, EINA_FALSE);
	}

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	if (dev->connected_mask & BT_MUSIC_PLAYER_CONNECTED) {
		__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_MUSIC_PLAYER_DEVICE);
	} else {
		__bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_MUSIC_PLAYER_DEVICE);
	}
#else
	if (dev->connected_mask & BT_STEREO_HEADSET_CONNECTED) {
		__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_STEREO_HEADSET_DEVICE);
	} else {
		__bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_STEREO_HEADSET_DEVICE);
	}
#endif

	_bt_update_genlist_item(item);

	FN_END;
}

#ifdef TIZEN_HID
static void __bt_profile_hid_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;

	ret_if(event_info == NULL);

	if(event_info) {
		item = (Elm_Object_Item *)event_info;
		elm_genlist_item_selected_set(item, EINA_FALSE);
	}

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;

	if (dev->connected_mask & BT_HID_CONNECTED) {
		__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_HID_DEVICE);
	} else {
		__bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_HID_DEVICE);
	}
	_bt_update_genlist_item(item);

	FN_END;
}
#endif

static void __bt_profile_nap_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Elm_Object_Item *item = NULL;

	ret_if(event_info == NULL);

	if(event_info) {
		item = (Elm_Object_Item *)event_info;
		elm_genlist_item_selected_set(item, EINA_FALSE);
	}

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	if (dev->status == BT_DEV_UNPAIRING)
		return;

	if (dev->connected_mask & BT_NETWORK_CONNECTED) {
		__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
					dev, BT_NETWORK_DEVICE);
	} else {
		__bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_NETWORK_DEVICE);
	}

	_bt_update_genlist_item(item);

	FN_END;
}

void _bt_update_detail_item_style(bt_profile_view_data *vd)
{
	FN_START;

	int item_count;
	int i = 1;
	Elm_Object_Item *item = NULL;

	ret_if(vd == NULL);
	ret_if(vd->genlist == NULL);

	item_count = elm_genlist_items_count(vd->genlist);

	BT_INFO("item_count %d", item_count);

	if (vd->title_item == NULL || item_count < 4)
		return;
	/* Do not need to take care first 4 items as they are fixed */
	item_count = item_count - 5;

	item = elm_genlist_item_next_get(vd->title_item);

	while (item != NULL) {
		if (item_count == 1) {
			elm_object_item_signal_emit(item, "elm,state,normal", "");
			break;
		} else if (i == 1) {
			elm_object_item_signal_emit(item, "elm,state,top", "");
			i++;
		} else if (item_count == i) {
			elm_object_item_signal_emit(item, "elm,state,bottom", "");
			break;
		} else {
			elm_object_item_signal_emit(item, "elm,state,center", "");
			i++;
		}

		item = elm_genlist_item_next_get(item);
	}

	FN_END;
}

static int __bt_profile_get_item_type(bt_profile_view_data *vd, Elm_Object_Item *item)
{
	retv_if(vd == NULL, BT_ITEM_NONE);
	retv_if(item == NULL, BT_ITEM_NONE);

	if (item == vd->name_item) {
		return BT_ITEM_NAME;
	} else if (item == vd->unpair_item) {
		return BT_ITEM_UNPAIR;
#ifndef TELEPHONY_DISABLED
	} else if (item == vd->call_item) {
		return BT_ITEM_CALL;
#endif
	} else if (item == vd->media_item) {
		return BT_ITEM_MEDIA;
	} else if (item == vd->hid_item) {
		return BT_ITEM_HID;
	} else if (item == vd->network_item) {
		return BT_ITEM_NETWORK;
	}

	return BT_ITEM_NONE;
}

static void __bt_profile_gl_realized(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;

	int item_type;
	bt_ug_data *ugd;
	bt_profile_view_data *vd;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
#ifdef KIRAN_ACCESSIBILITY
	Evas_Object *ao;
	char str[BT_STR_ACCES_INFO_MAX_LEN] = {0, };
	bt_dev_t *dev_info;
#endif

	ret_if(data == NULL);
	ret_if(item == NULL);

	ugd = (bt_ug_data *)data;

	vd = ugd->profile_vd;
	ret_if(vd == NULL);

	item_type = __bt_profile_get_item_type(vd, item);

	BT_INFO("type: %d", item_type);
#ifdef KIRAN_ACCESSIBILITY
	dev_info = (bt_dev_t *)elm_object_item_data_get(item);
	ao = elm_object_item_access_object_get(item);
#endif
	switch (item_type) {
	case BT_ITEM_NAME:
		elm_object_item_signal_emit(item, "elm,state,top", "");

#ifdef KIRAN_ACCESSIBILITY
		if (dev_info != NULL)
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_DEVICE_NAME,
				dev_info->name, BT_STR_DOUBLE_TAP_RENAME);
		else
			snprintf(str, sizeof(str), "%s, %s", BT_STR_DEVICE_NAME,
				BT_STR_DOUBLE_TAP_RENAME);

		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		break;
	case BT_ITEM_UNPAIR:
		elm_object_item_signal_emit(item, "elm,state,bottom", "");

#ifdef KIRAN_ACCESSIBILITY
		snprintf(str, sizeof(str), "%s, %s", BT_STR_UNPAIR,
				BT_STR_DOUBLE_TAP_UNPAIR);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		break;

#ifndef TELEPHONY_DISABLED
	case BT_ITEM_CALL:
#ifdef KIRAN_ACCESSIBILITY
		if (dev_info->call_checked)
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_CALL_AUDIO,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_SELECTED);
		else
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_CALL_AUDIO,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_UNSELECTED);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		break;
#endif
	case BT_ITEM_MEDIA:
#ifdef KIRAN_ACCESSIBILITY
		if (dev_info->media_checked)
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_MEDIA_AUDIO,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_SELECTED);
		else
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_MEDIA_AUDIO,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_UNSELECTED);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		break;

	case BT_ITEM_HID:
#ifdef KIRAN_ACCESSIBILITY
		if (dev_info->hid_checked)
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_INPUT_DEVICE,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_SELECTED);
		else
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_INPUT_DEVICE,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_UNSELECTED);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		break;

	case BT_ITEM_NETWORK:
#ifdef KIRAN_ACCESSIBILITY
		if (dev_info->network_checked)
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_INTERNET_ACCESS,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_SELECTED);
		else
			snprintf(str, sizeof(str), "%s, %s, %s", BT_STR_INTERNET_ACCESS,
				BT_STR_RADIO_BUTTON, BT_STR_RADIO_UNSELECTED);
		elm_access_info_set(ao, ELM_ACCESS_INFO, str);
#endif
		break;

	case BT_ITEM_NONE:
	default:
		break;
	}

	_bt_update_detail_item_style(vd);
	FN_END;
}

/* Create genlist and append items */
static Evas_Object *__bt_profile_draw_genlist(bt_ug_data *ugd, bt_dev_t *dev_info)
{
	FN_START;

	bt_profile_view_data *vd = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;

	retv_if(ugd == NULL, NULL);
	retv_if(ugd->profile_vd == NULL, NULL);

	vd = ugd->profile_vd;

	/* Set item class for dialogue normal items */
	vd->name_itc = elm_genlist_item_class_new();
	retv_if (vd->name_itc == NULL, NULL);
	vd->name_itc->item_style = BT_GENLIST_2LINE_BOTTOM_TEXT_STYLE;
	vd->name_itc->func.text_get = __bt_profile_name_label_get;
	vd->name_itc->func.content_get = NULL;
	vd->name_itc->func.state_get = NULL;
	vd->name_itc->func.del = NULL;

	vd->unpair_itc = elm_genlist_item_class_new();
	retv_if (vd->unpair_itc == NULL, NULL);

	vd->unpair_itc->item_style = BT_GENLIST_1LINE_TEXT_STYLE;
	vd->unpair_itc->func.text_get = __bt_profile_unpair_label_get;
	vd->unpair_itc->func.content_get = NULL;
	vd->unpair_itc->func.state_get = NULL;
	vd->unpair_itc->func.del = NULL;

	/* Create genlist */
	genlist = elm_genlist_add(ugd->navi_bar);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_FALSE);
	elm_genlist_block_count_set(genlist, 3);

	evas_object_smart_callback_add(genlist, "realized",
				__bt_profile_gl_realized, ugd);

	/* device name item */
	git = elm_genlist_item_append(genlist, vd->name_itc, dev_info, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    __bt_profile_name_item_sel, ugd);
	vd->name_item = git;

	/* unpair item */
	git = elm_genlist_item_append(genlist, vd->unpair_itc, NULL, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    __bt_profile_unpair_item_sel, dev_info);
	vd->unpair_item = git;

	/* If the device has no headset profile, exit this function */
	if (!(dev_info->service_list & BT_SC_HFP_SERVICE_MASK) &&
		!(dev_info->service_list & BT_SC_HSP_SERVICE_MASK) &&
		!(dev_info->service_list & BT_SC_A2DP_SERVICE_MASK) &&
		!(dev_info->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK) &&
		!(dev_info->service_list & BT_SC_HID_SERVICE_MASK) &&
		!(dev_info->service_list & BT_SC_NAP_SERVICE_MASK)) {
		return genlist;
	}

	vd->title_itc = elm_genlist_item_class_new();
	retv_if (vd->title_itc == NULL, NULL);

	vd->title_itc->item_style = BT_GENLIST_GROUP_INDEX_STYLE;
	vd->title_itc->func.text_get = __bt_proflie_title_label_get;
	vd->title_itc->func.content_get = NULL;
	vd->title_itc->func.state_get = NULL;
	vd->title_itc->func.del = NULL;

#ifndef TELEPHONY_DISABLED
	vd->call_itc = elm_genlist_item_class_new();
	retv_if (vd->call_itc == NULL, NULL);

	vd->call_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	vd->call_itc->func.text_get = __bt_proflie_call_option_label_get;
	vd->call_itc->func.content_get = __bt_profile_call_option_icon_get;
	vd->call_itc->func.state_get = NULL;
	vd->call_itc->func.del = NULL;
#endif
	vd->media_itc = elm_genlist_item_class_new();
	retv_if (vd->media_itc == NULL, NULL);

	vd->media_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	vd->media_itc->func.text_get = __bt_proflie_media_option_label_get;
	vd->media_itc->func.content_get = __bt_profile_media_option_icon_get;
	vd->media_itc->func.state_get = NULL;
	vd->media_itc->func.del = NULL;

	vd->hid_itc = elm_genlist_item_class_new();
	retv_if (vd->hid_itc == NULL, NULL);

	vd->hid_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	vd->hid_itc->func.text_get = __bt_proflie_hid_option_label_get;
	vd->hid_itc->func.content_get = __bt_profile_hid_option_icon_get;
	vd->hid_itc->func.state_get = NULL;
	vd->hid_itc->func.del = NULL;

#ifndef TIZEN_BT_A2DP_SINK_ENABLE
	vd->network_itc = elm_genlist_item_class_new();
	retv_if (vd->network_itc == NULL, NULL);

	vd->network_itc->item_style = BT_GENLIST_1LINE_TEXT_ICON_STYLE;
	vd->network_itc->func.text_get = __bt_proflie_nap_option_label_get;
	vd->network_itc->func.content_get = __bt_profile_nap_option_icon_get;
	vd->network_itc->func.state_get = NULL;
	vd->network_itc->func.del = NULL;
#endif
	/* Connection options title */
	git = elm_genlist_item_append(genlist, vd->title_itc, NULL, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    NULL, NULL);

	elm_genlist_item_select_mode_set(git,
				ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	vd->title_item = git;

#ifndef TELEPHONY_DISABLED
	if (dev_info->service_list & BT_SC_HFP_SERVICE_MASK ||
	     dev_info->service_list & BT_SC_HSP_SERVICE_MASK) {
		/* Call audio */
		git = elm_genlist_item_append(genlist, vd->call_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_call_option_item_sel,
					dev_info);
		vd->call_item = git;
	}
#endif
#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	if (dev_info->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK) {
#else
	if (dev_info->service_list & BT_SC_A2DP_SERVICE_MASK) {
#endif
		/* Media audio */
		git = elm_genlist_item_append(genlist, vd->media_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_media_option_item_sel,
					dev_info);

		vd->media_item = git;
	}
	BT_INFO("service list: %x", dev_info->service_list);
	BT_INFO("is hid: %d", dev_info->service_list & BT_SC_HID_SERVICE_MASK);

	if (dev_info->service_list & BT_SC_HID_SERVICE_MASK) {
#ifdef TIZEN_HID
		/* HID device */
		git = elm_genlist_item_append(genlist, vd->hid_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_hid_option_item_sel,
					dev_info);
		vd->hid_item = git;
#else
		/* HID device */
		BT_INFO("HID options is disabled");
		git = elm_genlist_item_append(genlist, vd->hid_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					NULL,
					NULL);
		vd->hid_item = git;
		BT_INFO("HID item disabled");
		elm_object_item_disabled_set (vd->hid_item, EINA_TRUE);
#endif
	}

#ifndef TIZEN_BT_A2DP_SINK_ENABLE
	if (dev_info->service_list & BT_SC_NAP_SERVICE_MASK) {
		/* NAP device */
		git = elm_genlist_item_append(genlist, vd->network_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_nap_option_item_sel,
					dev_info);
		vd->network_item = git;
	}
#endif
	FN_END;

	return genlist;
}

void _bt_profile_destroy_profile_view(void *data)
{
	FN_START;

	bt_ug_data *ugd = (bt_ug_data *)data;
	bt_profile_view_data *vd = NULL;

	ret_if(ugd == NULL);
	ret_if(ugd->profile_vd == NULL);

	vd = ugd->profile_vd;

	if (vd->name_itc) {
		elm_genlist_item_class_free(vd->name_itc);
		vd->name_itc = NULL;
	}

	if (vd->unpair_itc) {
		elm_genlist_item_class_free(vd->unpair_itc);
		vd->unpair_itc = NULL;
	}

	if (vd->title_itc) {
		elm_genlist_item_class_free(vd->title_itc);
		vd->title_itc = NULL;
	}

#ifndef TELEPHONY_DISABLED
	if (vd->call_itc) {
		elm_genlist_item_class_free(vd->call_itc);
		vd->call_itc = NULL;
	}
#endif

	if (vd->media_itc) {
		elm_genlist_item_class_free(vd->media_itc);
		vd->media_itc = NULL;
	}

	if (vd->hid_itc) {
		elm_genlist_item_class_free(vd->hid_itc);
		vd->hid_itc = NULL;
	}
#ifndef TIZEN_BT_A2DP_SINK_ENABLE
	if (vd->network_itc) {
		elm_genlist_item_class_free(vd->network_itc);
		vd->network_itc = NULL;
	}
#endif
	vd->save_btn = NULL;

	/* unregister callback functions */
	if (vd->imf_context) {
		ecore_imf_context_input_panel_event_callback_del(vd->imf_context,
					ECORE_IMF_INPUT_PANEL_STATE_EVENT,
					__bt_profile_input_panel_state_cb);
		vd->imf_context = NULL;
	}

	free(vd);
	ugd->profile_vd = NULL;

	FN_END;
}

static Eina_Bool __bt_profile_back_clicked_cb(void *data, Elm_Object_Item *it)
{
	FN_START;
	retv_if(!data, EINA_TRUE);

	bt_ug_data *ugd = (bt_ug_data *)data;
	ugd->profile_vd = NULL;

	if (ugd->popup != NULL){
		evas_object_del(ugd->popup);
		ugd->popup = NULL;
		ugd->is_popup_exist = 0;
	}

	FN_END;
	return EINA_TRUE;
}

static void __bt_profile_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	_bt_profile_delete_view(data);
}

/**********************************************************************
*                                              Common Functions
***********************************************************************/

void _bt_profile_create_view(bt_dev_t *dev_info)
{
	FN_START;

	bt_profile_view_data *vd;
	bt_ug_data *ugd;
	Evas_Object *genlist;
	Elm_Object_Item *navi_it;
	Evas_Object *back_button = NULL;
	int connected;

	ret_if(dev_info == NULL);
	ret_if(dev_info->ugd == NULL);

	ugd = dev_info->ugd;

	vd = calloc(1, sizeof(bt_profile_view_data));
	ret_if(vd == NULL);

	ugd->profile_vd = vd;
	vd->win_main = ugd->win_main;
	vd->navi_bar = ugd->navi_bar;


	if (dev_info->service_list & BT_SC_HFP_SERVICE_MASK ||
	    dev_info->service_list & BT_SC_HSP_SERVICE_MASK ||
	    dev_info->service_list & BT_SC_A2DP_SERVICE_MASK) {
		connected = _bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
						     dev_info->bd_addr);
		dev_info->connected_mask |= connected ? BT_HEADSET_CONNECTED : 0x00;

		connected =
		    _bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
					     dev_info->bd_addr);
		dev_info->connected_mask |=
		    connected ? BT_STEREO_HEADSET_CONNECTED : 0x00;
	} else if (dev_info->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK ) {
		connected = _bt_util_is_profile_connected(BT_MUSIC_PLAYER_CONNECTED,
						     dev_info->bd_addr);
		dev_info->connected_mask |=
		    connected ? BT_MUSIC_PLAYER_CONNECTED : 0x00;
	} else if (dev_info->service_list & BT_SC_HID_SERVICE_MASK) {
		connected = _bt_util_is_profile_connected(BT_HID_CONNECTED,
						     dev_info->bd_addr);
		dev_info->connected_mask |= connected ? BT_HID_CONNECTED : 0x00;
	} else {
		connected = _bt_util_is_profile_connected(BT_NETWORK_CONNECTED,
					dev_info->bd_addr);
		if (!connected)
			connected = _bt_util_is_profile_connected(
						BT_NETWORK_SERVER_CONNECTED,
						dev_info->bd_addr);
		dev_info->connected_mask |= connected ? BT_NETWORK_CONNECTED : 0x00;
	}

	genlist = __bt_profile_draw_genlist(ugd, dev_info);
	vd->genlist = genlist;

	/* Set ugd as genlist object data. */
	/* We can get this data from genlist object anytime. */
	evas_object_data_set(genlist, "view_data", vd);

	back_button = elm_button_add(vd->navi_bar);
	elm_object_style_set(back_button, "naviframe/end_btn/default");

	navi_it = elm_naviframe_item_push(vd->navi_bar, BT_STR_DETAILS,
					back_button, NULL, genlist, NULL);

	ugd->navi_it = navi_it;
	evas_object_smart_callback_add(back_button, "clicked", __bt_profile_back_cb, ugd);

	elm_naviframe_prev_btn_auto_pushed_set(vd->navi_bar, EINA_FALSE);

	elm_naviframe_item_pop_cb_set(navi_it, __bt_profile_back_clicked_cb,
								ugd);

	vd->navi_it = navi_it;

	FN_END;

	return;
}

void _bt_profile_delete_view(void *data)
{
	FN_START;

	bt_ug_data *ugd;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	_bt_profile_destroy_profile_view(ugd);

	elm_naviframe_item_pop(ugd->navi_bar);

	if (ugd->rename_popup != NULL) {
		BT_INFO("Destorying rename_popup");
		evas_object_del(ugd->rename_popup);
		ugd->rename_popup = NULL;
		ugd->rename_entry = NULL;
	}

	FN_END;
}


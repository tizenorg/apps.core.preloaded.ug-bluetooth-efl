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

#include <glib.h>
#include <bluetooth.h>
#include <Elementary.h>

#include "bt-main-ug.h"
#include "bt-main-view.h"
#include "bt-dbus-method.h"
#include "bt-debug.h"
#include "bt-widget.h"
#include "bt-type-define.h"
#include "bt-string-define.h"
#include "bt-profile-view.h"
#include "bt-ipc-handler.h"
#include "bt-util.h"
#include "bt-net-connection.h"

/**********************************************************************
*                                      Static Functions declaration
***********************************************************************/
static void __bt_profile_focused_cb(void *data, Evas_Object *obj,
					void *event_info);


/**********************************************************************
*                                               Static Functions
***********************************************************************/

static int __bt_profile_delete_button(void *data)
{
	FN_START;

	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	bt_profile_view_data *vd = NULL;

	retv_if(data == NULL, BT_UG_FAIL);

	dev = (bt_dev_t *)data;
	retv_if(dev->ugd == NULL, BT_UG_FAIL);

	ugd = dev->ugd;
	retv_if(ugd->profile_vd == NULL, BT_UG_FAIL);

	vd = ugd->profile_vd;

	/* When we try to delete the buttun, 'focused' event occurs.
	    To remove this event, delete the callback.
	*/
	evas_object_smart_callback_del(dev->entry, "focused",
				__bt_profile_focused_cb);

	evas_object_del(vd->save_btn);
	evas_object_del(vd->cancel_btn);

	vd->save_btn = NULL;
	vd->cancel_btn = NULL;

	/* To shutdown the IME, set the focus to FALSE */
	elm_object_focus_set(dev->entry, EINA_FALSE);

	evas_object_smart_callback_add(dev->entry, "focused",
				__bt_profile_focused_cb, dev);

	FN_END;
	return BT_UG_ERROR_NONE;
}

static void __bt_profile_save_clicked_cb(void *data, Evas_Object *obj,
				  void *event_info)
{
	FN_START;

	bt_dev_t *dev;
	const char *entry_string;
	char *str;
	bt_ug_data *ugd;

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->layout == NULL);
	ret_if(dev->entry == NULL);

	elm_object_signal_emit((Evas_Object *)dev->layout,
			"elm,state,eraser,hide", "elm");

	entry_string = elm_entry_entry_get(dev->entry);
	ret_if(entry_string == NULL);
	str = elm_entry_markup_to_utf8(entry_string);

	if (str == NULL || strlen(str) == 0) {
		elm_object_focus_set(dev->entry, EINA_TRUE);
		if (str)
			free(str);

		ret_if(dev->ugd == NULL);
		ugd = dev->ugd;

		ugd->popup =
		    _bt_create_popup(ugd->win_main, BT_STR_ERROR,
				BT_STR_EMPTY_NAME,
				_bt_main_popup_del_cb, ugd, 2);
		ugd->back_cb = _bt_util_launch_no_event;

		return;
	}

	g_strlcpy(dev->name, str, BT_DEVICE_NAME_LENGTH_MAX);
	bt_device_set_alias(dev->addr_str, str);
	elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);

	free(str);

	/* If we try to delete the button in this function,
	    the crash will occur. */
	g_idle_add((GSourceFunc) __bt_profile_delete_button, dev);

	FN_END;
}

static void __bt_profile_eraser_clicked_cb(void *data, Evas_Object *obj,
				const char *emission, const char *source)
{
	FN_START;

	ret_if(NULL == data);

	elm_entry_entry_set((Evas_Object *)data, "");

	FN_END;
}

static void __bt_profile_changed_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Evas_Object *layout = NULL;

	ret_if(obj == NULL);
	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->layout == NULL);

	layout = (Evas_Object *)dev->layout;

	if (elm_object_focus_get(layout)) {
		if (elm_entry_is_empty(obj)) {
			elm_object_signal_emit(layout,
					"elm,state,eraser,hide", "elm");
		} else {
			elm_object_signal_emit(layout,
					"elm,state,eraser,show", "elm");
		}
	}

	FN_END;
}

static void __bt_profile_focused_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	bt_profile_view_data *vd = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *btn = NULL;

	ret_if(obj == NULL);
	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->layout == NULL);

	layout = (Evas_Object *)dev->layout;

	if (!elm_entry_is_empty(obj)) {
		elm_object_signal_emit(layout,
				"elm,state,eraser,show", "elm");
	}

	ret_if(dev->ugd == NULL);
	ugd = dev->ugd;

	ret_if(ugd->profile_vd == NULL);
	vd = ugd->profile_vd;

	if (vd->cancel_btn == NULL) {
		btn = _bt_create_button(ugd->navi_bar,
					"naviframe/back_btn/default",
					NULL, NULL,
					__bt_profile_save_clicked_cb, dev);

		vd->cancel_btn = btn;
	}

	elm_object_item_part_content_set(vd->navi_it,
					"title_right_btn", vd->cancel_btn);

	FN_END;
}

static void __bt_profile_unfocused_cb(void *data, Evas_Object *obj,
					void *event_info)
{
	FN_START;

	bt_dev_t *dev = NULL;

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->layout == NULL);

	elm_object_signal_emit((Evas_Object *)dev->layout,
			"elm,state,eraser,hide", "elm");

	FN_END;
}

static void __bt_profile_maxlength_reached(void *data, Evas_Object *obj,
						void *event_info)
{
	FN_START;

	/* In now, there is no UX guide */

	FN_END;
}

static Evas_Object *__bt_profile_name_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Elm_Entry_Filter_Limit_Size limit_filter;
	Evas_Object *layout = NULL;
	Evas_Object *entry = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.icon")) {
		layout = elm_layout_add(obj);
		dev->layout = layout;
		elm_layout_theme_set(layout, "layout", "editfield", "title");
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);

		entry = elm_entry_add(obj);
		dev->entry = entry;

		limit_filter.max_byte_count = 0;
		limit_filter.max_char_count = BT_DEVICE_NAME_LENGTH_MAX;

		elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
					     &limit_filter);

		elm_entry_prediction_allow_set(entry, EINA_FALSE);

		elm_entry_entry_set(entry, dev->name);

		evas_object_smart_callback_add(entry, "changed",
					__bt_profile_changed_cb, dev);
		evas_object_smart_callback_add(entry, "focused",
					__bt_profile_focused_cb, dev);
		evas_object_smart_callback_add(entry, "unfocused",
					__bt_profile_unfocused_cb, dev);
		evas_object_smart_callback_add(entry, "maxlength,reached",
					       __bt_profile_maxlength_reached,
						dev);

		evas_object_show(entry);

		elm_object_part_content_set(layout,
				"elm.swallow.content", entry);

		elm_object_part_text_set(layout, "elm.text",
				(const char *)BT_STR_DEVICE_NAME);

		elm_object_signal_callback_add(layout, "elm,eraser,clicked",
					"elm", __bt_profile_eraser_clicked_cb,
					entry);
	}

	FN_END;

	return layout;
}

static void __bt_profile_name_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	if (event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
					      EINA_FALSE);

	FN_END;
}

static void __bt_profile_unpair_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;
	bt_dev_t *dev = NULL;

	if (event_info)
		elm_genlist_item_selected_set((Elm_Object_Item *)event_info,
					      EINA_FALSE);
	ret_if(NULL == data);

	dev = (bt_dev_t *)data;

	if (bt_device_destroy_bond(dev->addr_str) != BT_ERROR_NONE) {
		BT_DBG("Fail to unpair");
	}

	FN_END;
}

static char *__bt_profile_unpair_label_get(void *data, Evas_Object *obj,
					    const char *part)
{
	FN_START;

	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	if (!strcmp(part, "elm.text")) {
		g_strlcpy(buf, BT_STR_UNPAIR, BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_DBG("empty text for label. \n");
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

	if (strcmp(part, "elm.text") == 0) {
		/*Label */
		g_strlcpy(buf, BT_STR_CONNECTION_OPTIONS,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_proflie_call_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	bt_dev_t *dev = NULL;
	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (strcmp(part, "elm.text") == 0) {
		g_strlcpy(buf, BT_STR_CALL_AUDIO,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_proflie_media_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	bt_dev_t *dev = NULL;
	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (strcmp(part, "elm.text") == 0) {
		g_strlcpy(buf, BT_STR_MEDIA_AUDIO,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_proflie_hid_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	bt_dev_t *dev = NULL;
	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (strcmp(part, "elm.text") == 0) {
		g_strlcpy(buf, BT_STR_INPUT_DEVICE,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static char *__bt_proflie_nap_option_label_get(void *data, Evas_Object *obj,
					      const char *part)
{
	FN_START;

	bt_dev_t *dev = NULL;
	char buf[BT_GLOBALIZATION_STR_LENGTH] = { 0, };

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (strcmp(part, "elm.text") == 0) {
		g_strlcpy(buf, BT_STR_INTERNET_ACCESS,
			BT_GLOBALIZATION_STR_LENGTH);
	} else {
		BT_DBG("This part name is not exist in style");
		return NULL;
	}

	FN_END;
	return strdup(buf);
}

static Evas_Object *__bt_profile_call_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.icon")) {
		check = elm_check_add(obj);

		dev->call_checked = dev->connected_mask & \
					BT_HEADSET_CONNECTED;

		elm_check_state_pointer_set(check,
				(Eina_Bool *)&dev->call_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);
	}

	FN_END;

	return check;
}

static Evas_Object *__bt_profile_media_option_icon_get(void *data, Evas_Object *obj,
					  const char *part)
{
	FN_START;

	Evas_Object *check = NULL;
	bt_dev_t *dev = NULL;

	retv_if(NULL == data, NULL);

	dev = (bt_dev_t *)data;

	if (!strcmp(part, "elm.icon")) {
		check = elm_check_add(obj);

		dev->media_checked = dev->connected_mask & \
					BT_STEREO_HEADSET_CONNECTED;

		elm_check_state_pointer_set(check,
				(Eina_Bool *)&dev->media_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);
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

	if (!strcmp(part, "elm.icon")) {
		check = elm_check_add(obj);

		dev->hid_checked = dev->connected_mask & \
					BT_HID_CONNECTED;

		elm_check_state_pointer_set(check,
				(Eina_Bool *)&dev->hid_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);
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

	if (!strcmp(part, "elm.icon")) {
		check = elm_check_add(obj);

		dev->network_checked = dev->connected_mask & \
					BT_NETWORK_CONNECTED;

		elm_check_state_pointer_set(check,
				(Eina_Bool *)&dev->network_checked);

		evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND,
						EVAS_HINT_EXPAND);

		evas_object_size_hint_align_set(check, EVAS_HINT_FILL,
						EVAS_HINT_FILL);
	}

	FN_END;

	return check;
}

int __bt_profile_connect_option(bt_ug_data *ugd, bt_dev_t *dev,
				bt_device_type type)
{
	FN_START;

	int audio_profile;
	gboolean connected = FALSE;

	retv_if(ugd == NULL, BT_UG_FAIL);
	retv_if(dev == NULL, BT_UG_FAIL);

	if (dev->status != BT_IDLE) {
		_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_FAILED);
		return BT_UG_FAIL;
	}

	if (type == BT_HEADSET_DEVICE)
		connected = _bt_main_is_headset_connected(ugd);
	else if (type == BT_STEREO_HEADSET_DEVICE)
		connected = _bt_main_is_stereo_headset_connected(ugd);

	if (connected == TRUE) {
		_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_EXISTS);
		return BT_UG_FAIL;
	}

	if (type == BT_HEADSET_DEVICE || type == BT_STEREO_HEADSET_DEVICE) {
		if (type == BT_STEREO_HEADSET_DEVICE)
			audio_profile = BT_AUDIO_PROFILE_TYPE_A2DP;
		else
			audio_profile = BT_AUDIO_PROFILE_TYPE_HSP_HFP;

		if (bt_audio_connect(dev->addr_str,
				audio_profile) != BT_ERROR_NONE) {
			BT_DBG("Fail to connect Headset device");
			return BT_UG_FAIL;
		}
	} else if (type == BT_HID_DEVICE) {
		BT_DBG("HID connect request\n");

		if (bt_hid_host_connect(dev->addr_str) != BT_ERROR_NONE) {
			return BT_UG_FAIL;
		}
	} else if (type == BT_NETWORK_DEVICE){
		BT_DBG("Network connect request\n");

		if (_bt_connect_net_profile(ugd->connection,
					dev->net_profile,
					dev) != BT_UG_ERROR_NONE) {
			BT_ERR("Fail to connect the net profile");
			return BT_UG_FAIL;
		}
	} else {
		BT_DBG("Unknown type");
		return BT_UG_FAIL;
	}

	ugd->connect_req = TRUE;
	dev->status = BT_CONNECTING;
	elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);

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

	retv_if(ugd == NULL, BT_UG_FAIL);
	retv_if(dev == NULL, BT_UG_FAIL);

	if (dev->status != BT_IDLE) {
		_bt_main_draw_selection_info(ugd, BT_STR_CONNECTION_FAILED);
		return BT_UG_FAIL;
	}

	memset(&param, 0x00, sizeof(bt_ug_ipc_param_t));
	memcpy(param.param2, dev->bd_addr, BT_ADDRESS_LENGTH_MAX);

	if (type == BT_HEADSET_DEVICE) {
		connected = _bt_is_profile_connected(BT_HEADSET_CONNECTED,
						ugd->conn, dev->bd_addr);
	} else if (type == BT_STEREO_HEADSET_DEVICE) {
		connected = _bt_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
						ugd->conn, dev->bd_addr);
	} else if (type == BT_HID_DEVICE) {
		connected = _bt_is_profile_connected(BT_HID_CONNECTED,
						ugd->conn, dev->bd_addr);
	} else if (type == BT_NETWORK_DEVICE) {
		connected = _bt_get_connected_net_profile(ugd->connection,
					dev->bd_addr) != NULL ? TRUE : FALSE;
	}

	if (connected == FALSE) {
		BT_DBG("Not connected");
		return BT_UG_FAIL;
	}

	if (type == BT_HEADSET_DEVICE || type == BT_STEREO_HEADSET_DEVICE) {
		if (type == BT_STEREO_HEADSET_DEVICE)
			audio_profile = BT_AUDIO_PROFILE_TYPE_A2DP;
		else
			audio_profile = BT_AUDIO_PROFILE_TYPE_HSP_HFP;

		if (bt_audio_disconnect(dev->addr_str,
				audio_profile) != BT_ERROR_NONE) {
			BT_DBG("Fail to connect Headset device");
			return BT_UG_FAIL;
		}
	} else if (type == BT_HID_DEVICE) {
		BT_DBG("Disconnecting HID service!!\n");

		if (bt_hid_host_disconnect(dev->addr_str) != BT_ERROR_NONE) {
			BT_DBG("Fail to disconnect HID device");
			return BT_UG_FAIL;
		}
	} else if (type == BT_NETWORK_DEVICE) {
		BT_DBG("Disconnecting network service!!\n");

		if (_bt_disconnect_net_profile(ugd->connection,
					dev->net_profile,
					dev) != BT_UG_ERROR_NONE) {
			BT_ERR("Fail to disconnect the net profile");
			return BT_UG_FAIL;
		}
	} else {
		BT_DBG("Unknown type");
		return BT_UG_FAIL;
	}

	ugd->connect_req = TRUE;
	dev->status = BT_DISCONNECTING;
	elm_genlist_item_update((Elm_Object_Item *)dev->genlist_item);

	if (ugd->profile_vd->genlist) {
		_bt_util_set_list_disabled(ugd->profile_vd->genlist,
					EINA_TRUE);
	}

	FN_END;
	return BT_UG_ERROR_NONE;
}

static void __bt_profile_call_disconnect_cb(void *data, Evas_Object *obj,
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

	__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
				dev, BT_HEADSET_DEVICE);

	FN_END;
}

static void __bt_profile_call_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	int ret = BT_UG_ERROR_NONE;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	Elm_Object_Item *item = NULL;
	Evas_Object *popup = NULL;
	Evas_Object *popup_btn = NULL;
	char msg[BT_DISCONNECT_TEXT_LENGTH] = { 0 };

	ret_if(event_info == NULL);

	item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	ugd = dev->ugd;

	if (dev->call_checked) {
		/* connected case */
		snprintf(msg, sizeof(msg), "%s %s<br>%s", BT_STR_END_CONNECTION,
						dev->name,
						BT_STR_DISCONNECT_Q);

		if (ugd->popup) {
			evas_object_del(ugd->popup);
			ugd->popup = NULL;
		}

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
					__bt_profile_call_disconnect_cb, dev);

		popup_btn = elm_button_add(popup);
		elm_object_text_set(popup_btn, BT_STR_CANCEL);
		elm_object_part_content_set(popup, "button2", popup_btn);
		evas_object_smart_callback_add(popup_btn, "clicked",
					_bt_main_popup_del_cb, ugd);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_HEADSET_DEVICE);
	}

	elm_genlist_item_update(item);

	FN_END;
}

static void __bt_profile_media_disconnect_cb(void *data, Evas_Object *obj,
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

	__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
				dev, BT_STEREO_HEADSET_DEVICE);

	FN_END;
}

static void __bt_profile_media_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	int ret = BT_UG_ERROR_NONE;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	Elm_Object_Item *item = NULL;
	Evas_Object *popup = NULL;
	Evas_Object *popup_btn = NULL;
	char msg[BT_DISCONNECT_TEXT_LENGTH] = { 0 };

	ret_if(event_info == NULL);

	item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	ugd = dev->ugd;

	if (dev->media_checked) {
		/* connected case */
		snprintf(msg, sizeof(msg), "%s %s<br>%s", BT_STR_END_CONNECTION,
						dev->name,
						BT_STR_DISCONNECT_Q);

		if (ugd->popup) {
			evas_object_del(ugd->popup);
			ugd->popup = NULL;
		}

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
					__bt_profile_media_disconnect_cb, dev);

		popup_btn = elm_button_add(popup);
		elm_object_text_set(popup_btn, BT_STR_CANCEL);
		elm_object_part_content_set(popup, "button2", popup_btn);
		evas_object_smart_callback_add(popup_btn, "clicked",
					_bt_main_popup_del_cb, ugd);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_STEREO_HEADSET_DEVICE);
	}

	elm_genlist_item_update(item);

	FN_END;
}

static void __bt_profile_hid_disconnect_cb(void *data, Evas_Object *obj,
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

	__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
				dev, BT_HID_DEVICE);

	FN_END;
}

static void __bt_profile_network_disconnect_cb(void *data, Evas_Object *obj,
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

	__bt_profile_disconnect_option((bt_ug_data *)dev->ugd,
				dev, BT_NETWORK_DEVICE);

	FN_END;
}

static void __bt_profile_hid_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	int ret = BT_UG_ERROR_NONE;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	Elm_Object_Item *item = NULL;
	Evas_Object *popup = NULL;
	Evas_Object *popup_btn = NULL;
	char msg[BT_DISCONNECT_TEXT_LENGTH] = { 0 };

	ret_if(event_info == NULL);

	item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	ugd = dev->ugd;

	if (dev->hid_checked) {
		/* connected case */
		snprintf(msg, sizeof(msg), "%s %s<br>%s", BT_STR_END_CONNECTION,
						dev->name,
						BT_STR_DISCONNECT_Q);
		if (ugd->popup) {
			evas_object_del(ugd->popup);
			ugd->popup = NULL;
		}

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
					__bt_profile_hid_disconnect_cb, dev);

		popup_btn = elm_button_add(popup);
		elm_object_text_set(popup_btn, BT_STR_CANCEL);
		elm_object_part_content_set(popup, "button2", popup_btn);
		evas_object_smart_callback_add(popup_btn, "clicked",
					_bt_main_popup_del_cb, ugd);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_HID_DEVICE);
	}

	elm_genlist_item_update(item);

	FN_END;
}

static void __bt_profile_nap_option_item_sel(void *data, Evas_Object *obj,
				      void *event_info)
{
	FN_START;

	int ret = BT_UG_ERROR_NONE;
	bt_dev_t *dev = NULL;
	bt_ug_data *ugd = NULL;
	Elm_Object_Item *item = NULL;
	Evas_Object *popup = NULL;
	Evas_Object *popup_btn = NULL;
	char msg[BT_DISCONNECT_TEXT_LENGTH] = { 0 };

	ret_if(event_info == NULL);

	item = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	ret_if(data == NULL);

	dev = (bt_dev_t *)data;
	ret_if(dev->ugd == NULL);

	ugd = dev->ugd;

	if (dev->network_checked) {
		/* connected case */
		snprintf(msg, sizeof(msg), "%s %s<br>%s", BT_STR_END_CONNECTION,
						dev->name,
						BT_STR_DISCONNECT_Q);
		if (ugd->popup) {
			evas_object_del(ugd->popup);
			ugd->popup = NULL;
		}

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
					__bt_profile_network_disconnect_cb, dev);

		popup_btn = elm_button_add(popup);
		elm_object_text_set(popup_btn, BT_STR_CANCEL);
		elm_object_part_content_set(popup, "button2", popup_btn);
		evas_object_smart_callback_add(popup_btn, "clicked",
					_bt_main_popup_del_cb, ugd);
	} else {
		ret = __bt_profile_connect_option((bt_ug_data *)dev->ugd,
						dev, BT_NETWORK_DEVICE);
	}

	elm_genlist_item_update(item);

	FN_END;
}

static int __bt_profile_get_item_type(bt_profile_view_data *vd, Elm_Object_Item *item)
{
	int i = 0;
	int type = BT_ITEM_NO_TYPE;
	Elm_Object_Item *current = NULL;
	Elm_Object_Item *next = NULL;

	retv_if(vd == NULL, BT_ITEM_NO_TYPE);
	retv_if(item == NULL, BT_ITEM_NO_TYPE);

	if (item == vd->name_item) {
		return BT_ITEM_TOP;
	} else if (item == vd->unpair_item) {
		return BT_ITEM_BOTTOM;
	}

	current = elm_genlist_item_next_get(vd->title_item);

	while (current != NULL) {
		next = elm_genlist_item_next_get(current);

		if (current == item) {
			if (next == NULL) {
				type = (i == 0) ? BT_ITEM_NO_TYPE : BT_ITEM_BOTTOM;
			} else {
				type = (i == 0) ? BT_ITEM_TOP : BT_ITEM_CENTER;
			}
			return type;
		}

		current = next;
		i++;
	}

	return BT_ITEM_NO_TYPE;
}

static void __bt_profile_gl_realized(void *data, Evas_Object *obj, void *event_info)
{
	FN_START;

	int item_type;
	bt_ug_data *ugd;
	bt_profile_view_data *vd;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	ret_if(data == NULL);
	ret_if(item == NULL);

	ugd = (bt_ug_data *)data;

	vd = ugd->profile_vd;
	ret_if(vd == NULL);

	item_type = __bt_profile_get_item_type(vd, item);

	BT_DBG("type: %d", item_type);

	switch (item_type) {
	case BT_ITEM_TOP:
		elm_object_item_signal_emit(item, "elm,state,top", "");
		break;
	case BT_ITEM_CENTER:
		elm_object_item_signal_emit(item, "elm,state,center", "");
		break;
	case BT_ITEM_BOTTOM:
		elm_object_item_signal_emit(item, "elm,state,bottom", "");
		break;
	default:
		elm_object_item_signal_emit(item, "elm,state,default", "");
		break;
	}

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

	vd->name_itc->item_style = "dialogue/1icon";
	vd->name_itc->func.text_get = NULL;
	vd->name_itc->func.content_get = __bt_profile_name_icon_get;
	vd->name_itc->func.state_get = NULL;
	vd->name_itc->func.del = NULL;

	vd->unpair_itc = elm_genlist_item_class_new();
	retv_if (vd->unpair_itc == NULL, NULL);

	vd->unpair_itc->item_style = "dialogue/1text";
	vd->unpair_itc->func.text_get = __bt_profile_unpair_label_get;
	vd->unpair_itc->func.content_get = NULL;
	vd->unpair_itc->func.state_get = NULL;
	vd->unpair_itc->func.del = NULL;

	/* Create genlist */
	genlist = elm_genlist_add(ugd->navi_bar);

	evas_object_smart_callback_add(genlist, "realized",
				__bt_profile_gl_realized, ugd);

	/* Seperator */
	git = elm_genlist_item_append(genlist, ugd->sp_itc, NULL, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);

	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

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
	       !(dev_info->service_list & BT_SC_HID_SERVICE_MASK) &&
	        !(dev_info->service_list & BT_SC_NAP_SERVICE_MASK)) {
		return genlist;
	}

	vd->title_itc = elm_genlist_item_class_new();
	retv_if (vd->title_itc == NULL, NULL);

	vd->title_itc->item_style = "dialogue/title";
	vd->title_itc->func.text_get = __bt_proflie_title_label_get;
	vd->title_itc->func.content_get = NULL;
	vd->title_itc->func.state_get = NULL;
	vd->title_itc->func.del = NULL;

	vd->call_itc = elm_genlist_item_class_new();
	retv_if (vd->call_itc == NULL, NULL);

	vd->call_itc->item_style = "dialogue/1text.1icon.2";
	vd->call_itc->func.text_get = __bt_proflie_call_option_label_get;
	vd->call_itc->func.content_get = __bt_profile_call_option_icon_get;
	vd->call_itc->func.state_get = NULL;
	vd->call_itc->func.del = NULL;

	vd->media_itc = elm_genlist_item_class_new();
	retv_if (vd->media_itc == NULL, NULL);

	vd->media_itc->item_style = "dialogue/1text.1icon.2";
	vd->media_itc->func.text_get = __bt_proflie_media_option_label_get;
	vd->media_itc->func.content_get = __bt_profile_media_option_icon_get;
	vd->media_itc->func.state_get = NULL;
	vd->media_itc->func.del = NULL;

	vd->hid_itc = elm_genlist_item_class_new();
	retv_if (vd->hid_itc == NULL, NULL);

	vd->hid_itc->item_style = "dialogue/1text.1icon.2";
	vd->hid_itc->func.text_get = __bt_proflie_hid_option_label_get;
	vd->hid_itc->func.content_get = __bt_profile_hid_option_icon_get;
	vd->hid_itc->func.state_get = NULL;
	vd->hid_itc->func.del = NULL;

	vd->network_itc = elm_genlist_item_class_new();
	retv_if (vd->network_itc == NULL, NULL);

	vd->network_itc->item_style = "dialogue/1text.1icon.2";
	vd->network_itc->func.text_get = __bt_proflie_nap_option_label_get;
	vd->network_itc->func.content_get = __bt_profile_nap_option_icon_get;
	vd->network_itc->func.state_get = NULL;
	vd->network_itc->func.del = NULL;

	/* Connection options title */
	git = elm_genlist_item_append(genlist, vd->title_itc, NULL, NULL,
				    ELM_GENLIST_ITEM_NONE,
				    NULL, NULL);

	elm_genlist_item_select_mode_set(git,
				ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	vd->title_item = git;

	if (dev_info->service_list & BT_SC_HFP_SERVICE_MASK ||
	     dev_info->service_list & BT_SC_HSP_SERVICE_MASK) {
		/* Call audio */
		elm_genlist_item_append(genlist, vd->call_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_call_option_item_sel,
					dev_info);
	}

	if (dev_info->service_list & BT_SC_A2DP_SERVICE_MASK) {
		/* Media audio */
		elm_genlist_item_append(genlist, vd->media_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_media_option_item_sel,
					dev_info);
	}

	BT_DBG("service list: %x", dev_info->service_list);
	BT_DBG("is hid: %d", dev_info->service_list & BT_SC_HID_SERVICE_MASK);

	if (dev_info->service_list & BT_SC_HID_SERVICE_MASK) {
		/* HID device */
		elm_genlist_item_append(genlist, vd->hid_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_hid_option_item_sel,
					dev_info);
	}

	if (dev_info->service_list & BT_SC_NAP_SERVICE_MASK) {
		/* NAP device */
		elm_genlist_item_append(genlist, vd->network_itc,
					dev_info, NULL,
					ELM_GENLIST_ITEM_NONE,
					__bt_profile_nap_option_item_sel,
					dev_info);
	}

	FN_END;

	return genlist;
}

static void __bt_profile_back_cb(void *data, Evas_Object *obj,
				  void *event_info)
{
	FN_START;

	bt_dev_t *dev_info = NULL;
	bt_ug_data *ugd = NULL;
	bt_profile_view_data *vd = NULL;

	ret_if(data == NULL);

	dev_info = (bt_dev_t *)data;
	ret_if(dev_info == NULL);
	ret_if(dev_info->ugd == NULL);

	ugd = dev_info->ugd;
	ret_if(ugd->profile_vd == NULL);

	vd = ugd->profile_vd;

	if (vd->genlist) {
		evas_object_data_set(vd->genlist, "view_data", NULL);
		elm_genlist_clear(vd->genlist);
		vd->genlist = NULL;
	}

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

	if (vd->call_itc) {
		elm_genlist_item_class_free(vd->call_itc);
		vd->call_itc = NULL;
	}

	if (vd->media_itc) {
		elm_genlist_item_class_free(vd->media_itc);
		vd->media_itc = NULL;
	}

	if (vd->hid_itc) {
		elm_genlist_item_class_free(vd->hid_itc);
		vd->hid_itc = NULL;
	}

	vd->save_btn = NULL;
	vd->cancel_btn = NULL;

	free(vd);
	ugd->profile_vd = NULL;

	elm_naviframe_item_pop(ugd->navi_bar);

	FN_END;
}


/**********************************************************************
*                                              Common Functions
***********************************************************************/

void _bt_profile_create_view(bt_dev_t *dev_info)
{
	FN_START;

	bt_profile_view_data *vd = NULL;
	bt_ug_data *ugd = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *navi_it;
	Evas_Object *back_btn = NULL;

	ret_if(dev_info == NULL);
	ret_if(dev_info->ugd == NULL);

	ugd = dev_info->ugd;
	dev_info->layout = NULL;

	vd = calloc(1, sizeof(bt_profile_view_data));
	ret_if(vd == NULL);

	ugd->profile_vd = vd;
	vd->win_main = ugd->win_main;
	vd->navi_bar = ugd->navi_bar;

	_bt_create_bg(ugd->navi_bar, "group_list");

	layout = _bt_create_layout(ugd->navi_bar, NULL, NULL);
	vd->layout = layout;

	genlist = __bt_profile_draw_genlist(ugd, dev_info);
	vd->genlist = genlist;

	/* Set ugd as genlist object data. */
	/* We can get this data from genlist object anytime. */
	evas_object_data_set(genlist, "view_data", vd);

	/* create back button */
	back_btn = elm_button_add(layout);

	navi_it = elm_naviframe_item_push(ugd->navi_bar, BT_STR_DETAILS,
					back_btn, NULL, genlist, NULL);

	/* Style set should be called after elm_naviframe_item_push(). */
	elm_object_style_set(back_btn, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_btn, "clicked",
				       __bt_profile_back_cb, (void *)dev_info);

	vd->navi_it = navi_it;

	FN_END;

	return;
}

void _bt_profile_delete_view(void *data)
{
	FN_START;

	__bt_profile_back_cb(data, NULL, NULL);

	FN_END;
}


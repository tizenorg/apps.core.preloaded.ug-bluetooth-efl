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

#include <aul.h>
#include <vconf.h>
#include <vconf-internal-setting-keys.h>
#include <Evas.h>
#include <Edje.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>
#include <notification.h>
#include <efl_extension.h>
#include <device/power.h>
#include <gesture_recognition.h>
#include <bundle.h>

#ifdef TIZEN_REDWOOD
#include <setting-cfg.h>
#endif

#include "bt-main-ug.h"
#include "bt-util.h"
#include "bt-widget.h"
#include "bt-main-view.h"
#include "bt-ipc-handler.h"
#include "bt-debug.h"
#include "bt-resource.h"
#include "bt-callback.h"
#include "bt-string-define.h"
#include "bt-net-connection.h"
#include "bluetooth_internal.h"

#define TOGGLE_OFF	"0"
#define TOGGLE_ON	"1"
#ifndef __TIZEN_OPEN__
#define S_FINDER_TOGGLE_DATA	"s_finder_setting_check_value_set"
#endif

#define BT_AUTO_CONNECT_SYSPOPUP_TIMEOUT_FOR_MULTIPLE_POPUPS 200

static void __on_destroy(ui_gadget_h ug, app_control_h service, void *priv);

extern int power_wakeup(bool dim);


/**********************************************************************
*                                               Static Functions
***********************************************************************/

static void bt_ug_change_language(bt_ug_data *ugd)
{
	FN_START;

	ret_if(ugd == NULL);

	if (ugd->profile_vd) {
		if (ugd->profile_vd->genlist)
			elm_genlist_realized_items_update(
					ugd->profile_vd->genlist);

		if (ugd->profile_vd->navi_it)
			elm_object_item_text_set(ugd->profile_vd->navi_it,
					BT_STR_DETAILS);
	}

	if (ugd->main_genlist)
		elm_genlist_realized_items_update(ugd->main_genlist);

	if (ugd->navi_it)
		elm_object_item_text_set(ugd->navi_it, BT_STR_BLUETOOTH);
	if (ugd->scan_btn) {
		if (ugd->op_status == BT_SEARCHING)
			elm_object_text_set(ugd->scan_btn, BT_STR_STOP);
		else
			elm_object_text_set(ugd->scan_btn, BT_STR_SCAN);
	}

	if (ugd->popup) {
		_bt_set_popup_text(ugd, ugd->popup);
	}
	FN_END;
}

static void __bt_ug_release_memory(bt_ug_data *ugd)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	ret_if(ugd == NULL);
	ret_if(ugd->main_genlist == NULL);

	elm_genlist_clear(ugd->main_genlist);
	evas_object_del(ugd->main_genlist);
	ugd->main_genlist = NULL;

	/* Release paired device items */
	EINA_LIST_FOREACH_SAFE(ugd->paired_device, l, l_next, dev) {
		ugd->paired_device =
			eina_list_remove_list(ugd->paired_device, l);
		_bt_util_free_device_item(dev);
	}

	/* Release searched device items */
	EINA_LIST_FOREACH_SAFE(ugd->searched_device, l, l_next, dev) {
		ugd->searched_device =
		    eina_list_remove_list(ugd->searched_device, l);
		_bt_util_free_device_item(dev);
	}

	if (ugd->device_name_itc) {
		elm_genlist_item_class_free(ugd->device_name_itc);
		ugd->device_name_itc = NULL;
	}

	if (ugd->rename_entry_itc) {
		elm_genlist_item_class_free(ugd->rename_entry_itc);
		ugd->rename_entry_itc = NULL;
	}

	if (ugd->rename_desc_itc) {
		elm_genlist_item_class_free(ugd->rename_desc_itc);
		ugd->rename_desc_itc = NULL;
	}

	if (ugd->visible_itc) {
		elm_genlist_item_class_free(ugd->visible_itc);
		ugd->visible_itc = NULL;
	}

	if (ugd->timeout_value_itc) {
		elm_genlist_item_class_free(ugd->timeout_value_itc);
		ugd->timeout_value_itc = NULL;
	}

	if (ugd->paired_title_itc) {
		elm_genlist_item_class_free(ugd->paired_title_itc);
		ugd->paired_title_itc = NULL;
	}

	if (ugd->searched_title_itc) {
		elm_genlist_item_class_free(ugd->searched_title_itc);
		ugd->searched_title_itc = NULL;
	}

	if (ugd->paired_device_itc) {
		elm_genlist_item_class_free(ugd->paired_device_itc);
		ugd->paired_device_itc = NULL;
	}

	if (ugd->searched_device_itc) {
		elm_genlist_item_class_free(ugd->searched_device_itc);
		ugd->searched_device_itc = NULL;
	}

	if (ugd->no_device_itc) {
		elm_genlist_item_class_free(ugd->no_device_itc);
		ugd->no_device_itc = NULL;
	}

	if (ugd->on_itc) {
		elm_genlist_item_class_free(ugd->on_itc);
		ugd->on_itc = NULL;
	}

	if (ugd->off_itc) {
		elm_genlist_item_class_free(ugd->off_itc);
		ugd->off_itc = NULL;
	}

	if (ugd->popup_bundle) {
		free(ugd->popup_bundle);
		ugd->popup_bundle = NULL;
	}

	if (ugd->popup)
		_bt_main_popup_del_cb(ugd, NULL, NULL);

	if (ugd->popup_timer) {
		g_source_remove(ugd->popup_timer);
		ugd->popup_timer = 0;
	}

	if (ugd->base) {
		evas_object_del(ugd->base);
		ugd->base = NULL;
	}

	FN_END;
}

#ifndef __TIZEN_OPEN__
void __bt_motion_shake_cb(gesture_type_e motion,
					const gesture_data_h event_data,
					double timestamp,
					gesture_error_e error,
					void *data)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	gesture_event_e event;
	int ret = 0;

	ret_if(data == NULL);
	ret_if(event_data == NULL);
	ret_if(error != GESTURE_ERROR_NONE);

	ugd = (bt_ug_data *)data;

	if (motion == GESTURE_SHAKE) {
		ret = gesture_get_event(data, &event);
		ret_if(ret != GESTURE_ERROR_NONE);

		if (event == GESTURE_SHAKE_DETECTED) {
			BT_DBG("Scan devices");
			ret = power_wakeup(0);
			if (ret != 0) {
				BT_ERR("Power wakeup failed [%d]", ret);
			}
			_bt_main_scan_device(ugd);
		}
	}

	FN_END;
}
#endif

static void __bt_main_vconf_change_cb(keynode_t *key, void *data)
{
	retm_if(NULL == key, "key is NULL");
	retm_if(NULL == data, "data is NULL");
	bt_ug_data *ugd = (bt_ug_data *)data;

	char *vconf_name = vconf_keynode_get_name(key);

	if (!g_strcmp0(vconf_name, VCONFKEY_SETAPPL_DEVICE_NAME_STR)){
		char *name_value = NULL;
		name_value = vconf_get_str(VCONFKEY_SETAPPL_DEVICE_NAME_STR);
		retm_if (!name_value, "Get string is failed");
		BT_INFO("name : %s", name_value);

		if (ugd->device_name_item && g_strcmp0(ugd->phone_name, name_value)) {
			_bt_update_genlist_item(ugd->device_name_item);
			_bt_update_genlist_item(ugd->rename_entry_item);
		}
		g_free(name_value);
	} else {
		BT_ERR("vconf_name is error");
	}
}

static int __bt_initialize_view(bt_ug_data *ugd)
{
	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY) {
		_bt_main_draw_visibility_view(ugd);
	} else if (ugd->bt_launch_mode == BT_LAUNCH_ONOFF) {
		_bt_main_draw_onoff_view(ugd);
	} else {
		_bt_main_draw_list_view(ugd);
	}
	return 0;
}

#ifdef TIZEN_BT_A2DP_SINK_ENABLE
static void __bt_free_device_info(bt_dev_t *dev)
{
	ret_if(!dev);

	int i;

	if (dev->uuids) {
		for (i = 0; i < dev->uuid_count ; i++) {
			if (dev->uuids[i]) {
				g_free(dev->uuids[i]);
				dev->uuids[i] = NULL;
			}
		}
		g_free(dev->uuids);
		dev->uuids = NULL;
	}

	free(dev);
	dev = NULL;
}

static void __bt_free_paired_device(bt_ug_data *ugd)
{
	FN_START;
	retm_if(!ugd, "ad is NULL!");
	retm_if(!ugd->paired_device, "paired_device is NULL!");

	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	bt_dev_t *dev = NULL;

	EINA_LIST_FOREACH_SAFE(ugd->paired_device, l, l_next, dev) {
		__bt_free_device_info(dev);
	}

	eina_list_free(ugd->paired_device);
	ugd->paired_device = NULL;
	FN_END;
}

static bool __bt_cb_adapter_create_paired_device_list
			(bt_device_info_s *device_info, void *user_data)
{
	FN_START;
	bt_dev_t *dev = NULL;
	gboolean connected = FALSE;
	bt_ug_data *ugd = NULL;
	unsigned int service_class;


	ugd = (bt_ug_data *)user_data;
	retv_if(ugd == NULL, false);

	dev = _bt_main_create_paired_device_item(device_info);
	retv_if (!dev, false);

	dev->ugd = (void *)ugd;

	service_class = dev->service_class;

	if (_bt_main_is_matched_profile(ugd->search_type,
					dev->major_class,
					service_class,
					ugd->service,
					dev->minor_class) == TRUE) {

		if (dev->service_list & BT_SC_HFP_SERVICE_MASK ||
		    dev->service_list & BT_SC_HSP_SERVICE_MASK ||
		    dev->service_list & BT_SC_A2DP_SERVICE_MASK) {
			connected = _bt_util_is_profile_connected(BT_HEADSET_CONNECTED,
							dev->bd_addr);
			dev->connected_mask |= connected ? BT_HEADSET_CONNECTED : 0x00;

			connected = _bt_util_is_profile_connected(BT_STEREO_HEADSET_CONNECTED,
							dev->bd_addr);
			dev->connected_mask |=
				connected ? BT_STEREO_HEADSET_CONNECTED : 0x00;
		}

		if (dev->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK) {
			connected = _bt_util_is_profile_connected(BT_MUSIC_PLAYER_CONNECTED,
							dev->bd_addr);
			dev->connected_mask |=
				connected ? BT_MUSIC_PLAYER_CONNECTED : 0x00;
		}

		dev->status = BT_IDLE;
		dev->ugd = (void *)ugd;
		dev->is_bonded = TRUE;
		ugd->paired_device =
			eina_list_append(ugd->paired_device, dev);
		_bt_update_device_list(ugd);
	} else {
		BT_ERR("Device class and search type do not match");
		free(dev);
	}

	FN_END;
	return true;
}

static void __bt_get_paired_devices(bt_ug_data *ugd)
{
	FN_START;

	ret_if(ugd == NULL);
	__bt_free_paired_device(ugd);
	if (bt_adapter_foreach_bonded_device(
			__bt_cb_adapter_create_paired_device_list,
			(void *)ugd) != BT_ERROR_NONE) {
		BT_ERR("bt_adapter_foreach_bonded_device() failed");
	}

	FN_END;
	return;
}

static int __bt_get_paired_device_count(bt_ug_data *ugd)
{
	FN_START;
	retvm_if(!ugd, 0, "ugd is NULL!");
	__bt_get_paired_devices(ugd);
	retvm_if(!ugd->paired_device, 0, "paired_device is NULL!");
	int count = eina_list_count(ugd->paired_device);
	//BT_DBG("paired device count : %d", count);
	return count;
}

static void __bt_create_autoconnect_popup(bt_dev_t *dev)
{
	FN_START;

	int ret = 0;
	bundle *b = NULL;
	b = bundle_create();
	retm_if (!b, "Unable to create bundle");

	bundle_add_str(b, "event-type", "music-auto-connect-request");

	ret = syspopup_launch("bt-syspopup", b);
	if (0 > ret)
		BT_ERR("Popup launch failed...retry %d", ret);
	bundle_free(b);

	FN_END;
}

static void __bt_auto_connect(bt_ug_data *ugd)
{
	FN_START;
	if (__bt_get_paired_device_count(ugd) == 1) {
		BT_DBG("Launch mode is %d", ugd->bt_launch_mode);

		bt_dev_t *dev = eina_list_nth(ugd->paired_device, 0);
		if(dev == NULL) {
			BT_ERR("dev is NULL");
			__on_destroy(ugd->ug, NULL, ugd);
			return;
		}

		/* Check whether the only paired device is Headset */
		if (dev->service_list & BT_SC_A2DP_SOURCE_SERVICE_MASK) {
			BT_DBG("Remote device support A2DP Source");
			_bt_main_connect_device(ugd, dev);
			BT_DBG("dev->status : %d", dev->status);
			if (dev->status == BT_CONNECTING) {
				__bt_create_autoconnect_popup(dev);
				return;
			}
		}
	}

	if (__bt_initialize_view(ugd) < 0) {
		app_control_h service = NULL;
		BT_ERR("__bt_initialize_view failed");
		service = __bt_main_get_connection_result(ugd, FALSE);
		_bt_ug_destroy(ugd, (void *)service);
	}
	FN_END;
}
#endif

static Eina_Bool __bt_launch_idler(void *data)
{
	FN_START;
	BT_DBG("UG_LAUNCH_PROFILING");

	int ret;
	int err;
	bt_ug_data *ugd = NULL;

	retv_if(data == NULL, ECORE_CALLBACK_CANCEL);

	ugd = (bt_ug_data *)data;

	/* Set event callbacks */
#ifndef __TIZEN_OPEN__
	char *toggle_data = NULL;
	bt_adapter_state_e value;
#endif

#ifdef TIZEN_BT_A2DP_SINK_ENABLE
	if (ugd->bt_launch_mode == BT_LAUNCH_CONNECT_AUDIO_SOURCE) {
		if (bt_audio_initialize() != BT_ERROR_NONE)
			BT_ERR("bt_audio_initialize() failed");
		__bt_auto_connect(ugd);
	} else
#endif
		__bt_initialize_view(ugd);

	err = _bt_ipc_register_popup_event_signal(ugd);
	if (err != BT_UG_ERROR_NONE)
		BT_ERR("_bt_ipc_register_popup_event_signal failed: %d", err);

	if (bt_audio_initialize() != BT_ERROR_NONE)
		BT_ERR("bt_audio_initialize() failed");
	if (_bt_create_net_connection(&ugd->connection) != BT_UG_ERROR_NONE)
		BT_ERR("_bt_create_net_connection fail");

	ret =
	    bt_adapter_set_state_changed_cb(_bt_cb_state_changed, (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("bt_adapter_set_state_changed_cb failed");

	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY ||
	     ugd->bt_launch_mode == BT_LAUNCH_ONOFF) {
		/* Don't need to register callback */
		return ECORE_CALLBACK_CANCEL;
	}

	ret =
	    bt_audio_set_connection_state_changed_cb(_bt_cb_audio_state_changed,
						     (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("audio set connection state callback failed");

	ret = bt_device_set_connection_state_changed_cb
			(_bt_cb_device_connection_state_changed, (void *)ugd);

	if (ret != BT_ERROR_NONE)
		BT_ERR("device set connection state callback failed");

	ret =
	    bt_adapter_set_device_discovery_state_changed_cb
	    (_bt_cb_discovery_state_changed, (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("adapter set device discovery state callback failed");

	ret = bt_adapter_set_visibility_mode_changed_cb
	    (_bt_cb_visibility_mode_changed, (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("adapter set device visibility mode callback failed");

	ret = bt_device_set_bond_created_cb(_bt_cb_bonding_created,
					    (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("bt_device_set_bond_created_cb failed");

	ret = bt_device_set_bond_destroyed_cb(_bt_cb_bonding_destroyed,
					      (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("bt_device_set_bond_destroyed_cb failed");

	ret = bt_device_set_service_searched_cb(_bt_cb_service_searched,
						(void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("bt_device_set_service_searched_cb failed");

	ret = bt_adapter_set_name_changed_cb(_bt_cb_adapter_name_changed,
					     (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("bt_adapter_set_name_changed_cb failed");

	ret = bt_hid_host_initialize(_bt_cb_hid_state_changed, (void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("bt_hid_host_initialize failed");

	ret = bt_nap_set_connection_state_changed_cb(_bt_cb_nap_state_changed,
							(void *)ugd);
	if (ret != BT_ERROR_NONE)
		BT_ERR("bt_nap_set_connection_state_changed_cb failed");

#ifndef __TIZEN_OPEN__
	if (app_control_get_extra_data(ugd->service, S_FINDER_TOGGLE_DATA,
				  &toggle_data) == APP_CONTROL_ERROR_NONE) {
		if (toggle_data) {
			if(bt_adapter_get_state(&value) == BT_ERROR_NONE) {
				if (g_strcmp0(toggle_data, TOGGLE_OFF) == 0 &&
					value == BT_ADAPTER_ENABLED) {
					ret = _bt_main_disable_bt(ugd);
					if (ret != BT_ERROR_NONE)
						BT_ERR("_bt_main_disable_bt fail!");
				} else if(g_strcmp0(toggle_data, TOGGLE_ON) == 0 &&
					value == BT_ADAPTER_DISABLED) {
					ret = _bt_main_enable_bt(ugd);
					if (ret != BT_ERROR_NONE)
						BT_ERR("_bt_main_enable_bt fail!");
				}
			}
			free(toggle_data);
		}
	}
#endif

	/* In the NFC case, we don't need to display the paired device */
	if (ugd->op_status == BT_ACTIVATED &&
	    ugd->bt_launch_mode != BT_LAUNCH_USE_NFC) {
		_bt_main_draw_paired_devices(ugd);
	} else if (ugd->op_status == BT_DEACTIVATED &&
		   ugd->bt_launch_mode != BT_LAUNCH_NORMAL) {
		_bt_main_enable_bt(ugd);
	}

	/* In the NFC case, we don't need to display the paired device */
	if (ugd->op_status == BT_ACTIVATED &&
	    ugd->bt_launch_mode != BT_LAUNCH_USE_NFC &&
	    ugd->bt_launch_mode != BT_LAUNCH_ONOFF) {
		if (_bt_util_is_battery_low() == FALSE) {
			ret = bt_adapter_start_device_discovery();
			if (!ret) {
				ugd->op_status = BT_SEARCHING;
				elm_object_text_set(ugd->scan_btn,
						    BT_STR_STOP);

				if (ugd->searched_title == NULL)
					_bt_main_add_searched_title
					    (ugd);
			} else {
				BT_ERR
				    ("Operation failed : Error Cause[%d]",
				     ret);
			}
		}
	}
	FN_END;
	BT_DBG("UG_LAUNCH_PROFILING");
	return ECORE_CALLBACK_CANCEL;
}

static void *__on_create(ui_gadget_h ug, enum ug_mode mode, app_control_h service,
			 void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	BT_DBG("UG_LAUNCH_PROFILING");
#ifndef __TIZEN_OPEN__
	gesture_h handle;
#endif

	retv_if (!ug || !priv, NULL);

	ugd = (bt_ug_data *)priv;
	ugd->ug = ug;

	bindtextdomain(PKGNAME, LOCALEDIR);

	ugd->win_main = ug_get_window();

	if (!ugd->win_main) {
		__on_destroy(ug, NULL, ugd);
		return NULL;
	}

	_bt_main_init_status(ugd, service);
	BT_DBG("ugd->bt_launch_mode : %d", ugd->bt_launch_mode);

#ifndef __TIZEN_OPEN__
	int ret;
	bool supported = false;

	ret = gesture_is_supported(GESTURE_SHAKE, &supported);
	if (ret != GESTURE_ERROR_NONE) {
		BT_ERR("gesture_is_supported failed : %d", ret);
	} else  {
		if (!supported) {
			BT_ERR("gesture is not supported");
		} else {
			ret =  gesture_create(&handle);
			if (ret != GESTURE_ERROR_NONE) {
				BT_ERR("gesture_create failed : %d", ret);
			} else {
				ugd->handle = handle;
				ret = gesture_start_recognition(handle, GESTURE_SHAKE,
					GESTURE_OPTION_DEFAULT, __bt_motion_shake_cb, ugd);
				if (ret != GESTURE_ERROR_NONE) {
					BT_ERR("gesture_start_recognition failed : %d", ret);
					gesture_release(handle);
					ugd->handle = NULL;
				}
			}
		}
	}
#endif
	ugd->service = service;

	ugd->base = _bt_main_base_layout_create(ugd->win_main, ugd);
	if (!ugd->base) {
		__on_destroy(ug, NULL, ugd);
		return NULL;
	}
	/* Add layout for custom styles */
	elm_theme_extension_add(NULL, BT_GENLIST_EDJ);

#if 0
	if (ugd->bt_launch_mode == BT_LAUNCH_VISIBILITY) {
		_bt_main_draw_visibility_view(ugd);
	} else if (ugd->bt_launch_mode == BT_LAUNCH_ONOFF) {
		_bt_main_draw_onoff_view(ugd);
	} else {
		_bt_main_draw_list_view(ugd);
	}
#endif

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_DEVICE_NAME_STR,
			__bt_main_vconf_change_cb, ugd);

	if (ret < 0) {
		BT_ERR("vconf_notify_key_changed failed");
	}

//	ugd->visibility_changed_by_ug = FALSE;
	ugd->ug_status = BT_UG_CREATE;

	BT_DBG("UG_LAUNCH_PROFILING");
	FN_END;
	return ugd->base;
}

static void __on_start(ui_gadget_h ug, app_control_h service, void *priv)
{
	FN_START;
	BT_DBG("UG_LAUNCH_PROFILING");

	bt_ug_data *ugd = NULL;

	ugd = priv;

/* Tizen 2.4's setting supports auto rotate mode */
#if 0
	elm_win_wm_rotation_preferred_rotation_set(ugd->win_main, 0);
#endif

	if (!ecore_idler_add(__bt_launch_idler, ugd))
		BT_ERR("idler can not be added");

	ugd->ug_status = BT_UG_START;
	BT_DBG("UG_LAUNCH_PROFILING");

	FN_END;
}

static void __on_pause(ui_gadget_h ug, app_control_h service, void *priv)
{
	FN_START;
	BT_DBG("UG_LAUNCH_PROFILING");
	bt_ug_data *ugd = NULL;

	retm_if(priv == NULL, "Invalid argument: priv is NULL");

	BT_INFO("Pause UG");

	ugd = (bt_ug_data *)priv;

	if (ugd->op_status == BT_SEARCHING)
		bt_adapter_stop_device_discovery();

	ugd->ug_status = BT_UG_PAUSE;

	FN_END;
}

static void __on_resume(ui_gadget_h ug, app_control_h service, void *priv)
{
	FN_START;
	BT_DBG("UG_LAUNCH_PROFILING");

	bt_ug_data *ugd = NULL;

	retm_if(priv == NULL, "Invalid argument: priv is NULL");

	BT_INFO("Resume UG");

	ugd = (bt_ug_data *)priv;

	ugd->ug_status = BT_UG_RESUME;
	_bt_update_genlist_item(ugd->visible_item);

	FN_END;
}

static void __on_destroy(ui_gadget_h ug, app_control_h service, void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	int err;

	if (!ug || !priv)
		return;

	ugd = priv;

	ugd->ug_status = BT_UG_DESTORY;

	if (ugd->request_timer) {
		ecore_timer_del(ugd->request_timer);
		ugd->request_timer = NULL;
	}

	if (ugd->timeout_id) {
		g_source_remove(ugd->timeout_id);
		ugd->timeout_id = 0;
	}

	if (ugd->network_timer) {
		ecore_timer_del(ugd->network_timer);
		ugd->network_timer = NULL;
	}

	/* This has been added before cancel discovery so as to
		prevent device addition till cancel discovery */
	err = bt_adapter_unset_device_discovery_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset of device discovery state cb failed: %d", err);
	if (ugd->op_status == BT_SEARCHING) {
		err = bt_adapter_stop_device_discovery();
		if (err != BT_ERROR_NONE)
			BT_ERR("Stop device discovery failed: %d", err);
	}

	err = bt_adapter_unset_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset of state change cb  failed: %d", err);

	err = bt_device_unset_bond_created_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset of bond creation cb failed: %d", err);

	err = bt_device_unset_bond_destroyed_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset of bond destroyed cb failed: %d", err);

	err = bt_device_unset_service_searched_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset of service search cb failed: %d", err);

	err = bt_audio_unset_connection_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset audio connection state cb failed: %d", err);

	err = bt_device_unset_connection_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset device connection state cb failed: %d", err);

	err = bt_adapter_unset_name_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("unset name change cb failed: %d", err);

	err = bt_hid_host_deinitialize();
	if (err != BT_ERROR_NONE)
		BT_ERR("bt_hid_host_deinitialize failed: %d", err);

	err = bt_audio_deinitialize();
	if (err != BT_ERROR_NONE)
		BT_ERR("bt_audio_deinitialize failed: %d", err);

	err = bt_nap_unset_connection_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_ERR("bt_nap_unset_connection_state_changed_cb err=%d", err);

	err = bt_deinitialize();
	if (err != BT_ERROR_NONE)
		BT_ERR("bt_deinitialize failed: %d", err);

	if (ugd->connection) {
		err = _bt_destroy_net_connection(ugd->connection);
		if (err != BT_UG_ERROR_NONE)
			BT_ERR("_bt_destroy_net_connection failed: %d", err);

		ugd->connection = NULL;
	}

	err = vconf_ignore_key_changed(VCONFKEY_SETAPPL_DEVICE_NAME_STR,
			(vconf_callback_fn) __bt_main_vconf_change_cb);

	if (err < 0) {
		BT_ERR("vconf_ignore_key_changed failed");
	}

	err = _bt_ipc_unregister_popup_event_signal(ugd);
	if (err != BT_UG_ERROR_NONE)
		BT_ERR("_bt_ipc_unregister_popup_event_signal failed: %d", err);

	__bt_ug_release_memory(ugd);

#ifndef __TIZEN_OPEN__
	if (ugd->handle) {
		gesture_release(ugd->handle);
		ugd->handle = NULL;
	}
#endif
	FN_END;
}

static void __on_message(ui_gadget_h ug, app_control_h msg, app_control_h service,
			 void *priv)
{
	FN_START;
	FN_END;
}

static void __on_event(ui_gadget_h ug, enum ug_event event, app_control_h service,
		       void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ret_if(priv == NULL);

	ugd = (bt_ug_data *)priv;

	BT_INFO("Event : %d", event);

	switch (event) {
		case UG_EVENT_LOW_MEMORY:
			break;
		case UG_EVENT_LOW_BATTERY:
			if (_bt_util_is_battery_low() == FALSE)
				return;

			if (ugd->op_status == BT_SEARCHING)
				bt_adapter_stop_device_discovery();

			_bt_main_create_information_popup(ugd, BT_STR_LOW_BATTERY);
			break;
		case UG_EVENT_LANG_CHANGE:
			bt_ug_change_language(ugd);
			break;
		default:
			break;
	}
	FN_END;
}

static void __on_key_event(ui_gadget_h ug, enum ug_key_event event,
			   app_control_h service, void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	BT_INFO("Key event UG : %d", event);

	ugd = (bt_ug_data *)priv;

	switch (event) {
	case UG_KEY_EVENT_END:
		_bt_ug_destroy(ugd, NULL);
		break;
	default:
		break;
	}
	FN_END;
}

/**********************************************************************
*                                             Common Functions
***********************************************************************/

UG_MODULE_API int UG_MODULE_INIT(struct ug_module_ops *ops)
{
	FN_START;
	BT_DBG("UG_LAUNCH_PROFILING");

	bt_ug_data *ugd = NULL;

	if (!ops)
		return BT_UG_FAIL;

	ugd = calloc(1, sizeof(bt_ug_data));
	if (!ugd)
		return BT_UG_FAIL;

	ops->create = __on_create;
	ops->start = __on_start;
	ops->pause = __on_pause;
	ops->resume = __on_resume;
	ops->destroy = __on_destroy;
	ops->message = __on_message;
	ops->event = __on_event;
	ops->key_event = __on_key_event;
	ops->priv = ugd;
	ops->opt = UG_OPT_INDICATOR_ENABLE;

	FN_END;
	return BT_UG_ERROR_NONE;
}

UG_MODULE_API void UG_MODULE_EXIT(struct ug_module_ops *ops)
{
	FN_START;

	bt_ug_data *ugd;

	if (!ops)
		return;

	ugd = ops->priv;
	if (ugd)
		free(ugd);

	FN_END;
}

UG_MODULE_API int setting_plugin_reset(app_control_h service, void *priv)
{
	FN_START;

	int ret;
	int result = 0;

	ret = bt_initialize();

	if (ret != BT_ERROR_NONE) {
		BT_ERR("Fail to init BT %d", ret);
		return ret;
	}

	ret = bt_adapter_reset();

	if (ret != BT_ERROR_NONE) {
		BT_ERR("Fail to reset adapter: %d", ret);
		result = ret;
	}

	ret = bt_deinitialize();

	if (ret != BT_ERROR_NONE)
		BT_ERR("Fail to deinit BT: %d", ret);

	FN_END;
	return result;
}

void _bt_ug_destroy(void *data, void *result)
{
	FN_START;
	int ret = 0;
	bt_ug_data *ugd = NULL;

	BT_INFO("Destroy UG");

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	if (result != NULL) {
		ug_send_result(ugd->ug, result);
	}

	if (ugd->ug)
		ret = ug_destroy_me(ugd->ug);

	if (ret < 0)
		BT_ERR("Fail to destroy me");

	FN_END;
}

#ifndef __TIZEN_OPEN__
#ifdef TIZEN_REDWOOD

static bt_setting_cfg_node_t s_cfg_node_array_bt[] = {
		{"IDS_BT_BODY_BLUETOOTH", NULL,
			"viewtype:frontpage;tab:first;keyword:IDS_BT_BODY_BLUETOOTH",
			Cfg_Item_Pos_Level0, 0, 0, Cfg_Item_View_Node_Toggle, NULL},
		{"IDS_BT_BODY_VISIBLE", NULL,
			"viewtype:frontpage;tab:first;keyword:IDS_BT_BODY_VISIBLE",
			Cfg_Item_Pos_Level0, 0, 0, BT_CFG_ITEM_VIEW_NODE, NULL},
};

UG_MODULE_API int setting_plugin_search_init(app_control_h service,
					void *priv, char **applocale)
{
	FN_START;
	BT_DBG("ug-bluetooth-efl DB search code: SETTING_SEARCH");

	Eina_List **pplist = NULL;
	int i;
	int size = sizeof(s_cfg_node_array_bt)/sizeof(s_cfg_node_array_bt[0]);

	*applocale = strdup(PKGNAME);
	pplist = (Eina_List **)priv;

	for (i = 0; i < size; i++) {
		bt_setting_cfg_node_t *node =
				(bt_setting_cfg_node_t *)setting_plugin_search_item_add(
					s_cfg_node_array_bt[i].key_name,
					s_cfg_node_array_bt[i].ug_args,
					NULL, s_cfg_node_array_bt[i].item_type,
					s_cfg_node_array_bt[i].data);
		*pplist = eina_list_append(*pplist, node);
	}
	FN_END;
	return 0;
}

/**************************************************************
 toggle state get/set function for "bluetooth status"
************************************************************/
EXPORT_PUBLIC
int get_display_ug_state(Cfg_Item_State *stat, void *data)
{
	FN_START;

	int value = -1;
	int ret = vconf_get_int(VCONFKEY_BT_STATUS, &value);
	retvm_if (ret != 0, ret, "fail to get vconf key!");

	if (value == VCONFKEY_BT_STATUS_OFF){
		*stat = Cfg_Item_Off;
	} else {
		*stat = Cfg_Item_On;
	}

	FN_END;
	return ret;
}

EXPORT_PUBLIC
int set_display_ug_state(Cfg_Item_State stat, void *item, void *data)
{
	FN_START;
	int value = -1;
	int ret = 0;

	if (stat == Cfg_Item_On){
		value = VCONFKEY_BT_STATUS_ON;
	} else {
		value = VCONFKEY_BT_STATUS_OFF;
	}

	ret = vconf_set_int(VCONFKEY_BT_STATUS, value);
	retvm_if (ret != 0, ret, "fail to set vconf key!");

	FN_END;
	return ret;
}

EXPORT_PUBLIC
int set_display_ug_update_ui(Cfg_Item_State stat, void* data)
{
	FN_START;

	FN_END;
	return 0;
}

EXPORT_PUBLIC
cfg_func_table opt_tab_bluetooth = {
	.get_item_state = get_display_ug_state,
	.set_item_state = set_display_ug_state,
	.set_item_update_ui = set_display_ug_update_ui,
};

UG_MODULE_API int setting_plugin_search_query_ops(char *str_id,
								void **tfunc_obj)
{
	FN_START;
	BT_DBG(">> get tfunc operation via plugin-model 1");
	if (str_id && !g_strcmp0(str_id, _("IDS_BT_BODY_BLUETOOTH"))) {
		*tfunc_obj = (void*)&opt_tab_bluetooth;
	}
	FN_END;
	return 0;
}
#endif
#endif


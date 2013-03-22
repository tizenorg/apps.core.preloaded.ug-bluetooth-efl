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

#include <aul.h>
#include <vconf.h>
#include <vconf-internal-setting-keys.h>
#include <Evas.h>
#include <Edje.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>

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

static void __on_destroy(ui_gadget_h ug, service_h service, void *priv);


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

	FN_END;
}

static Evas_Object *__bt_create_fullview(Evas_Object * parent, bt_ug_data *ugd)
{
	FN_START;

	Evas_Object *base = NULL;

	base = elm_layout_add(parent);
	if (!base)
		return NULL;

	elm_layout_theme_set(base, "layout", "application", "default");
	evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(base, EVAS_HINT_FILL, EVAS_HINT_FILL);

	FN_END;
	return base;
}

static Evas_Object *__bt_create_frameview(Evas_Object * parent,
					  bt_ug_data *ugd)
{
	FN_START;

	Evas_Object *base = NULL;

	base = elm_layout_add(parent);
	if (!base)
		return NULL;

	elm_layout_theme_set(base, "layout", "application", "default");
	evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(base, EVAS_HINT_FILL, EVAS_HINT_FILL);

	FN_END;
	return base;
}

static void __bt_ug_release_memory(bt_ug_data *ugd)
{
	FN_START;

	bt_dev_t *dev = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	ret_if(ugd == NULL);

	/* Release profile view data */
	if (ugd->profile_vd) {
		if (ugd->profile_vd->genlist)
			elm_genlist_clear(ugd->profile_vd->genlist);

		if (ugd->profile_vd->name_itc) {
			elm_genlist_item_class_free(ugd->profile_vd->name_itc);
			ugd->profile_vd->name_itc = NULL;
		}

		if (ugd->profile_vd->unpair_itc) {
			elm_genlist_item_class_free(ugd->profile_vd->unpair_itc);
			ugd->profile_vd->unpair_itc = NULL;
		}

		if (ugd->profile_vd->title_itc) {
			elm_genlist_item_class_free(ugd->profile_vd->title_itc);
			ugd->profile_vd->title_itc = NULL;
		}

		if (ugd->profile_vd->call_itc) {
			elm_genlist_item_class_free(ugd->profile_vd->call_itc);
			ugd->profile_vd->call_itc = NULL;
		}

		if (ugd->profile_vd->media_itc) {
			elm_genlist_item_class_free(ugd->profile_vd->media_itc);
			ugd->profile_vd->media_itc = NULL;
		}

		if (ugd->profile_vd->hid_itc) {
			elm_genlist_item_class_free(ugd->profile_vd->hid_itc);
			ugd->profile_vd->hid_itc = NULL;
		}

		free(ugd->profile_vd);
		ugd->profile_vd = NULL;
	}

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

	/* Release genlist item class */
	if (ugd->sp_itc) {
		elm_genlist_item_class_free(ugd->sp_itc);
		ugd->sp_itc = NULL;
	}

	if (ugd->status_itc) {
		elm_genlist_item_class_free(ugd->status_itc);
		ugd->status_itc = NULL;
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

	if (ugd->device_itc) {
		elm_genlist_item_class_free(ugd->device_itc);
		ugd->device_itc = NULL;
	}

	if (ugd->searched_itc) {
		elm_genlist_item_class_free(ugd->searched_itc);
		ugd->searched_itc = NULL;
	}

	if (ugd->no_device_itc) {
		elm_genlist_item_class_free(ugd->no_device_itc);
		ugd->no_device_itc = NULL;
	}

	if (ugd->main_genlist)
		elm_genlist_clear(ugd->main_genlist);

	/* Unref the Dbus connections */
	if (ugd->conn)
		dbus_g_connection_unref(ugd->conn);

	if (ugd->popup_bundle) {
		free(ugd->popup_bundle);
		ugd->popup_bundle = NULL;
	}

	if (ugd->popup_timer) {
		g_source_remove(ugd->popup_timer);
		ugd->popup_timer = 0;
	}

	FN_END;
}

static void *__on_create(ui_gadget_h ug, enum ug_mode mode, service_h service,
			 void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;
	Evas_Object *bg = NULL;

	if (!ug || !priv)
		return NULL;

	ugd = (bt_ug_data *)priv;
	ugd->ug = ug;

	bindtextdomain(PKGNAME, LOCALEDIR);

	ugd->win_main = ug_get_parent_layout(ug);

	if (!ugd->win_main)
		return NULL;

	if (mode == UG_MODE_FULLVIEW) {
		ugd->base = __bt_create_fullview(ugd->win_main, ugd);
		bg = _bt_create_bg(ugd->win_main, "group_list");
		elm_object_part_content_set(ugd->base, "elm.swallow.bg", bg);
	} else {
		ugd->base = __bt_create_frameview(ugd->win_main, ugd);
		bg = _bt_create_bg(ugd->win_main, "transparent");
		elm_object_part_content_set(ugd->base, "elm.swallow.bg", bg);
	}

	_bt_main_init_status(ugd, service);
	_bt_ipc_init_event_signal((void *)ugd);

	ugd->service = service;

	if (ugd->bt_launch_mode != BT_LAUNCH_VISIBILITY) {
		_bt_main_draw_list_view(ugd);
	} else {
		_bt_main_draw_visibility_view(ugd);
	}

	evas_object_show(ugd->base);

	ugd->ug_status = BT_UG_CREATE;

	FN_END;
	return ugd->base;
}

static void __on_start(ui_gadget_h ug, service_h service, void *priv)
{
	FN_START;

	int g_angle = 0;
	bt_ug_data *ugd = NULL;

	retm_if(priv == NULL, "Invalid argument: priv is NULL\n");

	ugd = priv;

	g_angle = elm_win_rotation_get(ugd->win_main);
	ugd->rotation = (int)(g_angle / 90);

	BT_DBG("rotation: %d", ugd->rotation);

	ugd->ug_status = BT_UG_START;

	FN_END;
}

static void __on_pause(ui_gadget_h ug, service_h service, void *priv)
{
	FN_START;
	bt_ug_data *ugd = NULL;

	retm_if(priv == NULL, "Invalid argument: priv is NULL\n");

	ugd = (bt_ug_data *)priv;

	if (ugd->op_status == BT_SEARCHING)
		bt_adapter_stop_device_discovery();

	ugd->ug_status = BT_UG_PAUSE;

	FN_END;
}

static void __on_resume(ui_gadget_h ug, service_h service, void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	retm_if(priv == NULL, "Invalid argument: priv is NULL\n");

	ugd = (bt_ug_data *)priv;

	ugd->ug_status = BT_UG_RESUME;

	if (ugd->visible_item)
		elm_genlist_item_update(ugd->visible_item);

	FN_END;
}

static void __on_destroy(ui_gadget_h ug, service_h service, void *priv)
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

	if (ugd->op_status == BT_SEARCHING) {
		err = bt_adapter_stop_device_discovery();
		if (err != BT_ERROR_NONE)
			BT_DBG("Stop device discovery failed: %d", err);
	}

	if (ugd->selectioninfo) {
		evas_object_del(ugd->selectioninfo);
		ugd->selectioninfo = NULL;
	}

	err = bt_adapter_unset_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_DBG("unset of state change cb  failed: %d", err);

	err = bt_adapter_unset_device_discovery_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_DBG("unset of device discovery state cb failed: %d", err);

	err = bt_device_unset_bond_created_cb();
	if (err != BT_ERROR_NONE)
		BT_DBG("unset of bond creation cb failed: %d", err);

	err = bt_device_unset_bond_destroyed_cb();
	if (err != BT_ERROR_NONE)
		BT_DBG("unset of bond destroyed cb failed: %d", err);

	err = bt_device_unset_service_searched_cb();
	if (err != BT_ERROR_NONE)
		BT_DBG("unset of service search cb failed: %d", err);

	err = bt_audio_unset_connection_state_changed_cb();
	if (err != BT_ERROR_NONE)
		BT_DBG("unset audio connection state cb failed: %d", err);

	err = bt_hid_host_deinitialize();
	if (err != BT_ERROR_NONE)
		BT_DBG("bt_hid_host_deinitialize failed: %d", err);

	err = bt_audio_deinitialize();
	if (err != BT_ERROR_NONE)
		BT_DBG("bt_audio_deinitialize failed: %d", err);

	err = bt_deinitialize();
	if (err != BT_ERROR_NONE)
		BT_DBG("bt_deinitialize failed: %d", err);

	err = _bt_destroy_net_connection(ugd->connection);
	if (err != BT_UG_ERROR_NONE)
		BT_DBG("_bt_destroy_net_connection failed: %d", err);

	err = _bt_ipc_unregister_popup_event_signal(ugd->EDBusHandle,
				(void *)ugd);
	if (err != BT_UG_ERROR_NONE)
		BT_DBG("_bt_ipc_unregister_popup_event_signal failed: %d", err);

	err = _bt_ipc_deinit_event_signal((void *)ugd);
	if (err != BT_UG_ERROR_NONE)
		BT_DBG("_bt_ipc_deinit_event_signal failed: %d", err);

	_bt_main_remove_callback(ugd);

	__bt_ug_release_memory(ugd);

	if (ugd->base) {
		evas_object_del(ugd->base);
		ugd->base = NULL;
	}

	FN_END;
}

static void __on_message(ui_gadget_h ug, service_h msg, service_h service,
			 void *priv)
{
	FN_START;
	FN_END;
}

static void __on_event(ui_gadget_h ug, enum ug_event event, service_h service,
		       void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	ret_if(priv == NULL);

	ugd = (bt_ug_data *)priv;

	BT_DBG("event: %d", event);

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
	case UG_EVENT_ROTATE_PORTRAIT:
		ugd->rotation = BT_ROTATE_PORTRAIT;
		break;
	case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
		ugd->rotation = BT_ROTATE_PORTRAIT_UPSIDEDOWN;
		break;
	case UG_EVENT_ROTATE_LANDSCAPE:
		ugd->rotation = BT_ROTATE_LANDSCAPE;
		break;
	case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		ugd->rotation = BT_ROTATE_LANDSCAPE_UPSIDEDOWN;
		break;
	default:
		break;
	}

	if (event == UG_EVENT_ROTATE_PORTRAIT ||
	     event == UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN ||
	      event == UG_EVENT_ROTATE_LANDSCAPE ||
	       event == UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN) {
		_bt_rotate_selectioninfo(ugd->selectioninfo, ugd->rotation);
		_bt_main_change_rotate_mode((void *)ugd);
		_bt_profile_change_rotate_mode((void *)ugd);
	}

	FN_END;
}

static void __on_key_event(ui_gadget_h ug, enum ug_key_event event,
			   service_h service, void *priv)
{
	FN_START;

	bt_ug_data *ugd = NULL;

	BT_DBG("event: %d\n", event);

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

UG_MODULE_API int setting_plugin_reset(service_h service, void *priv)
{
	FN_START;

	int ret;
	int result = 0;

	ret = bt_initialize();

	if (ret != BT_ERROR_NONE) {
		BT_DBG("Fail to init BT %d", ret);
		return ret;
	}

	ret = bt_adapter_reset();

	if (ret != BT_ERROR_NONE) {
		BT_DBG("Fail to reset adapter: %d", ret);
		result = ret;
	}

	ret = bt_deinitialize();

	if (ret != BT_ERROR_NONE)
		BT_DBG("Fail to deinit BT: %d", ret);

	FN_END;
	return result;
}

void _bt_ug_destroy(void *data, void *result)
{
	FN_START;
	int ret = 0;
	bt_ug_data *ugd = NULL;

	ret_if(data == NULL);

	ugd = (bt_ug_data *)data;

	if (result != NULL) {
		ug_send_result(ugd->ug, result);
	}

	if (ugd->ug)
		ret = ug_destroy_me(ugd->ug);

	if (ret < 0)
		BT_DBG("Fail to destroy me");

	FN_END;
}

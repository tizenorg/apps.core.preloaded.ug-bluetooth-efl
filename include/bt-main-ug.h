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

#ifndef __BT_MAIN_UG_H__
#define __BT_MAIN_UG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <libintl.h>
#include <appcore-efl.h>
#include <Elementary.h>
#include <efl_extension.h>
#include <ui-gadget-module.h>
#include <dlog.h>
#include <E_DBus.h>
#include <bundle.h>
#include <gio/gio.h>

#include "bt-type-define.h"
#include "bt-profile-view.h"

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#define PKGNAME "ug-setting-bluetooth-efl"

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "/usr/ug/"

#define LOCALEDIR PREFIX"/res/locale"

#define BT_EDJ_PATH PREFIX"/res/edje/ug-setting-bluetooth-efl"

#define _EDJ(o)			elm_layout_edje_get(o)
#define BT_ICON_EDJ BT_EDJ_PATH"/bluetooth_images.edj"
#define BT_GENLIST_EDJ BT_EDJ_PATH"/bluetooth_genlist.edj"

#ifdef _
#undef _
#endif
#define _(s)			dgettext(PKGNAME, s)

#define dgettext_noop(s)	(s)

#ifdef N_
#undef N_
#endif
#define N_(s)			dgettext_noop(s)

#define BT_UG_VCONF_PRINTSETTING		"memory/bluetooth/printsetting"

typedef struct {
	/* UI gadget data */
	ui_gadget_h ug;
	Evas_Object *base;
	Evas_Object *win_main;
	Elm_Theme *theme;

	bt_launch_mode_t bt_launch_mode;

	/* Request timer */
	Ecore_Timer *request_timer;

	/*************************
	*          Main View objects
	************************ */
	Evas_Object *navi_bar;
	Evas_Object *main_layout;
	Elm_Object_Item *navi_it;
	Elm_Object_Item *service_navi_it;
	/* Genlist */
	Evas_Object *main_genlist;


	/* Paired / Searched devices */
	Evas_Object *paired_dlggrp;
	Evas_Object *searched_dlggrp;

	/* Selected device's genlist items */
	Elm_Object_Item *paired_item;
	Elm_Object_Item *searched_item;

	/* Help object */
	Evas_Object *help_more_popup;
	Evas_Object *help_popup;

	/* Rename Device object*/
	Evas_Object *rename_popup;
	Evas_Object *rename_entry;
	Evas_Object *rename_button;

	/* Visibility object */
	Evas_Object *visibility_popup;

	/* Paired / Searched device list */
	Eina_List *paired_device;
	Eina_List *searched_device;

	/* Network profile list */
	Eina_List *net_profile_list;

	/* Button */
	Evas_Object *onoff_btn;
	Evas_Object *scan_btn;
	Evas_Object *radio_main;

	/* Genlist Item class */
	Elm_Genlist_Item_Class *device_name_itc;
	Elm_Genlist_Item_Class *rename_entry_itc;
	Elm_Genlist_Item_Class *rename_desc_itc;
	Elm_Genlist_Item_Class *visible_itc;
	Elm_Genlist_Item_Class *paired_title_itc;
	Elm_Genlist_Item_Class *searched_title_itc;
	Elm_Genlist_Item_Class *timeout_value_itc;
	Elm_Genlist_Item_Class *searched_device_itc;
	Elm_Genlist_Item_Class *no_device_itc;
	Elm_Genlist_Item_Class *paired_device_itc;
	Elm_Genlist_Item_Class *on_itc;
	Elm_Genlist_Item_Class *off_itc;

	/* Genlist Items */
	Elm_Object_Item *onoff_item;
	Elm_Object_Item *device_name_item;
	Elm_Object_Item *visible_item;
	Elm_Object_Item *paired_title;
	Elm_Object_Item *searched_title;
	Elm_Object_Item *no_device_item;
	Elm_Object_Item *visible_exp_item[BT_MAX_TIMEOUT_ITEMS + 1];
	Elm_Object_Item *empty_status_item;
	Elm_Object_Item *rename_entry_item;

	/*************************
	*           Popup objects
	************************ */
	Evas_Object *popup;
	bt_popup_data popup_data;
	Evas_Object *popup_menu;

	/*************************
	*          Status Variables
	************************ */
	bool waiting_service_response;
	bool disconn_req;
	bool connect_req;
	bool aul_launching_req;
	bool aul_pairing_req;
	bool is_discovery_started;
	unsigned int op_status;
	unsigned int ug_status;
	unsigned int help_status;
	unsigned int search_type;

	/*************************
	*          Grobal variables
	************************ */
	app_control_h service;
	bundle *popup_bundle;
	void *handle;
	char phone_name[BT_GLOBALIZATION_STR_LENGTH];
	int selected_radio;
	int remain_time;
	time_t start_time;
	bool visible;
	bool visibility_changed_by_ug;
	int timeout_id;
	int popup_timer;
	int visibility_timeout;
	bt_dev_t *pick_device;
	void *connection;
	bool is_popup_exist;

	GDBusConnection *g_conn;
	guint gdbus_owner_id;

	/* End key callback */
	bt_app_back_cb back_cb;

	Ecore_Timer *network_timer;
	/*************************
	*           Profile  View Data
	************************ */
	bt_profile_view_data *profile_vd;
	bt_confirm_req_t confirm_req;

	void *dpm_handle;
	void *dpm_policy_handle;
	int dpm_callback_id;
} bt_ug_data;


typedef enum _bt_cfg_item_reset_type {
	BT_CFG_ITEM_UNRESETABLE = 0,
	BT_CFG_ITEM_RESETABLE = 1,
} bt_cfg_item_reset_type;

typedef enum _bt_cfg_item_type {
	BT_CFG_ITEM_NODE_ERROR = 0,	/** Error */
	BT_CFG_ITEM_UG_NODE = 1,		/** general UG */
	BT_CFG_ITEM_UI_NODE = 2,		/** no UG, no app launching, just menu name */
	BT_CFG_ITEM_TITLE_NODE = 3,	/** view name */
	BT_CFG_ITEM_APP_NODE = 4,		/** app type - by launcher - read doc '4' */
	BT_CFG_ITEM_VIEW_NODE = 5,		/** view name - 2depth search */
} bt_cfg_item_type;

typedef struct {
	char *key_name;				/** key name */
	char *icon_path;			/** icon path */
	char *ug_args;				/** UG path or hyperlink */
	int pos;				/** position : 1st, 2st -- deprecated */
	bt_cfg_item_reset_type reset_type;	/** if ug supports Reset function */
	int click_times;			/** UG menu need to check */
	bt_cfg_item_type item_type;		/** 0:item 	1:header title */
	void *data;
} bt_setting_cfg_node_t;

void _bt_ug_destroy(void *data, void *result);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_MAIN_UG_H__ */

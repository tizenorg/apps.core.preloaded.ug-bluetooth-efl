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

#ifndef __BT_MAIN_UG_H__
#define __BT_MAIN_UG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <libintl.h>
#include <appcore-efl.h>
#include <Elementary.h>
#include <ui-gadget-module.h>
#include <dlog.h>
#include <E_DBus.h>
#include <bundle.h>

#include "bt-type-define.h"
#include "bt-profile-view.h"

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#define PKGNAME "ug-setting-bluetooth-efl"

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "/opt/ug/"

#define LOCALEDIR PREFIX"res/locale"

#define _EDJ(o)			elm_layout_edje_get(o)

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

	/* Navigation Controlbar */
	Elm_Object_Item *scan_item; /* Left Controlbar */

	/* Genlist */
	Evas_Object *main_genlist;

	Evas_Object *selectioninfo;

	/* Paired / Searched devices */
	Evas_Object *paired_dlggrp;
	Evas_Object *searched_dlggrp;

	/* Selected device's genlist items */
	Elm_Object_Item *paired_item;
	Elm_Object_Item *searched_item;

	/* Paired / Searched device list */
	Eina_List *paired_device;
	Eina_List *searched_device;

	/* Button */
	Evas_Object *onoff_btn;
	Evas_Object *radio_main;

	/* Genlist Item class */
	Elm_Genlist_Item_Class *sp_itc;
	Elm_Genlist_Item_Class *status_itc;
	Elm_Genlist_Item_Class *visible_itc;
	Elm_Genlist_Item_Class *paired_title_itc;
	Elm_Genlist_Item_Class *searched_title_itc;
	Elm_Genlist_Item_Class *timeout_value_itc;
	Elm_Genlist_Item_Class *searched_itc;
	Elm_Genlist_Item_Class *no_device_itc;
	Elm_Genlist_Item_Class *device_itc;
	Elm_Genlist_Item_Class *end_itc;

	/* Genlist Items */
	Elm_Object_Item *status_item;
	Elm_Object_Item *visible_item;
	Elm_Object_Item *paired_title;
	Elm_Object_Item *searched_title;
	Elm_Object_Item *no_device_item;
	Elm_Object_Item *paired_padding;
	Elm_Object_Item *searched_padding;

	/*************************
	*           Popup objects
	************************ */
	Evas_Object *popup;
	Evas_Object *popup_menu;

	/*************************
	*          Status Variables
	************************ */
	bool waiting_service_response;
	bool auto_service_search;
	bool connect_req;
	bool search_req;
	bool aul_launching_req;
	bool aul_pairing_req;
	bool syspoup_req;
	unsigned int op_status;
	unsigned int ug_status;
	unsigned int search_type;

	/*************************
	*          Grobal variables
	************************ */
	Elm_Genlist_Item_Class popup_menu_itc;
	service_h service;
	bundle *popup_bundle;
	char phone_name[BT_DEVICE_NAME_LENGTH_MAX + 1];
	int unbonding_count;
	int selected_radio;
	int remain_time;
	bool visible;
	int timeout_id;
	int popup_timer;
	int visibility_timeout;
	bt_rotate_mode_t rotation;
	bt_dev_t *pick_device;

	/* IPC handler */
	E_DBus_Connection *EDBusHandle;
	E_DBus_Signal_Handler *sh;
	E_DBus_Signal_Handler *popup_sh;

	/* Dbus connection / proxy */
	void *conn;

	/* End key callback */
	bt_app_back_cb back_cb;

	/*************************
	*           Profile  View Data
	************************ */
	bt_profile_view_data *profile_vd;
} bt_ug_data;

void _bt_ug_destroy(void *data, void *result);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_MAIN_UG_H__ */

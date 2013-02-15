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

#ifndef __BT_MAIN_VIEW_H__
#define __BT_MAIN_VIEW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

#include "bt-type-define.h"
#include "bt-main-ug.h"

typedef struct _bt_radio_item bt_radio_item;

struct _bt_radio_item {
	Elm_Object_Item *it;	/* Genlist Item pointer */
	int index;		/* Index*/
	void *ugd;
};

void _bt_main_phone_name_changing_btn_cb(void *data, Evas_Object *obj,
						 void *event_info);

void _bt_main_popup_del_cb(void *data, Evas_Object *obj,
				    void *event_info);

void _bt_main_selectioninfo_hide_cb(void *data, Evas * e,
					Evas_Object *obj, void *event_info);

int _bt_main_draw_list_view(bt_ug_data *ugd);

int _bt_main_draw_visibility_view(bt_ug_data *ugd);

void _bt_main_draw_popup_menu(Evas_Object *parent, bt_dev_t *dev,
				bt_ug_data *ugd);

void _bt_main_draw_paired_devices(bt_ug_data *ugd);

Elm_Object_Item *_bt_main_add_paired_device(bt_ug_data *ugd,
					bt_dev_t *dev);

Elm_Object_Item *_bt_main_add_searched_device(bt_ug_data *ugd,
						bt_dev_t *dev);

Elm_Object_Item *_bt_main_add_no_device_found(bt_ug_data *ugd);

void _bt_main_remove_paired_device(bt_ug_data *ugd, bt_dev_t *dev);

void _bt_main_remove_searched_device(bt_ug_data *ugd, bt_dev_t *dev);

void _bt_main_remove_all_paired_devices(bt_ug_data *ugd);

void _bt_main_remove_all_searched_devices(bt_ug_data *ugd);

gboolean _bt_main_is_headset_connected(bt_ug_data *ugd);

gboolean _bt_main_is_stereo_headset_connected(bt_ug_data *ugd);

bt_dev_t *_bt_main_get_dev_info(Eina_List *list,
				Elm_Object_Item *genlist_item);

bt_dev_t *_bt_main_get_dev_info_by_address(Eina_List *list,
						char *address);

void _bt_main_retry_pairing(void *data, int response);

void _bt_main_change_rotate_mode(void *data);

void _bt_main_change_connection_status(bool connected, bt_ug_data *ugd,
					bt_dev_t *dev);

void _bt_main_retry_connection(void *data, int response);

void _bt_main_connect_device(bt_ug_data *ugd, bt_dev_t *dev);

void _bt_main_disconnect_device(bt_ug_data *ugd, bt_dev_t *dev);

int _bt_main_request_pairing_with_effect(bt_ug_data *ugd,
					Elm_Object_Item *seleted_item);

void _bt_main_init_status(bt_ug_data *ugd, void *data);

bt_dev_t *_bt_main_create_paired_device_item(void *data);

bt_dev_t *_bt_main_create_searched_device_item(void *data);

void _bt_main_get_paired_device(bt_ug_data *ugd);

void _bt_main_scan_device(bt_ug_data *ugd);

void _bt_main_draw_selection_info(bt_ug_data *ugd, char *message);

int _bt_main_service_request_cb(void *data);

char *_bt_main_get_device_icon(int major_class, int minor_class, int connected);

int _bt_main_check_and_update_device(Eina_List *list,
					char *addr, char *name);

void _bt_main_launch_syspopup(void *data, char *event_type,
				char *title, char *type);

gboolean _bt_main_is_matched_profile(unsigned int search_type,
				unsigned int major_class,
				unsigned int service_class,
				service_h service);

void _bt_main_create_information_popup(bt_ug_data *ugd, char *msg);

void _bt_main_add_searched_title(bt_ug_data *ugd);

void _bt_update_paired_item_style(bt_ug_data *ugd);

void _bt_update_searched_item_style(bt_ug_data *ugd);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_MAIN_VIEW_H__ */

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

int _bt_main_enable_bt(void *data);

int _bt_main_disable_bt(void *data);

void _bt_main_phone_name_changing_btn_cb(void *data, Evas_Object *obj,
						 void *event_info);

void _bt_main_popup_del_cb(void *data, Evas_Object *obj, void *event_info);

void _bt_back_btn_popup_del_cb(void *data, Evas_Object *obj, void *event_info);

void _bt_main_create_information_popup(void *data, char *msg);

int _bt_main_draw_list_view(bt_ug_data *ugd);

int _bt_main_draw_onoff_view(bt_ug_data *ugd);

int _bt_main_draw_visibility_view(bt_ug_data *ugd);

void _bt_main_draw_paired_devices(bt_ug_data *ugd);

Elm_Object_Item *_bt_main_add_paired_device_on_bond(bt_ug_data *ugd,
					bt_dev_t *dev);

Elm_Object_Item *_bt_main_add_paired_device(bt_ug_data *ugd,
					bt_dev_t *dev);

Elm_Object_Item *_bt_main_add_searched_device(bt_ug_data *ugd,
						bt_dev_t *dev);

Elm_Object_Item *_bt_main_add_no_device_found(bt_ug_data *ugd);

void _bt_main_remove_paired_device(bt_ug_data *ugd, bt_dev_t *dev);

void _bt_sort_paired_device_list(bt_ug_data *ugd, bt_dev_t *dev, int connected);

void _bt_main_remove_searched_device(bt_ug_data *ugd, bt_dev_t *dev);

void _bt_main_remove_all_paired_devices(bt_ug_data *ugd);

void _bt_main_remove_all_searched_devices(bt_ug_data *ugd);

bt_dev_t *_bt_main_get_dev_info(Eina_List *list,
				Elm_Object_Item *genlist_item);

bt_dev_t *_bt_main_get_dev_info_by_address(Eina_List *list,
						char *address);

gboolean _bt_main_is_connectable_device(bt_dev_t *dev);

void _bt_main_connect_device(bt_ug_data *ugd, bt_dev_t *dev);

void _bt_main_disconnect_device(bt_ug_data *ugd, bt_dev_t *dev);

int _bt_main_request_pairing_with_effect(bt_ug_data *ugd,
					Elm_Object_Item *seleted_item);

void _bt_main_init_status(bt_ug_data *ugd, void *data);

bt_dev_t *_bt_main_create_paired_device_item(void *data);

bt_dev_t *_bt_main_create_searched_device_item(void *data);

void _bt_main_get_paired_device(bt_ug_data *ugd);

void _bt_main_scan_device(bt_ug_data *ugd);

int _bt_main_service_request_cb(void *data);

char *_bt_main_get_device_icon(int major_class, int minor_class,
				int connected, gboolean highlighted);

int _bt_main_check_and_update_device(Eina_List *list,
					char *addr, char *name);

void _bt_main_launch_syspopup(void *data, char *event_type,
				char *title, char *type);

gboolean _bt_main_is_matched_profile(unsigned int search_type,
				unsigned int major_class,
				unsigned int service_class,
				app_control_h service,
				unsigned int minor_class);

void _bt_main_add_searched_title(bt_ug_data *ugd);

void _bt_main_add_device_name_item(bt_ug_data *ugd, Evas_Object *genlist);

void _bt_main_add_visible_item(bt_ug_data *ugd, Evas_Object *genlist);

void _bt_update_device_list(bt_ug_data *ugd);

Evas_Object * _bt_main_create_scan_button(bt_ug_data *ugd);

int _bt_idle_destroy_ug(void *data);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_MAIN_VIEW_H__ */

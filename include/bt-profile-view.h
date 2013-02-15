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

#ifndef __BT_PROFILE_VIEW_H__
#define __BT_PROFILE_VIEW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Elementary.h>

#include "bt-type-define.h"

typedef struct _bt_profile_view_data bt_profile_view_data;

struct _bt_profile_view_data {
	Evas_Object *win_main;
	Evas_Object *navi_bar;
	Evas_Object *layout;
	Evas_Object *genlist;
	Evas_Object *save_btn;
	Evas_Object *cancel_btn;
	Elm_Object_Item *navi_it;
	Elm_Object_Item *name_item;
	Elm_Object_Item *unpair_item;
	Elm_Object_Item *title_item;
	Elm_Genlist_Item_Class *name_itc;
	Elm_Genlist_Item_Class *unpair_itc;
	Elm_Genlist_Item_Class *title_itc;
	Elm_Genlist_Item_Class *call_itc;
	Elm_Genlist_Item_Class *media_itc;
	Elm_Genlist_Item_Class *hid_itc;
	Elm_Genlist_Item_Class *network_itc;
};

void _bt_profile_create_view(bt_dev_t *dev_info);

void _bt_profile_delete_view(void *data);

#ifdef __cplusplus
}
#endif
#endif /* __BT_PROFILE_VIEW_H__ */

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

#ifndef __BT_WIDGET_H__
#define __BT_WIDGET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "Elementary.h"
#include <efl_assist.h>

Evas_Object *_bt_create_naviframe(Evas_Object *parent);

Evas_Object *_bt_create_icon(Evas_Object *parent, char *img);

Evas_Object *_bt_create_progressbar(Evas_Object *parent,
				const char *style);

void _bt_update_genlist_item(Elm_Object_Item *item);

Evas_Object *_bt_main_base_layout_create(Evas_Object *parent, void *data);

void _bt_set_popup_text(void *data, Evas_Object *popup);

Evas_Object *_bt_create_popup(void *data, void *cb, void *cb_data,
					int timer_val);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_WIDGET_H__ */

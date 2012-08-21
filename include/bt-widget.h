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

#ifndef __BT_WIDGET_H__
#define __BT_WIDGET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "Elementary.h"

Evas_Object *_bt_create_naviframe(Evas_Object *parent);

Evas_Object *_bt_create_icon(Evas_Object *parent, char *img);

Evas_Object *_bt_create_button(Evas_Object *parent, char *style,
				char *text, char *icon_path,
				Evas_Smart_Cb func, void *data);

Evas_Object *_bt_modify_button(Evas_Object *btn, char *style,
				char *text, char *icon_path);

Evas_Object *_bt_create_onoff_toggle(Evas_Object *parent,
				Evas_Smart_Cb func, void *data);

Evas_Object *_bt_create_progressbar(Evas_Object *parent,
				const char *style);

Evas_Object *_bt_create_genlist(Evas_Object *parent);

Evas_Object *_bt_create_separator(Evas_Object *parent,
				const char *style);

Evas_Object *_bt_create_box(Evas_Object *parent);

Evas_Object *_bt_create_scroller(Evas_Object *parent,
				Evas_Object *ly);

Evas_Object *_bt_create_controlbar(Evas_Object *parent, char *style);

Evas_Object *_bt_create_bg(Evas_Object *parent, char *style);

Evas_Object *_bt_create_layout(Evas_Object *parent, char *edj,
				char *content);

Evas_Object *_bt_create_conformant(Evas_Object *parent,
				   Evas_Object *content);

Evas_Object *_bt_create_popup(Evas_Object *parent, char *title,
				char *text, void *cb, void *cb_data,
				int timer_val);

Evas_Object *_bt_create_selectioninfo(Evas_Object *parent, char *text,
					int rotation, void *cb,
					void *cb_data,int timeout);

void _bt_rotate_selectioninfo(Evas_Object *selectioninfo, int rotation);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_WIDGET_H__ */

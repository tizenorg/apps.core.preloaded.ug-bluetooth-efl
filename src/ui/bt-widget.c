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

#include "bt-debug.h"
#include "bt-widget.h"
#include "bt-main-ug.h"
#include "bt-type-define.h"
#include "bt-string-define.h"

#define COLOR_TABLE "/usr/apps/ug-bluetooth-efl/res/tables/ug-bluetooth-efl_ChangeableColorTable.xml"
#define FONT_TABLE "/usr/apps/ug-bluetooth-efl/res/tables/ug-bluetooth-efl_FontInfoTable.xml"

Evas_Object *_bt_create_naviframe(Evas_Object *parent)
{
	FN_START;

	Evas_Object *nf;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL");

	nf = elm_naviframe_add(parent);
	elm_object_part_content_set(parent, "elm.swallow.content", nf);
	evas_object_show(nf);

	FN_END;

	return nf;
}

Evas_Object *_bt_create_icon(Evas_Object *parent, char *img)
{
	Evas_Object *ic;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL");
	retvm_if(img == NULL, NULL, "Invalid argument: img is NULL");
	retvm_if(strlen(img) >= BT_IMG_PATH_MAX, NULL, "image path is wrong [%s]", img);

	ic = elm_image_add(parent);

	elm_image_file_set(ic, BT_ICON_EDJ, img);
	evas_object_size_hint_align_set(ic, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(ic, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
/*
	evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL,
					 1, 1);
	elm_image_resizable_set(ic, 1, 1);
*/
	evas_object_show(ic);

	return ic;
}

Evas_Object *_bt_create_progressbar(Evas_Object *parent, const char *style)
{
	FN_START;

	Evas_Object *progress_bar = NULL;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL");

	progress_bar = elm_progressbar_add(parent);

	if (style)
		elm_object_style_set(progress_bar, style);
	else
		elm_object_style_set(progress_bar, "process_large");

	elm_progressbar_horizontal_set(progress_bar, EINA_TRUE);
	evas_object_propagate_events_set(progress_bar, EINA_FALSE);
	evas_object_show(progress_bar);
	elm_progressbar_pulse(progress_bar, EINA_TRUE);

	FN_END;

	return progress_bar;
}

void _bt_update_genlist_item(Elm_Object_Item *item)
{
	ret_if(!item);
	elm_genlist_item_fields_update(item, "*",
					ELM_GENLIST_ITEM_FIELD_TEXT);
	elm_genlist_item_fields_update(item, "*",
					ELM_GENLIST_ITEM_FIELD_CONTENT);
}

static Evas_Object *__bt_create_bg(Evas_Object *parent, char *style)
{
	FN_START;

	Evas_Object *bg;

	retvm_if(!parent, NULL, "Invalid argument: parent is NULL");

	bg = elm_bg_add(parent);
	retv_if(!bg, NULL);

	if (style)
		elm_object_style_set(bg, style);

	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
	evas_object_show(bg);

	FN_END;

	return bg;
}

Evas_Object *_bt_main_base_layout_create(Evas_Object *parent, void *data)
{
	FN_START;

	Evas_Object *layout = NULL;

	layout = elm_layout_add(parent);
	retv_if(!layout, NULL);

	elm_layout_theme_set(layout, "layout", "application", "default");

	Evas_Object *bg = __bt_create_bg(layout, "default");
	elm_object_part_content_set(layout, "elm.swallow.bg", bg);
	evas_object_show(layout);

	FN_END;
	return layout;
}

void _bt_set_popup_text(void *data, Evas_Object *popup)
{
	FN_START;
	ret_if(!data);
	ret_if(!popup);
	bt_ug_data *ugd = (bt_ug_data *)data;

	char *temp = NULL;
	char *markup_text = NULL;
	switch(ugd->popup_data.type) {
		case BT_POPUP_PAIRING_ERROR :
			if (ugd->popup_data.data) {
				temp = g_strdup_printf(
					BT_STR_UNABLE_TO_PAIR_WITH_PS,
					ugd->popup_data.data);
			}
			break;
		case BT_POPUP_CONNECTION_ERROR :
			if (ugd->popup_data.data) {
				temp = g_strdup_printf(
					BT_STR_UNABLE_TO_CONNECT_TO_PS,
					ugd->popup_data.data);
			}
			break;
		case BT_POPUP_DISCONNECT :
			if (ugd->popup_data.data) {
				temp = g_strdup_printf(
					BT_STR_END_CONNECTION,
					ugd->popup_data.data);
			}
			break;
		case BT_POPUP_GET_SERVICE_LIST_ERROR :
		case BT_POPUP_GETTING_SERVICE_LIST :
		case BT_POPUP_ENTER_DEVICE_NAME :
		case BT_POPUP_LOW_BATTERY :
		default :
			break;
	}

	if (temp) {
		markup_text = elm_entry_utf8_to_markup(temp);
		elm_object_text_set(popup, markup_text);
		free(markup_text);
		g_free(temp);
	}
}

Evas_Object *_bt_create_popup(void *data, void *cb,
					void *cb_data, int timer_val)
{
	FN_START;
	retvm_if(!data, NULL, "Invalid argument: ugd is NULL");
	bt_ug_data *ugd = (bt_ug_data *)data;
	retvm_if(!ugd->win_main, NULL, "Invalid argument: win_main is NULL");

	Evas_Object *popup = NULL;

	popup = elm_popup_add(ugd->win_main);
	retvm_if(!popup, NULL, "fail to create popup");

	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	/* Set title text */
	switch(ugd->popup_data.type) {
		case BT_POPUP_PAIRING_ERROR :
		case BT_POPUP_GET_SERVICE_LIST_ERROR :
		case BT_POPUP_CONNECTION_ERROR :
		case BT_POPUP_ENTER_DEVICE_NAME :
		case BT_POPUP_LOW_BATTERY :
			elm_object_domain_translatable_part_text_set(
				popup, "title,text",
				PKGNAME, "IDS_BT_HEADER_BLUETOOTH_ERROR_ABB");
			break;

		case BT_POPUP_GETTING_SERVICE_LIST :
			elm_object_domain_translatable_part_text_set(
				popup, "title,text",
				PKGNAME, "IDS_BT_HEADER_BLUETOOTH");
			break;
		case BT_POPUP_DISCONNECT :
			elm_object_domain_translatable_part_text_set(
				popup, "title,text",
				PKGNAME, "IDS_BT_HEADER_DISCONNECT_DEVICE_ABB");
			break;
	}

	/* Set content text */
	if (ugd->popup_data.type == BT_POPUP_GET_SERVICE_LIST_ERROR)
		elm_object_domain_translatable_text_set(
			ugd->popup, PKGNAME,
			"IDS_BT_POP_UNABLE_TO_GET_SERVICE_LIST");
	else if (ugd->popup_data.type == BT_POPUP_GETTING_SERVICE_LIST)
		elm_object_domain_translatable_text_set(
			ugd->popup, PKGNAME,
			"IDS_BT_POP_GETTINGSERVICELIST");
	else if (ugd->popup_data.type == BT_POPUP_ENTER_DEVICE_NAME)
		elm_object_domain_translatable_text_set(
			ugd->popup, PKGNAME,
			"IDS_ST_POP_ENTER_DEVICE_NAME");
	else if (ugd->popup_data.type == BT_POPUP_LOW_BATTERY)
		elm_object_domain_translatable_text_set(
			ugd->popup, PKGNAME,
			"IDS_ST_BODY_LEDOT_LOW_BATTERY");
	else
		_bt_set_popup_text(ugd, popup);

	/* Set callback */
	if (cb != NULL)
		evas_object_smart_callback_add(popup, "block,clicked",
					       (Evas_Smart_Cb) cb, cb_data);

	/* Set timer value */
	if (timer_val != 0)
		elm_popup_timeout_set(popup, (double)timer_val);

	FN_END;
	return popup;
}


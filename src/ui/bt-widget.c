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

#include "bt-debug.h"
#include "bt-widget.h"
#include "bt-main-ug.h"
#include "bt-type-define.h"

Evas_Object *_bt_create_naviframe(Evas_Object *parent)
{
	FN_START;

	Evas_Object *nf;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	nf = elm_naviframe_add(parent);
	elm_object_part_content_set(parent, "elm.swallow.content", nf);
	evas_object_show(nf);

	FN_END;

	return nf;
}

Evas_Object *_bt_create_icon(Evas_Object *parent, char *img)
{
	FN_START;

	Evas_Object *ic;
	char buf[BT_IMG_PATH_MAX] = { 0, };

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");
	retvm_if(img == NULL, NULL, "Invalid argument: img is NULL\n");

	if (strlen(img) >= BT_IMG_PATH_MAX) {
		BT_DBG("image path is wrong [%s]", img);
	}

	ic = elm_image_add(parent);
	snprintf(buf, BT_IMG_PATH_MAX, "%s", img);
	elm_image_file_set(ic, buf, NULL);
	evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL,
					 1, 1);
	elm_image_resizable_set(ic, 1, 1);

	evas_object_show(ic);

	FN_END;

	return ic;
}

Evas_Object *_bt_create_button(Evas_Object *parent, char *style, char *text,
			       char *icon_path, Evas_Smart_Cb func, void *data)
{
	FN_START;

	Evas_Object *btn = NULL;
	Evas_Object *icon = NULL;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");
	btn = elm_button_add(parent);

	if (style)
		elm_object_style_set(btn, style);

	if (icon_path) {
		icon = elm_image_add(btn);
		elm_image_file_set(icon, icon_path, NULL);
		elm_object_part_content_set(btn, "elm.icon", icon);
	}

	if (text)
		elm_object_text_set(btn, text);

	if (func)
		evas_object_smart_callback_add(btn, "clicked", func, data);

	evas_object_show(btn);

	FN_END;

	return btn;
}

Evas_Object *_bt_modify_button(Evas_Object *btn, char *style, char *text,
			       char *icon_path)
{
	FN_START;

	Evas_Object *icon = NULL;

	retvm_if(btn == NULL, NULL, "Invalid argument: parent is NULL\n");

	if (style)
		elm_object_style_set(btn, style);

	if (icon_path) {
		icon = elm_image_add(btn);
		elm_image_file_set(icon, icon_path, NULL);
		elm_object_part_content_set(btn, "elm.icon", icon);
	} else {
		elm_object_part_content_set(btn, "elm.icon", NULL);
	}

	if (text)
		elm_object_text_set(btn, text);

	evas_object_show(btn);

	FN_END;

	return btn;
}

Evas_Object *_bt_create_progressbar(Evas_Object *parent, const char *style)
{
	FN_START;

	Evas_Object *progress_bar = NULL;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	progress_bar = elm_progressbar_add(parent);

	if (style)
		elm_object_style_set(progress_bar, style);
	else
		elm_object_style_set(progress_bar, "list_process");

	evas_object_show(progress_bar);
	elm_progressbar_pulse(progress_bar, EINA_TRUE);

	FN_END;

	return progress_bar;
}

Evas_Object *_bt_create_genlist(Evas_Object *parent)
{
	FN_START;

	Evas_Object *genlist = NULL;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	genlist = elm_genlist_add(parent);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL,
					EVAS_HINT_FILL);
	evas_object_show(genlist);

	FN_END;
	return genlist;
}

Evas_Object *_bt_create_separator(Evas_Object *parent, const char *style)
{
	FN_START;

	Evas_Object *separator = NULL;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	separator = elm_separator_add(parent);
	elm_separator_horizontal_set(separator, EINA_TRUE);
	elm_object_style_set(separator, style);
	evas_object_show(separator);

	FN_END;
	return separator;
}

Evas_Object *_bt_create_box(Evas_Object *parent)
{
	FN_START;

	Evas_Object *box = NULL;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	box = elm_box_add(parent);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_align_set(box, 0.0, 0.0);
	evas_object_show(box);

	FN_END;
	return box;
}

Evas_Object *_bt_create_controlbar(Evas_Object *parent, char *style)
{
	FN_START;

	Evas_Object *toolbar = NULL;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	toolbar = elm_toolbar_add(parent);
	elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);

	if (style)
		elm_object_style_set(toolbar, style);

	evas_object_show(toolbar);

	FN_END;

	return toolbar;
}

Evas_Object *_bt_create_scroller(Evas_Object *parent, Evas_Object *ly)
{
	FN_START;

	Evas_Object *scroller;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	scroller = elm_scroller_add(parent);

	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF,
				ELM_SCROLLER_POLICY_AUTO);

	if (ly != NULL)
		elm_object_content_set(scroller, ly);

	evas_object_show(scroller);

	FN_END;

	return scroller;
}


Evas_Object *_bt_create_bg(Evas_Object *parent, char *style)
{
	FN_START;

	Evas_Object *bg;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	bg = elm_bg_add(parent);

	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);

	if (style)
		elm_object_style_set(bg, style);

	elm_win_resize_object_add(parent, bg);

	evas_object_show(bg);

	FN_END;

	return bg;
}

Evas_Object *_bt_create_layout(Evas_Object *parent, char *edj, char *content)
{
	FN_START;

	Evas_Object *layout;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	layout = elm_layout_add(parent);

	if (edj != NULL && content != NULL)
		elm_layout_file_set(layout, edj, content);
	else {
		elm_layout_theme_set(layout, "layout", "application",
				     "default");
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
	}

	evas_object_show(layout);

	FN_END;

	return layout;
}

Evas_Object *_bt_create_conformant(Evas_Object *parent, Evas_Object *content)
{
	FN_START;

	Evas_Object *conform = NULL;

	elm_win_conformant_set(parent, 1);
	conform = elm_conformant_add(parent);
	elm_object_style_set(conform, "internal_layout");
	evas_object_show(conform);

	if (content)
		elm_object_content_set(conform, content);

	FN_END;

	return conform;
}

Evas_Object *_bt_create_popup(Evas_Object *parent, char *title, char *text,
			      void *cb, void *cb_data, int timer_val)
{
	FN_START;
	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");

	Evas_Object *popup = NULL;

	popup = elm_popup_add(parent);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);

	if (title != NULL)
		elm_object_part_text_set(popup, "title,text", title);

	if (text != NULL)
		elm_object_text_set(popup, text);

	if (cb != NULL)
		evas_object_smart_callback_add(popup, "block,clicked",
					       (Evas_Smart_Cb) cb, cb_data);

	if (timer_val != 0)
		elm_popup_timeout_set(popup, (double)timer_val);

	evas_object_show(popup);

	FN_END;
	return popup;
}

Evas_Object *_bt_create_selectioninfo(Evas_Object *parent, char *text,
					int rotation, void *cb,
					void *cb_data,int timeout)
{
	FN_START;

	Evas_Object *selectioninfo = NULL;
	Evas_Object *layout = NULL;

	retv_if(parent == NULL, NULL);
	retv_if(text == NULL, NULL);

	/* Add notify */
	selectioninfo = elm_notify_add(parent);
	retv_if(selectioninfo == NULL, NULL);
	evas_object_size_hint_weight_set(selectioninfo,
					 EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);

	/* selectioninfo layout add */
	layout = elm_layout_add(selectioninfo);
	retv_if(layout == NULL, NULL);

	if (rotation == BT_ROTATE_LANDSCAPE ||
	     rotation == BT_ROTATE_LANDSCAPE_UPSIDEDOWN) {
		elm_layout_theme_set(layout,
				     "standard", "selectioninfo",
				     "horizontal/bottom_64");
	} else {
		elm_layout_theme_set(layout,
				     "standard", "selectioninfo",
				     "vertical/bottom_64");
	}

	/* selectioninfo layout content set to notify */
	elm_object_content_set(selectioninfo, layout);


	/* callback add */
	if (cb != NULL)
		evas_object_event_callback_add(selectioninfo,
					       EVAS_CALLBACK_HIDE,
					       cb,
					       cb_data);

	/* text set and timeout set */
	edje_object_part_text_set(_EDJ(layout), "elm.text", text);
	elm_notify_timeout_set(selectioninfo, timeout);
	evas_object_show(selectioninfo);

	FN_END;

	return selectioninfo;
}

void _bt_rotate_selectioninfo(Evas_Object *selectioninfo, int rotation)
{
	FN_START;

	Evas_Object *layout = NULL;
	const char *text = NULL;

	ret_if(selectioninfo == NULL);

	layout = elm_object_content_get(selectioninfo);
	ret_if(layout == NULL);

	text = edje_object_part_text_get(_EDJ(layout), "elm.text");
	ret_if(text == NULL);

	BT_DBG("rotation: %d", rotation);
	BT_DBG("text: %s", text);

	if (rotation == BT_ROTATE_LANDSCAPE ||
	     rotation == BT_ROTATE_LANDSCAPE_UPSIDEDOWN) {
		elm_layout_theme_set(layout,
				     "standard", "selectioninfo",
				     "horizontal/bottom_12");
	} else {
		elm_layout_theme_set(layout,
				     "standard", "selectioninfo",
				     "vertical/bottom_12");
	}

	edje_object_part_text_set(_EDJ(layout), "elm.text", text);
}

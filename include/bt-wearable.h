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

#ifndef __BT_WEARABLE_H
#define __BT_WEARABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bt-type-define.h"

#ifdef TIZEN_REDWOOD

gboolean _bt_handle_wearable_device(bt_ug_data *ugd, bt_dev_t *dev);

#endif

#ifdef __cplusplus
}
#endif
#endif /* __BT_WEARABLE_H */

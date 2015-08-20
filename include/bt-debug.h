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

#ifndef __BT_DEBUG_H
#define __BT_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dlog.h>
#include <stdio.h>
#include <string.h>

#undef LOG_TAG
#define LOG_TAG "BLUETOOTH_SETTING_UG"

#define BT_INFO(fmt, arg...) SLOGI(fmt, ##arg)

#define BT_ERR(fmt, arg...) SLOGE(fmt, ##arg)

#define BT_DBG(fmt, arg...) SLOGD(fmt, ##arg)


#define DBG_SECURE(fmt, args...) SECURE_SLOGD(fmt, ##args)
#define ERR_SECURE(fmt, args...) SECURE_SLOGE(fmt, ##args)
#define INFO_SECURE(fmt, args...) SECURE_SLOGI(fmt, ##args)

#define FUNCTION_TRACE
#ifdef FUNCTION_TRACE
#define	FN_START BT_DBG("[ENTER FUNC]");
#define	FN_END BT_DBG("[EXIT FUNC]");
#else
#define	FN_START
#define	FN_END
#endif

#define warn_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			BT_ERR("(%s) -> "fmt, #expr, ##arg); \
		} \
	} while (0);

#define ret_if(expr) \
	do { \
		if (expr) { \
			BT_ERR("(%s) return", #expr); \
			return; \
		} \
	} while (0);

#define retv_if(expr, val) \
	do { \
		if (expr) { \
			BT_ERR("(%s) return", #expr); \
			return (val); \
		} \
	} while (0);

#define retm_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			BT_ERR(fmt, ##arg); \
			BT_ERR("(%s) return", #expr); \
			return; \
		} \
	} while (0);

#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (expr) { \
			BT_ERR(fmt, ##arg); \
			BT_ERR("(%s) return", #expr); \
			return (val); \
		} \
	} while (0);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_DEBUG_H */

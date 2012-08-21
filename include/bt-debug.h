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

#ifndef __BT_DEBUG_H
#define __BT_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BT_USE_PLATFORM_DBG
#include <dlog.h>
#include <stdio.h>
#else
#include <stdio.h>
#endif
#include <string.h>

#define BT_LOG_TAG "BT_UG"

#ifdef BT_USE_PLATFORM_DBG
#define BT_INFO(fmt, arg...) \
		SLOG(LOG_INFO, BT_LOG_TAG, "%s:%d "fmt, \
			__func__, __LINE__, ##arg)

#define BT_ERR(fmt, arg...) \
		SLOG(LOG_DEBUG, BT_LOG_TAG, "%s:%d "fmt, \
			__func__, __LINE__, ##arg)

#define BT_DBG(fmt, arg...) \
		SLOG(LOG_DEBUG, BT_LOG_TAG, "%s:%d "fmt, \
			__func__, __LINE__, ##arg)

#define	FN_START \
	do {\
		SLOG(LOG_DEBUG, BT_LOG_TAG, \
			"\n\033[0;92m[ENTER FUNC]\033[0m : %s. \t%s:%d \n", \
			__FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);\
	} while (0);

#define	FN_END  \
	do {\
		SLOG(LOG_DEBUG, BT_LOG_TAG, \
			"\n\033[0;32m[EXIT FUNC]\033[0m : %s. \t%s:%d \n", \
			__FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);\
	} while (0);
#else
#define BT_INFO(fmt, arg...) \
	printf("[INFO] %s:%d \n"fmt, __func__, __LINE__, ##arg)

#define BT_ERR(fmt, arg...) \
	printf("[ERR] %s:%d \n"fmt, __func__, __LINE__, ##arg)

#define BT_DBG(fmt, arg...) \
	printf("[DEBUG] %s:%d \n"fmt, __func__, __LINE__, ##arg)

#define FN_START \
	do { \
		printf("\n[DEBUG] \033[0;92m[ENTER FUNC]\033" \
			"[0m : %s. \t%s:%d \n", \
			__FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__); \
	} while (0);

#define	FN_END  \
	do { \
		printf("\n[DEBUG] \033[0;32m[EXIT FUNC]\033" \
			"[0m : %s. \t%s:%d \n", \
			__FUNCTION__, strrchr(__FILE__, '/')+1, __LINE__);\
	} while (0);
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
			BT_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0);

#define retv_if(expr, val) \
	do { \
		if (expr) { \
			BT_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0);

#define retm_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			BT_ERR(fmt, ##arg); \
			BT_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0);

#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (expr) { \
			BT_ERR(fmt, ##arg); \
			BT_ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0);

#ifdef __cplusplus
}
#endif
#endif				/* __BT_DEBUG_H */

/*******************************************************************************
 * texturepack_extension - Log
 * Copyright (C) 2026 eternalfuture-e38299
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Author: eternalfuture-e38299
 * GitHub: https://github.com/eternalfuture-e38299
 * Created: 2026/6/7
 *******************************************************************************/


#pragma once


#define LOG_TAG "texturepack_extension"

#if  defined(__ANDROID__)
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#else

#include <cstdlib>

#define LOGD(...) printf(LOG_TAG "DEBUG: " __VA_ARGS__); printf("\n")
#define LOGI(...) printf(LOG_TAG "INFO: " __VA_ARGS__); printf("\n")
#define LOGW(...) printf(LOG_TAG "WARNING: " __VA_ARGS__); printf("\n")
#define LOGE(...) printf(LOG_TAG "ERROR: " __VA_ARGS__); printf("\n")

#endif

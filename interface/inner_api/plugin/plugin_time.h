/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HISTREAMER_PLUGIN_COMMON_TIME_H
#define HISTREAMER_PLUGIN_COMMON_TIME_H

#include <chrono>

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr int HST_TIME_NONE = (int64_t)-1;
constexpr int HST_TIME_BASE = (int64_t)1;
constexpr int HST_NSECOND = HST_TIME_BASE;
constexpr int HST_USECOND = ((int64_t)1000 * HST_NSECOND);
constexpr int HST_MSECOND = ((int64_t)1000 * HST_USECOND);
constexpr int HST_SECOND = ((int64_t)1000 * HST_MSECOND);

inline int64_t HstTime2Ns(int64_t hTime)
{
    return hTime / HST_NSECOND;
}

inline bool Ns2HstTime (int64_t ns, int64_t& hTime)
{
    hTime = ns * HST_NSECOND;
    return true;
}

inline int64_t HstTime2Us(int64_t hTime)
{
    return hTime / HST_USECOND;
}

inline int32_t HstTime2Us32(int64_t hTime)
{
    int64_t ms = hTime / HST_USECOND;
    if (INT32_MAX < ms ||  INT32_MIN > ms) { // overflow
        return INT32_MAX;
    }
    return static_cast<int32_t>(ms);
}

inline bool Us2HstTime (int64_t us, int64_t& hTime)
{
    if (INT64_MAX / HST_USECOND < us || INT64_MIN / HST_USECOND > us) { // overflow
        return false;
    }
    hTime = us * HST_USECOND;
    return true;
}

inline bool Ms2Us(int64_t ms, int64_t& us)
{
    return Us2HstTime(ms, us);
}

inline int64_t HstTime2Ms(int64_t hTime)
{
    return hTime / HST_MSECOND;
}

inline bool Ms2HstTime (int64_t ms, int64_t& hTime)
{
    if (INT64_MAX / HST_MSECOND < ms || INT64_MIN / HST_MSECOND > ms) { // overflow
        return false;
    }
    hTime = ms * HST_MSECOND;
    return true;
}

inline int64_t HstTime2Sec(int64_t hTime)
{
    return hTime / HST_SECOND;
}

inline bool Sec2HstTime (int64_t sec, int64_t& hTime)
{
    if (INT64_MAX / HST_SECOND < sec || INT64_MIN / HST_SECOND > sec) { // overflow
        return false;
    }
    hTime = sec * HST_SECOND;
    return true;
}

inline int64_t Us2Ms(int64_t us)
{
    return us / HST_USECOND;
}

inline int64_t GetCurrentMillisecond()
{
    std::chrono::system_clock::duration duration = std::chrono::system_clock::now().time_since_epoch();
    int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return time;
}

inline int64_t GetCurrentMicrosecond()
{
    std::chrono::system_clock::duration duration = std::chrono::system_clock::now().time_since_epoch();
    int64_t time = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return time;
}
} // Plugins
} // Media
} // OHOS
#endif // HISTREAMER_PLUGIN_COMMON_TIME_H

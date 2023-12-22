/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "osal/utils/util.h"
#include <climits>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#include "libavutil/error.h"

void usleep(__int64 usec)
{
    HANDLE timer = CreateWaitableTimer(nullptr, TRUE, nullptr);
    // set interval to 100 nanosecond, using relative time
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -(10 * usec); // 10
    SetWaitableTimer(timer, &dueTime, 0, nullptr, nullptr, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

char *realpath(const char *__restrict name, char *__restrict resolved)
{
    return _fullpath(resolved, name, PATH_MAX);
}

int access(const char *name, int type)
{
    return _access(name, type);
}
#else
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/error.h"
#ifdef __cplusplus
};
#endif
#endif

#define ERROR_MAX_STRING_SIZE 64

namespace OHOS {
namespace Media {
namespace OSAL {
void SleepFor(unsigned ms)
{
    constexpr int factor = 1000;
    usleep(ms * factor);
}

bool ConvertFullPath(const std::string& partialPath, std::string& fullPath)
{
    if (partialPath.empty() || partialPath.length() >= PATH_MAX) {
        return false;
    }
    char tmpPath[PATH_MAX] = {0};
    if (realpath(partialPath.c_str(), tmpPath) == nullptr) {
        return false;
    }
    if (access(tmpPath, F_OK) || access(tmpPath, R_OK)) {
        return false;
    }
    fullPath = tmpPath;
    return true;
}

std::string AVStrError(int errnum)
{
#ifdef MEDIA_OHOS
    char errbuf[ERROR_MAX_STRING_SIZE] = {0};
    return std::string(errbuf);
#else
    return "AVStrError " + std::to_string(errnum);
#endif
}
} // namespace OSAL
} // namespace Media
} // namespace OHOS
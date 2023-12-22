/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AV_SURFACE_MEMORY_H
#define AV_SURFACE_MEMORY_H

#include "buffer/avallocator.h"
#include "buffer/avbuffer.h"

namespace OHOS {
namespace Media {
class AVSurfaceMemory : public AVMemory {
public:
    explicit AVSurfaceMemory();
    ~AVSurfaceMemory() override;
    uint8_t *GetAddr() override;
    MemoryType GetMemoryType() override;
    int32_t GetFileDescriptor() override;
    sptr<SurfaceBuffer> GetSurfaceBuffer() override;

private:
    Status Init() override;
    Status Init(MessageParcel &parcel) override;
    Status InitSurfaceBuffer(MessageParcel &parcel) override;
    bool WriteToMessageParcel(MessageParcel &parcel) override;
    bool ReadFromMessageParcel(MessageParcel &parcel) override;
    Status MapMemoryAddr();
    void Close();
    sptr<SurfaceBuffer> surfaceBuffer_;
    bool isFirstFlag_;
};
} // namespace Media
} // namespace OHOS
#endif // AV_SURFACE_MEMORY_H
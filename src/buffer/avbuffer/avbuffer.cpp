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

#include "buffer/avbuffer.h"
#include <atomic>
#include <iomanip>
#include <sstream>
#include "avbuffer_utils.h"
#include "common/log.h"
#include "common/status.h"
#include "surface_buffer.h"
#include "surface_type.h"
#include "unistd.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_FOUNDATION, "AVBuffer" };
}

namespace OHOS {
namespace Media {
AVBuffer::AVBuffer() : pts_(0), dts_(0), duration_(0), flag_(0), meta_(nullptr), memory_(nullptr) {}

AVBuffer::~AVBuffer() {}

std::shared_ptr<AVBuffer> AVBuffer::CreateAVBuffer(const AVBufferConfig &config)
{
    std::shared_ptr<AVAllocator> allocator = nullptr;
    int32_t capacity = std::max(config.size, config.capacity);
    MemoryFlag memflag = MemoryFlag::MEMORY_READ_WRITE;
    switch (config.memoryType) {
        case MemoryType::VIRTUAL_MEMORY: {
            allocator = AVAllocatorFactory::CreateVirtualAllocator();
            break;
        }
        case MemoryType::SHARED_MEMORY: {
            memflag = config.memoryFlag;
            allocator = AVAllocatorFactory::CreateSharedAllocator(config.memoryFlag);
            break;
        }
        case MemoryType::SURFACE_MEMORY: {
            allocator = AVAllocatorFactory::CreateSurfaceAllocator(*(config.surfaceBufferConfig));
            break;
        }
        case MemoryType::HARDWARE_MEMORY: {
            memflag = config.memoryFlag;
            allocator = AVAllocatorFactory::CreateHardwareAllocator(config.dmaFd, capacity, config.memoryFlag);
            break;
        }
        default:
            return nullptr;
    }
    auto buffer = CreateAVBuffer(allocator, capacity, config.align);
    if (buffer != nullptr) {
        buffer->config_ = config;
        buffer->config_.capacity = capacity;
        buffer->config_.memoryFlag = memflag;
    }
    return buffer;
}

const AVBufferConfig &AVBuffer::GetConfig()
{
    FALSE_RETURN_V(memory_ != nullptr, config_);
    config_.size = memory_->GetSize();
    if (config_.memoryType == MemoryType::UNKNOWN_MEMORY) {
        config_.memoryType = memory_->GetMemoryType();
        config_.capacity = memory_->GetCapacity();
        config_.align = memory_->align_;
        config_.memoryFlag = memory_->GetMemoryFlag();
        switch (config_.memoryType) {
            case MemoryType::VIRTUAL_MEMORY: {
                break;
            }
            case MemoryType::SHARED_MEMORY: {
                break;
            }
            case MemoryType::HARDWARE_MEMORY: {
                config_.dmaFd = memory_->GetFileDescriptor();
                break;
            }
            case MemoryType::SURFACE_MEMORY: {
                auto surfaceBuffer = memory_->GetSurfaceBuffer();
                config_.surfaceBufferConfig->width = surfaceBuffer->GetWidth();
                config_.surfaceBufferConfig->height = surfaceBuffer->GetHeight();
                config_.surfaceBufferConfig->strideAlignment = surfaceBuffer->GetStride();
                config_.surfaceBufferConfig->format = surfaceBuffer->GetFormat();
                config_.surfaceBufferConfig->usage = surfaceBuffer->GetUsage();
                config_.surfaceBufferConfig->colorGamut = surfaceBuffer->GetSurfaceBufferColorGamut();
                config_.surfaceBufferConfig->transform = surfaceBuffer->GetSurfaceBufferTransform();
                break;
            }
            default:
                break;
        }
    }
    return config_;
}

std::shared_ptr<AVBuffer> AVBuffer::CreateAVBuffer(std::shared_ptr<AVAllocator> allocator, int32_t capacity,
                                                   int32_t align)
{
    FALSE_RETURN_V_MSG_E(allocator != nullptr, nullptr, "allocator is nullptr");
    FALSE_RETURN_V_MSG_E(capacity >= 0, nullptr, "capacity is invalid");
    FALSE_RETURN_V_MSG_E(align >= 0, nullptr, "align is invalid");

    auto buffer = std::shared_ptr<AVBuffer>(new AVBuffer());
    FALSE_RETURN_V_MSG_E(buffer != nullptr, nullptr, "Create AVBuffer failed, no memory");

    Status ret = buffer->Init(allocator, capacity, align);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, nullptr, "Init AVBuffer failed");

    buffer->meta_ = std::make_shared<Meta>();
    FALSE_RETURN_V_MSG_E(buffer->meta_ != nullptr, nullptr, "Create meta_ failed, no memory");
    return buffer;
}

std::shared_ptr<AVBuffer> AVBuffer::CreateAVBuffer(uint8_t *ptr, int32_t capacity, int32_t size)
{
    FALSE_RETURN_V_MSG_E(ptr != nullptr, nullptr, "ptr is nullptr");
    FALSE_RETURN_V_MSG_E(capacity >= 0, nullptr, "capacity is invalid");
    FALSE_RETURN_V_MSG_E((0 <= size) && (size <= capacity), nullptr, "size is invalid");

    auto buffer = std::shared_ptr<AVBuffer>(new AVBuffer());
    FALSE_RETURN_V_MSG_E(buffer != nullptr, nullptr, "Create AVBuffer failed, no memory");

    buffer->meta_ = std::make_shared<Meta>();
    FALSE_RETURN_V_MSG_E(buffer->meta_ != nullptr, nullptr, "Create meta_ failed, no memory");

    Status ret = buffer->Init(ptr, capacity, size);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, nullptr, "Init AVBuffer failed");
    return buffer;
}

std::shared_ptr<AVBuffer> AVBuffer::CreateAVBuffer(sptr<SurfaceBuffer> surfaceBuffer)
{
    FALSE_RETURN_V_MSG_E(surfaceBuffer != nullptr, nullptr, "surfaceBuffer is nullptr");
    FALSE_RETURN_V_MSG_E(surfaceBuffer->GetSptrRefCount() > 0, nullptr, "GetSptrRefCount is invalid, count:%{public}d",
                         surfaceBuffer->GetSptrRefCount());

    auto buffer = std::shared_ptr<AVBuffer>(new AVBuffer());
    FALSE_RETURN_V_MSG_E(buffer != nullptr, nullptr, "Create AVBuffer failed, no memory");

    buffer->meta_ = std::make_shared<Meta>();
    FALSE_RETURN_V_MSG_E(buffer->meta_ != nullptr, nullptr, "Create meta_ failed, no memory");

    Status ret = buffer->Init(surfaceBuffer);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, nullptr, "Init AVBuffer failed");
    return buffer;
}

std::shared_ptr<AVBuffer> AVBuffer::CreateAVBuffer()
{
    auto buffer = std::shared_ptr<AVBuffer>(new AVBuffer());
    FALSE_RETURN_V_MSG_E(buffer != nullptr, nullptr, "Create AVBuffer failed, no memory");

    buffer->meta_ = std::make_shared<Meta>();
    FALSE_RETURN_V_MSG_E(buffer->meta_ != nullptr, nullptr, "Create meta_ failed, no memory");
    return buffer;
}

Status AVBuffer::Init(std::shared_ptr<AVAllocator> allocator, int32_t capacity, int32_t align)
{
    memory_ = AVMemory::CreateAVMemory(allocator, capacity, align);
    FALSE_RETURN_V_MSG_E(memory_ != nullptr, Status::ERROR_UNKNOWN, "Create memory failed");
    return Status::OK;
}

Status AVBuffer::Init(uint8_t *ptr, int32_t capacity, int32_t size)
{
    memory_ = AVMemory::CreateAVMemory(ptr, capacity, size);
    FALSE_RETURN_V_MSG_E(memory_ != nullptr, Status::ERROR_UNKNOWN, "Create memory failed");
    return Status::OK;
}

Status AVBuffer::Init(sptr<SurfaceBuffer> surfaceBuffer)
{
    memory_ = AVMemory::CreateAVMemory(surfaceBuffer);
    FALSE_RETURN_V_MSG_E(memory_ != nullptr, Status::ERROR_UNKNOWN, "Create memory failed");
    return Status::OK;
}

uint64_t AVBuffer::GetUniqueId()
{
    if (memory_ == nullptr) {
        return 0;
    }
    return memory_->uid_;
}

bool AVBuffer::WriteToMessageParcel(MessageParcel &parcel)
{
#ifdef MEDIA_OHOS
    MessageParcel bufferParcel;
    bool ret = bufferParcel.WriteUint64(GetUniqueId()) && bufferParcel.WriteInt64(pts_) &&
               bufferParcel.WriteInt64(dts_) && bufferParcel.WriteInt64(duration_) && bufferParcel.WriteUint32(flag_) &&
               meta_->ToParcel(bufferParcel);

    if (memory_ != nullptr) {
        MemoryType type = memory_->GetMemoryType();
        FALSE_RETURN_V_MSG_E(type != MemoryType::VIRTUAL_MEMORY, false, "Virtual memory not support");

        ret = ret && bufferParcel.WriteUint8(static_cast<uint8_t>(type)) &&
              memory_->WriteCommonToMessageParcel(bufferParcel) && memory_->WriteToMessageParcel(bufferParcel);
    }
    if (ret) {
        parcel.Append(bufferParcel);
    }
    return ret;
#else
    return false;
#endif
}

bool AVBuffer::ReadFromMessageParcel(MessageParcel &parcel, bool isSurfaceBuffer)
{
#ifdef MEDIA_OHOS
    if (isSurfaceBuffer) {
        FALSE_RETURN_V(memory_ == nullptr, false);
        memory_ = AVMemory::CreateAVMemory(parcel, true);
        FALSE_RETURN_V_MSG_E(memory_ != nullptr, false, "Create memory failed");
        return true;
    }
    // 1. 不同buffer更新attr：  memroy != nullptr，uid != fromParcel， 返回错误
    // 2. 相同buffer更新attr：  memroy != nullptr，uid == fromParcel，不创建memory，更新attr + memory的attr
    // 3. 初始化buffer：        memroy == nullptr，fromParcel != 0，创建memory
    // 4. 只传buffer的attr：    memroy == nullptr，fromParcel == 0，更新attr
    uint64_t uid = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t duration = 0;
    uint32_t flag = 0;
    Meta meta;
    bool ret = parcel.ReadUint64(uid) && parcel.ReadInt64(pts) && parcel.ReadInt64(dts) &&
               parcel.ReadInt64(duration) && parcel.ReadUint32(flag) && meta.FromParcel(parcel);
    FALSE_RETURN_V_MSG_E(ret, false, "Unmarshalling buffer info failed");

    if (memory_ != nullptr) {
        FALSE_RETURN_V_MSG_E(GetUniqueId() == uid, false, "Can't read message parcel from other AVBuffer object!");
        (void)parcel.ReadUint8();
        ret = memory_->SkipCommonFromMessageParcel(parcel) && memory_->ReadFromMessageParcel(parcel);
        FALSE_RETURN_V_MSG_E(ret, false, "Update memory info failed");
    } else if (uid != 0) {
        memory_ = AVMemory::CreateAVMemory(parcel, false);
        FALSE_RETURN_V_MSG_E(memory_ != nullptr, false, "Create memory failed");
        memory_->uid_ = uid;
    }
    pts_ = pts;
    dts_ = dts;
    duration_ = duration;
    flag_ = flag;
    if (meta_ == nullptr) {
        meta_ = std::make_shared<Meta>();
    }
    *meta_ = std::move(meta);
#endif
    return true;
}

Status AVBuffer::Clone(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer)
{
    FALSE_RETURN_V_MSG(
        srcBuffer != nullptr && dstBuffer != nullptr, Status::ERROR_INVALID_PARAMETER, "Clone param invalid");

    // copy basic data
    dstBuffer->pts_ = srcBuffer->pts_;
    dstBuffer->dts_ = srcBuffer->dts_;
    dstBuffer->duration_ = srcBuffer->duration_;
    dstBuffer->flag_ = srcBuffer->flag_;

    CopyMeta(srcBuffer, dstBuffer);
    if (dstBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS)) {
        return Status::OK;
    }
    return CopyAVMemory(srcBuffer, dstBuffer);
}

void AVBuffer::CopyMeta(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer)
{
    if (srcBuffer->meta_ == nullptr) {
        dstBuffer->meta_ = nullptr;
        return;
    }
    dstBuffer->meta_ = std::make_shared<Meta>(*(srcBuffer->meta_));
}

Status AVBuffer::CopyAVMemory(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer)
{
    std::shared_ptr<AVMemory>& srcMemory = srcBuffer->memory_;
    std::shared_ptr<AVMemory>& dstMemory = dstBuffer->memory_;
    if (!srcMemory || !srcMemory->GetAddr() || !dstMemory || !dstMemory->GetAddr()) {
        return Status::ERROR_NULL_POINT_BUFFER;
    }
    if (srcMemory->GetSize() > dstMemory->GetCapacity()) {
        return Status::ERROR_INVALID_BUFFER_SIZE;
    }

    errno_t copyRet = memcpy_s(dstMemory->GetAddr(),
        dstMemory->GetCapacity(),
        srcMemory->GetAddr() + srcMemory->GetOffset(),
        srcMemory->GetSize());
    if (copyRet != EOK) {
        return Status::ERROR_UNKNOWN;
    }
    dstMemory->SetSize(srcMemory->GetSize());
    dstMemory->SetOffset(srcMemory->GetOffset());
    return Status::OK;
}
} // namespace Media
} // namespace OHOS
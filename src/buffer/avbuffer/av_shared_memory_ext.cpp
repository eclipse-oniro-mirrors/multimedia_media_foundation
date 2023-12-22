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

#include "av_shared_memory_ext.h"
#include <unistd.h>
#include "ashmem.h"
#include "av_shared_allocator.h"
#include "avbuffer_utils.h"
#include "buffer/avallocator.h"
#include "common/log.h"
#include "common/status.h"
#include "message_parcel.h"
#include "scope_guard.h"

#ifdef MEDIA_OHOS
#include "sys/mman.h"
#endif

namespace OHOS {
namespace Media {
std::shared_ptr<AVAllocator> AVAllocatorFactory::CreateSharedAllocator(MemoryFlag memFlag)
{
    auto allocator = std::shared_ptr<AVSharedAllocator>(new AVSharedAllocator());
    FALSE_RETURN_V_MSG_E(allocator != nullptr, nullptr, "Create AVSharedAllocator failed, no memory");
    allocator->memFlag_ = memFlag;
    return allocator;
}

AVSharedAllocator::AVSharedAllocator(){};

void *AVSharedAllocator::Alloc(int32_t capacity)
{
    int32_t fd = AshmemCreate(0, static_cast<size_t>(capacity)); // release by close(fd)
    FALSE_RETURN_V_MSG_E(fd > 0, nullptr, "fd is invalid, fd = " PUBLIC_LOG_D32, fd);

    return reinterpret_cast<void *>(fd);
}

bool AVSharedAllocator::Free(void *ptr)
{
    int32_t fd = reinterpret_cast<intptr_t>(ptr);
    if (fd > 0) {
        (void)::close(fd);
        return true;
    }
    return false;
}

MemoryType AVSharedAllocator::GetMemoryType()
{
    return MemoryType::SHARED_MEMORY;
}

MemoryFlag AVSharedAllocator::GetMemFlag()
{
    return memFlag_;
}

AVSharedMemoryExt::AVSharedMemoryExt() : fd_(-1), isFirstFlag_(true), memFlag_(MemoryFlag::MEMORY_READ_ONLY) {}

AVSharedMemoryExt::~AVSharedMemoryExt()
{
    MEDIA_LOG_DD("enter dtor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s", FAKE_POINTER(this),
                 name_.c_str());
    Close();
    if (allocator_ == nullptr) {
        if (fd_ > 0) {
            (void)::close(fd_);
            fd_ = -1;
        }
        return;
    }
    bool ret = allocator_->Free(reinterpret_cast<void *>(fd_));
    FALSE_RETURN_MSG(ret, "Free memory failed, instance: 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
}

Status AVSharedMemoryExt::Init()
{
    memFlag_ = std::static_pointer_cast<AVSharedAllocator>(allocator_)->GetMemFlag();

    int32_t allocSize = align_ ? (capacity_ + align_ - 1) : capacity_;
    fd_ = reinterpret_cast<intptr_t>(allocator_->Alloc(allocSize));
    FALSE_RETURN_V_MSG_E(fd_ > 0, Status::ERROR_NO_MEMORY, "Alloc AVSharedMemoryExt failed");

    uintptr_t addrBase = reinterpret_cast<uintptr_t>(base_);
    offset_ = static_cast<size_t>(AlignUp(addrBase, static_cast<uintptr_t>(offset_)) - addrBase);

    MEDIA_LOG_DD("enter init, instance: 0x%{public}06" PRIXPTR ", name = %{public}s", FAKE_POINTER(this),
                 name_.c_str());
    return Status::OK;
}

Status AVSharedMemoryExt::Init(MessageParcel &parcel)
{
#ifdef MEDIA_OHOS
    int32_t fd = parcel.ReadFileDescriptor();
    FALSE_RETURN_V_MSG_E(fd > 0, Status::ERROR_INVALID_DATA, "File descriptor is invalid");
    fd_ = dup(fd);

    memFlag_ = static_cast<MemoryFlag>(parcel.ReadUint32());
    MEDIA_LOG_DD("enter init, instance: 0x%{public}06" PRIXPTR ", name = %{public}s", FAKE_POINTER(this),
                 name_.c_str());
    (void)::close(fd);
    return Status::OK;
#else
    return Status::OK;
#endif
}

bool AVSharedMemoryExt::WriteToMessageParcel(MessageParcel &parcel)
{
#ifdef MEDIA_OHOS
    MessageParcel bufferParcel;
    bool ret = bufferParcel.WriteFileDescriptor(fd_) && bufferParcel.WriteUint32(static_cast<uint32_t>(memFlag_));
    if (ret) {
        parcel.Append(bufferParcel);
    }
    return ret;
#else
    return false;
#endif
}

bool AVSharedMemoryExt::ReadFromMessageParcel(MessageParcel &parcel)
{
#ifdef MEDIA_OHOS
    int32_t fd = parcel.ReadFileDescriptor();
    (void)parcel.ReadUint32();
    if (fd > 0) {
        (void)::close(fd);
    }
#endif
    return true;
}

uint8_t *AVSharedMemoryExt::GetAddr()
{
    if (isFirstFlag_) {
        Status ret = MapMemoryAddr();
        FALSE_RETURN_V_MSG_E(ret == Status::OK, nullptr, "MapMemory failed");
        isFirstFlag_ = false;
    }
    return base_;
}

MemoryType AVSharedMemoryExt::GetMemoryType()
{
    return MemoryType::SHARED_MEMORY;
}

MemoryFlag AVSharedMemoryExt::GetMemoryFlag()
{
    return memFlag_;
}

int32_t AVSharedMemoryExt::GetFileDescriptor()
{
    return fd_;
}

void AVSharedMemoryExt::Close() noexcept
{
#ifdef MEDIA_OHOS
    if (base_ != nullptr) {
        (void)::munmap(base_, static_cast<size_t>(capacity_));
        base_ = nullptr;
        size_ = 0;
    }
#endif
}

Status AVSharedMemoryExt::MapMemoryAddr()
{
#ifdef MEDIA_OHOS
    ON_SCOPE_EXIT(0)
    {
        MEDIA_LOG_E("create avsharedmemory failed, name = %{public}s, size = " PUBLIC_LOG_D32 ", "
                    "flags = 0x%{public}x, fd = " PUBLIC_LOG_D32,
                    name_.c_str(), capacity_, memFlag_, fd_);
        Close();
        return Status::ERROR_NO_MEMORY;
    };
    FALSE_RETURN_V_MSG_E(capacity_ > 0, Status::ERROR_INVALID_DATA, "size is invalid, size = " PUBLIC_LOG_D32,
                         capacity_);
    unsigned int prot = PROT_READ | PROT_WRITE;
    if (memFlag_ == MemoryFlag::MEMORY_READ_ONLY) {
        prot &= ~PROT_WRITE;
    } else if (memFlag_ == MemoryFlag::MEMORY_WRITE_ONLY) {
        prot &= ~PROT_READ;
    }
    int result = AshmemSetProt(fd_, static_cast<int>(prot));
    FALSE_RETURN_V_MSG_E(result >= 0, Status::ERROR_INVALID_OPERATION, "AshmemSetProt failed, result = " PUBLIC_LOG_D32,
                         result);

    void *addr = ::mmap(nullptr, static_cast<size_t>(capacity_), static_cast<int>(prot), MAP_SHARED, fd_, 0);
    FALSE_RETURN_V_MSG_E(addr != MAP_FAILED, Status::ERROR_INVALID_OPERATION, "mmap failed, please check params");

    base_ = static_cast<uint8_t *>(addr);
    CANCEL_SCOPE_EXIT_GUARD(0);
#endif
    return Status::OK;
}
} // namespace Media
} // namespace OHOS

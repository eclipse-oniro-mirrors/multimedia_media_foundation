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

#include "avbuffer_queue_consumer_impl.h"
#include "avbuffer_queue_impl.h"
#include "avbuffer_queue_producer_impl.h"
#include "common/log.h"
#include "meta/media_types.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_FOUNDATION, "AVBufferQueue" };
static constexpr uint8_t LOG_LIMIT_LOW_FREQ = 64;
constexpr int64_t MILLISECONDS_TO_MICROSECONDS = 1000;
}

namespace OHOS {
namespace Media {

std::shared_ptr<AVBufferQueue> AVBufferQueue::Create(
    uint32_t size, MemoryType type, const std::string& name, bool disableAlloc)
{
    MEDIA_LOG_D("AVBufferQueue::Create size = %u, type = %u, name = %s",
                size, static_cast<uint32_t>(type), name.c_str());
    return std::make_shared<AVBufferQueueImpl>(size, type, name, disableAlloc);
}

std::shared_ptr<AVBufferQueueProducer> AVBufferQueueImpl::GetLocalProducer()
{
    std::lock_guard<std::mutex> lockGuard(producerCreatorMutex_);
    std::shared_ptr<AVBufferQueueProducerImpl> producer = nullptr;
    if (localProducer_.expired()) {
        auto shared_this = shared_from_this();
        FALSE_RETURN_V(shared_this != nullptr, nullptr);
        producer = std::make_shared<AVBufferQueueProducerImpl>(shared_this);
        localProducer_ = producer;
    }

    return localProducer_.lock();
}

std::shared_ptr<AVBufferQueueConsumer> AVBufferQueueImpl::GetLocalConsumer()
{
    std::lock_guard<std::mutex> lockGuard(consumerCreatorMutex_);
    std::shared_ptr<AVBufferQueueConsumerImpl> consumer = nullptr;
    if (localConsumer_.expired()) {
        auto shared_this = shared_from_this();
        FALSE_RETURN_V(shared_this != nullptr, nullptr);
        consumer = std::make_shared<AVBufferQueueConsumerImpl>(shared_this);
        localConsumer_ = consumer;
    }
    return localConsumer_.lock();
}

sptr<AVBufferQueueProducer> AVBufferQueueImpl::GetProducer()
{
    std::lock_guard<std::mutex> lockGuard(producerCreatorMutex_);
    sptr<AVBufferQueueProducerImpl> producer = nullptr;
    if (producer_ == nullptr || producer_->GetSptrRefCount() <= 0) {
        auto shared_this = shared_from_this();
        FALSE_RETURN_V(shared_this != nullptr, nullptr);
        producer = new AVBufferQueueProducerImpl(shared_this);
        producer_ = producer;
    }

    return producer_.promote();
}

sptr<AVBufferQueueConsumer> AVBufferQueueImpl::GetConsumer()
{
    std::lock_guard<std::mutex> lockGuard(consumerCreatorMutex_);
    sptr<AVBufferQueueConsumerImpl> consumer = nullptr;
    if (consumer_ == nullptr || consumer_->GetSptrRefCount() <= 0) {
        auto shared_this = shared_from_this();
        FALSE_RETURN_V(shared_this != nullptr, nullptr);
        consumer = new AVBufferQueueConsumerImpl(shared_this);
        consumer_ = consumer;
    }

    return consumer_.promote();
}

AVBufferQueueImpl::AVBufferQueueImpl(const std::string &name)
    : AVBufferQueue(), name_(name), size_(0), memoryType_(MemoryType::UNKNOWN_MEMORY), disableAlloc_(false) {}

AVBufferQueueImpl::AVBufferQueueImpl(uint32_t size, MemoryType type, const std::string &name, bool disableAlloc)
    : AVBufferQueue(), name_(name), size_(size), memoryType_(type), disableAlloc_(disableAlloc)
{
    if (size_ > AVBUFFER_QUEUE_MAX_QUEUE_SIZE) {
        size_ = AVBUFFER_QUEUE_MAX_QUEUE_SIZE;
    }
}

uint32_t AVBufferQueueImpl::GetQueueSize()
{
    return size_;
}

Status AVBufferQueueImpl::SetQueueSize(uint32_t size)
{
    FALSE_RETURN_V(size >= 0 && size <= AVBUFFER_QUEUE_MAX_QUEUE_SIZE && size != size_,
                   Status::ERROR_INVALID_BUFFER_SIZE);

    return SetLargerQueueSize(size);
}

Status AVBufferQueueImpl::SetLargerQueueSize(uint32_t size)
{
    FALSE_RETURN_V(size >= 0 && size <= AVBUFFER_QUEUE_MAX_QUEUE_SIZE_FOR_LARGER  && size != size_,
                   Status::ERROR_INVALID_BUFFER_SIZE);

    if (size > size_) {
        size_ = size;
        if (!disableAlloc_) {
            requestCondition.notify_all();
        }
    } else {
        std::lock_guard<std::mutex> lockGuard(queueMutex_);
        DeleteBuffers(size_ - size);
        size_ = size;
    }

    return Status::OK;
}

Status AVBufferQueueImpl::ClearBufferIf(std::function<bool(const std::shared_ptr<AVBuffer>&)> pred)
{
    std::lock_guard<std::mutex> lockGuard(queueMutex_);
    for (auto dirtyIt = dirtyBufferList_.begin(); dirtyIt != dirtyBufferList_.end();) {
        uint64_t uniqueId = *dirtyIt;
        auto cacheIt = cachedBufferMap_.find(uniqueId);
        if (cacheIt == cachedBufferMap_.end()) {
            MEDIA_LOG_E("unexpected buffer uniqueId=" PUBLIC_LOG_U64, uniqueId);
            ++dirtyIt;
            continue;
        }
        if (cacheIt->second.state != AVBUFFER_STATE_PUSHED && cacheIt->second.state != AVBUFFER_STATE_RETURNED) {
            MEDIA_LOG_I("ignore unexpected buffer status uniqueId=" PUBLIC_LOG_U64 ",state= " PUBLIC_LOG_D32,
                uniqueId,
                static_cast<int32_t>(cacheIt->second.state));
            ++dirtyIt;
            continue;
        }
 
        if (pred(cacheIt->second.buffer)) {
            MEDIA_LOG_D("ClearBufferIf pred ok uniqueId=" PUBLIC_LOG_U64 ",pts=" PUBLIC_LOG_D64,
                uniqueId,
                cacheIt->second.buffer->pts_);
            cacheIt->second.state = AVBUFFER_STATE_RELEASED;
            InsertFreeBufferInOrder(uniqueId);
            dirtyIt = dirtyBufferList_.erase(dirtyIt);
        } else {
            ++dirtyIt;
        }
    }
    requestCondition.notify_all();
    return Status::OK;
}

bool AVBufferQueueImpl::IsBufferInQueue(const std::shared_ptr<AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, false);
    auto uniqueId = buffer->GetUniqueId();
    return cachedBufferMap_.find(uniqueId) != cachedBufferMap_.end();
}

uint32_t AVBufferQueueImpl::GetCachedBufferCount() const
{
    // 确保cachedBufferMap_.size()不会超过MAX_UINT32
    return static_cast<uint32_t>(cachedBufferMap_.size());
}

Status AVBufferQueueImpl::PopFromFreeBufferList(std::shared_ptr<AVBuffer>& buffer, const AVBufferConfig& config)
{
    for (auto it = freeBufferList_.begin(); it != freeBufferList_.end(); it++) {
        if (config <= cachedBufferMap_[*it].config) {
            buffer = cachedBufferMap_[*it].buffer;
            freeBufferList_.erase(it);
            return Status::OK;
        }
    }

    if (freeBufferList_.empty()) {
        buffer = nullptr;
        // 没有可以重用的freeBuffer
        return Status::ERROR_NO_FREE_BUFFER;
    }

    buffer = cachedBufferMap_[freeBufferList_.front()].buffer;
    freeBufferList_.pop_front();

    return Status::OK;
}

Status AVBufferQueueImpl::PopFromDirtyBufferList(std::shared_ptr<AVBuffer>& buffer)
{
    FALSE_RETURN_V_NOLOG(!dirtyBufferList_.empty(), Status::ERROR_NO_DIRTY_BUFFER);

    buffer = cachedBufferMap_[dirtyBufferList_.front()].buffer;
    dirtyBufferList_.pop_front();
    return Status::OK;
}

Status AVBufferQueueImpl::AllocBuffer(std::shared_ptr<AVBuffer>& buffer, const AVBufferConfig& config)
{
    auto bufferImpl = AVBuffer::CreateAVBuffer(config);
    FALSE_RETURN_V(bufferImpl != nullptr, Status::ERROR_CREATE_BUFFER);

    auto uniqueId = bufferImpl->GetUniqueId();
    AVBufferElement ele = {
        .config = bufferImpl->GetConfig(),
        .state = AVBUFFER_STATE_RELEASED,
        .isDeleting = false,
        .buffer = bufferImpl,
    };
    cachedBufferMap_[uniqueId] = ele;
    buffer = bufferImpl;
    TotalMemoryCalculation(true, ele.config.capacity);

    return Status::OK;
}

Status AVBufferQueueImpl::RequestReuseBuffer(std::shared_ptr<AVBuffer>& buffer, const AVBufferConfig& config)
{
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);

    auto uniqueId = buffer->GetUniqueId();
    FALSE_RETURN_V(cachedBufferMap_.find(uniqueId) != cachedBufferMap_.end(), Status::ERROR_CREATE_BUFFER);

    if (config <= cachedBufferMap_[uniqueId].config) {
        // 不需要重新分配，直接更新buffer大小
        cachedBufferMap_[uniqueId].config.size = config.size;
    } else {
        // 重新分配
        DeleteCachedBufferById(uniqueId);
        NOK_RETURN(AllocBuffer(buffer, config));
    }

    // 注意这里的uniqueId可能因为重新分配buffer而更新，所以需要再次获取
    cachedBufferMap_[buffer->GetUniqueId()].state = AVBUFFER_STATE_REQUESTED;
    return Status::OK;
}

void AVBufferQueueImpl::DeleteBuffers(uint32_t count)
{
    FALSE_RETURN(count > 0);

    while (!freeBufferList_.empty()) {
        DeleteCachedBufferById(freeBufferList_.front());
        freeBufferList_.pop_front();
        count--;
        if (count <= 0) {
            return;
        }
    }

    while (!dirtyBufferList_.empty()) {
        DeleteCachedBufferById(dirtyBufferList_.front());
        dirtyBufferList_.pop_front();
        count--;
        if (count <= 0) {
            return;
        }
    }

    for (auto&& ele : cachedBufferMap_) {
        ele.second.isDeleting = true;
        // we don't have to do anything
        count--;
        if (count <= 0) {
            break;
        }
    }
}

void AVBufferQueueImpl::DeleteCachedBufferById(uint64_t uniqueId)
{
    auto it = cachedBufferMap_.find(uniqueId);
    if (it != cachedBufferMap_.end()) {
        MEDIA_LOG_D("DeleteCachedBufferById uniqueId:%llu, state:%d", uniqueId, it->second.state);
        TotalMemoryCalculation(false, it->second.config.capacity);
        cachedBufferMap_.erase(it);
    }
}

Status AVBufferQueueImpl::CheckConfig(const AVBufferConfig& config)
{
    if (config.memoryType == MemoryType::UNKNOWN_MEMORY) {
        MEDIA_LOG_D("config.memoryType != MemoryType::UNKNOWN_MEMORY");
        return Status::ERROR_UNEXPECTED_MEMORY_TYPE;
    }
    // memoryType_初始化之后将无法改变。
    if (memoryType_ != MemoryType::UNKNOWN_MEMORY && config.memoryType != memoryType_) {
        MEDIA_LOG_D("memoryType_ != MemoryType::UNKNOWN_MEMORY && config.memoryType != memoryType_");
        return Status::ERROR_UNEXPECTED_MEMORY_TYPE;
    }
    memoryType_ = config.memoryType;
    return Status::OK;
}

bool AVBufferQueueImpl::wait_for(std::unique_lock<std::mutex>& lock, int64_t timeoutUs)
{
    MEDIA_LOG_D("wait for free buffer, timeout = %{public}" PRId64, timeoutUs);
    if (timeoutUs > 0) {
        return requestCondition.wait_for(
            lock, std::chrono::microseconds(timeoutUs), [this]() {
                return !freeBufferList_.empty() || (GetCachedBufferCount() < GetQueueSize());
            });
    } else if (timeoutUs < 0) {
        requestCondition.wait(lock);
    }
    return true;
}

Status AVBufferQueueImpl::RequestBuffer(
    std::shared_ptr<AVBuffer>& buffer, const AVBufferConfig& config, int32_t timeoutMs)
{
    return RequestBufferWaitUs(buffer, config, static_cast<int64_t>(timeoutMs) * MILLISECONDS_TO_MICROSECONDS);
}

Status AVBufferQueueImpl::RequestBufferWaitUs(
    std::shared_ptr<AVBuffer>& buffer, const AVBufferConfig& config, int64_t timeoutUs)
{
    auto configCopy = config;
    if (config.memoryType == MemoryType::UNKNOWN_MEMORY) {
        MEDIA_LOG_D("AVBufferQueueImpl::RequestBuffer config.memoryType unknown, "
                    "memoryType_ = %u", static_cast<uint32_t>(memoryType_));
        configCopy.memoryType = memoryType_;
    }

    // check param
    std::unique_lock<std::mutex> lock(queueMutex_);
    auto res = CheckConfig(configCopy);
    FALSE_RETURN_V_MSG(res == Status::OK,
        res, "CheckConfig not OK, code %{public}d", static_cast<int32_t>(res));
    // dequeue from free list
    auto ret = PopFromFreeBufferList(buffer, configCopy);
    if (ret == Status::OK) {
        return RequestReuseBuffer(buffer, configCopy);
    }

    // check queue size
    if (GetCachedBufferCount() >= GetQueueSize()) {
        if (!wait_for(lock, timeoutUs)) {
            MEDIA_LOG_D("FALSE_RETURN_V wait_for(lock, timeoutUs)");
            return Status::ERROR_WAIT_TIMEOUT;
        }
        // 被条件唤醒后，再次尝试从freeBufferList中取buffer
        ret = PopFromFreeBufferList(buffer, configCopy);
        if (ret == Status::OK) {
            return RequestReuseBuffer(buffer, configCopy);
        }
        if (GetCachedBufferCount() >= GetQueueSize()) {
            return Status::ERROR_NO_FREE_BUFFER;
        }
    }

    NOK_RETURN(AllocBuffer(buffer, configCopy));
    cachedBufferMap_[buffer->GetUniqueId()].state = AVBUFFER_STATE_REQUESTED;

    return Status::OK;
}

void AVBufferQueueImpl::InsertFreeBufferInOrder(uint64_t uniqueId)
{
    for (auto it = freeBufferList_.begin(); it != freeBufferList_.end(); it++) {
        if ((*it != uniqueId) &&
                (cachedBufferMap_[*it].config.capacity >= cachedBufferMap_[uniqueId].config.capacity)) {
            freeBufferList_.insert(it, uniqueId);
            return;
        }
    }
    freeBufferList_.emplace_back(uniqueId);
}

Status AVBufferQueueImpl::CancelBuffer(uint64_t uniqueId)
{
    FALSE_RETURN_V(cachedBufferMap_.find(uniqueId) != cachedBufferMap_.end(), Status::ERROR_INVALID_BUFFER_ID);

    FALSE_RETURN_V(cachedBufferMap_[uniqueId].state == AVBUFFER_STATE_REQUESTED ||
                   cachedBufferMap_[uniqueId].state == AVBUFFER_STATE_PUSHED,
                   Status::ERROR_INVALID_BUFFER_STATE);

    InsertFreeBufferInOrder(uniqueId);

    cachedBufferMap_[uniqueId].state = AVBUFFER_STATE_RELEASED;

    requestCondition.notify_all();

    MEDIA_LOG_D("cancel buffer id = %llu", uniqueId);

    return Status::OK;
}

Status AVBufferQueueImpl::PushBuffer(uint64_t uniqueId, bool available)
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    {
        std::lock_guard<std::mutex> lockGuard(queueMutex_);
        FALSE_RETURN_V(cachedBufferMap_.find(uniqueId) != cachedBufferMap_.end(),
                       Status::ERROR_INVALID_BUFFER_ID);

        auto& ele = cachedBufferMap_[uniqueId];
        if (ele.isDeleting) {
            DeleteCachedBufferById(uniqueId);
            MEDIA_LOG_D("delete push buffer uniqueId(%llu)", uniqueId);
            return Status::OK;
        }

        if (available) {
            FALSE_RETURN_V(ele.buffer->GetConfig().size >= 0, Status::ERROR_INVALID_BUFFER_SIZE);
        }

        FALSE_RETURN_V(ele.state == AVBUFFER_STATE_REQUESTED || ele.state == AVBUFFER_STATE_ATTACHED,
                       Status::ERROR_INVALID_BUFFER_STATE);

        ele.state = AVBUFFER_STATE_PUSHED;
        buffer = cachedBufferMap_[uniqueId].buffer;
    }

    if (available) {
        std::lock_guard<std::mutex> lockGuard(brokerListenerMutex_);
        if (!brokerListeners_.empty() && brokerListeners_.back() != nullptr) {
            brokerListeners_.back()->OnBufferFilled(buffer);
            return Status::OK;
        }
    }

    return ReturnBuffer(uniqueId, available);
}

Status AVBufferQueueImpl::PushBuffer(const std::shared_ptr<AVBuffer>& buffer, bool available)
{
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);

    return PushBuffer(buffer->GetUniqueId(), available);
}

Status __attribute__((no_sanitize("cfi"))) AVBufferQueueImpl::ReturnBuffer(uint64_t uniqueId, bool available)
{
    {
        std::lock_guard<std::mutex> lockGuard(queueMutex_);
        FALSE_RETURN_V(cachedBufferMap_.find(uniqueId) != cachedBufferMap_.end(),
                       Status::ERROR_INVALID_BUFFER_ID);

        if (cachedBufferMap_[uniqueId].isDeleting) {
            DeleteCachedBufferById(uniqueId);
            MEDIA_LOG_D("delete return buffer uniqueId(%llu)", uniqueId);
            return Status::OK;
        }

        FALSE_RETURN_V(cachedBufferMap_[uniqueId].state == AVBUFFER_STATE_PUSHED,
                       Status::ERROR_INVALID_BUFFER_STATE);

        if (!available) {
            NOK_RETURN(CancelBuffer(uniqueId));
        } else {
            auto& config = cachedBufferMap_[uniqueId].buffer->GetConfig();
            bool isEosBuffer = cachedBufferMap_[uniqueId].buffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS);
            if (!isEosBuffer) {
                FALSE_RETURN_V(config.size > 0, Status::ERROR_INVALID_BUFFER_SIZE);
            }
            cachedBufferMap_[uniqueId].config = config;
            cachedBufferMap_[uniqueId].state = AVBUFFER_STATE_RETURNED;
            dirtyBufferList_.push_back(uniqueId);
        }
    }

    if (!available) {
        std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
        if (producerListener_ != nullptr) {
            producerListener_->OnBufferAvailable();
        }
        return Status::OK;
    }

    std::lock_guard<std::mutex> lockGuard(consumerListenerMutex_);
    FALSE_RETURN_V(consumerListener_ != nullptr, Status::ERROR_NO_CONSUMER_LISTENER);
    consumerListener_->OnBufferAvailable();

    return Status::OK;
}

Status AVBufferQueueImpl::ReturnBuffer(const std::shared_ptr<AVBuffer>& buffer, bool available)
{
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);

    return ReturnBuffer(buffer->GetUniqueId(), available);
}

uint32_t AVBufferQueueImpl::GetMemoryUsage()
{
    return memoryUsage_.load();
}

Status AVBufferQueueImpl::SetQueueSizeAndAttachBuffer(uint32_t size,
    std::shared_ptr<AVBuffer>& buffer, bool isFilled)
{
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);
    auto config = buffer->GetConfig();
    auto uniqueId = buffer->GetUniqueId();
    {
        std::lock_guard<std::mutex> lockGuard(queueMutex_);
        if (size >= 0 && size <= AVBUFFER_QUEUE_MAX_QUEUE_SIZE && size != size_) {
            SetQueueSizeBeforeAttachBufferLocked(size);
        }
        FALSE_RETURN_V(cachedBufferMap_.find(uniqueId) == cachedBufferMap_.end(),
                       Status::ERROR_INVALID_BUFFER_ID);
        NOK_RETURN(CheckConfig(config));
        Status result = AttachAvailableBufferLocked(buffer);
        FALSE_RETURN_V(result == Status::OK, result);
    }
    if (isFilled) {
        return PushBufferOnFilled(uniqueId, isFilled);
    }
    return ReleaseBuffer(uniqueId);
}

void AVBufferQueueImpl::TotalMemoryCalculation(bool isAdd, int32_t capacity)
{
    FALSE_RETURN(capacity > 0);
    uint32_t capacityUint = static_cast<uint32_t>(capacity);
    if (isAdd) {
        memoryUsage_.fetch_add(capacityUint);
    } else {
        FALSE_RETURN(capacityUint <= memoryUsage_.load());
        memoryUsage_.fetch_sub(capacityUint);
    }
}

Status AVBufferQueueImpl::AttachAvailableBufferLocked(std::shared_ptr<AVBuffer>& buffer)
{
    auto config = buffer->GetConfig();
    auto uniqueId = buffer->GetUniqueId();
    AVBufferElement ele = {
        .config = config,
        .state = AVBUFFER_STATE_ATTACHED,
        .isDeleting = false,
        .buffer = buffer
    };

    auto cachedCount = GetCachedBufferCount();
    auto queueSize = GetQueueSize();
    if (cachedCount >= queueSize) {
        auto validCount = static_cast<uint32_t>(dirtyBufferList_.size() + freeBufferList_.size());
        auto toBeDeleteCount = cachedCount - queueSize;
        // 这里表示有可以删除的buffer，或者
        if (validCount > toBeDeleteCount) {
            // 在什么场景下需要在此处删除buffer？
            DeleteBuffers(toBeDeleteCount + 1); // 多删除一个，用于attach当前buffer
            cachedBufferMap_[uniqueId] = ele;
            TotalMemoryCalculation(true, ele.config.capacity);
            MEDIA_LOG_D("uniqueId(%llu) attached with delete", uniqueId);
        } else {
            MEDIA_LOG_E("attach failed, out of range");
            return Status::ERROR_OUT_OF_RANGE;
        }
    } else {
        cachedBufferMap_[uniqueId] = ele;
        TotalMemoryCalculation(true, ele.config.capacity);
        MEDIA_LOG_D("uniqueId(%llu) attached without delete", uniqueId);
    }
    return Status::OK;
}

Status AVBufferQueueImpl::PushBufferOnFilled(uint64_t uniqueId, bool isFilled)
{
    auto ret = PushBuffer(uniqueId, isFilled);
    if (ret != Status::OK) {
        // PushBuffer失败，强制Detach
        DetachBuffer(uniqueId, true);
    }
    return ret;
}

void AVBufferQueueImpl::SetQueueSizeBeforeAttachBufferLocked(uint32_t size)
{
    if (size > size_) {
        size_ = size;
        if (!disableAlloc_) {
            requestCondition.notify_all();
        }
    } else {
        DeleteBuffers(size_ - size);
        size_ = size;
    }
}

Status AVBufferQueueImpl::AttachBuffer(std::shared_ptr<AVBuffer>& buffer, bool isFilled)
{
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);

    auto config = buffer->GetConfig();
    auto uniqueId = buffer->GetUniqueId();
    {
        std::lock_guard<std::mutex> lockGuard(queueMutex_);
        FALSE_RETURN_V(cachedBufferMap_.find(uniqueId) == cachedBufferMap_.end(),
                       Status::ERROR_INVALID_BUFFER_ID);

        NOK_RETURN(CheckConfig(config));

        Status result = AttachAvailableBufferLocked(buffer);
        FALSE_RETURN_V(result == Status::OK, result);
    }

    if (isFilled) {
        return PushBufferOnFilled(uniqueId, isFilled);
    }

    return ReleaseBuffer(uniqueId);
}

Status AVBufferQueueImpl::DetachBuffer(uint64_t uniqueId, bool force)
{
    FALSE_RETURN_V_NOLOG(cachedBufferMap_.find(uniqueId) != cachedBufferMap_.end(),
        Status::ERROR_INVALID_BUFFER_ID);

    const auto& ele = cachedBufferMap_[uniqueId];

    if (!force) {
        // 只有生产者或消费者在获取到buffer后才能detach
        if (ele.state == AVBUFFER_STATE_REQUESTED) {
            MEDIA_LOG_D("detach buffer(%llu) on state requested", uniqueId);
        } else if (ele.state == AVBUFFER_STATE_ACQUIRED) {
            MEDIA_LOG_D("detach buffer(%llu) on state acquired", uniqueId);
        } else {
            MEDIA_LOGW_LIMIT(LOG_LIMIT_LOW_FREQ, "detach buffer(" PUBLIC_LOG_U64 ") on state "
                PUBLIC_LOG_D32 "forbidden", uniqueId, ele.state);
            return Status::ERROR_INVALID_BUFFER_STATE;
        }
    }
    TotalMemoryCalculation(false, ele.config.capacity);
    cachedBufferMap_.erase(uniqueId);

    return Status::OK;
}

Status AVBufferQueueImpl::DetachBuffer(uint64_t uniqueId)
{
    std::lock_guard<std::mutex> lockGuard(queueMutex_);
    return DetachBuffer(uniqueId, false);
}

Status AVBufferQueueImpl::DetachBuffer(const std::shared_ptr<AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);

    return DetachBuffer(buffer->GetUniqueId());
}

Status AVBufferQueueImpl::AcquireBuffer(std::shared_ptr<AVBuffer>& buffer)
{
    std::lock_guard<std::mutex> lockGuard(queueMutex_);
    auto ret = PopFromDirtyBufferList(buffer);
    FALSE_RETURN_V_MSG_D(ret == Status::OK, ret, "acquire buffer failed");

    cachedBufferMap_[buffer->GetUniqueId()].state = AVBUFFER_STATE_ACQUIRED;

    return Status::OK;
}

Status AVBufferQueueImpl::ReleaseBuffer(uint64_t uniqueId)
{
    {
        std::lock_guard<std::mutex> lockGuard(queueMutex_);
        FALSE_RETURN_V(cachedBufferMap_.find(uniqueId) != cachedBufferMap_.end(), Status::ERROR_INVALID_BUFFER_ID);

        FALSE_RETURN_V(cachedBufferMap_[uniqueId].state == AVBUFFER_STATE_ACQUIRED ||
            cachedBufferMap_[uniqueId].state == AVBUFFER_STATE_ATTACHED, Status::ERROR_INVALID_BUFFER_STATE);

        cachedBufferMap_[uniqueId].state = AVBUFFER_STATE_RELEASED;
        if (cachedBufferMap_[uniqueId].isDeleting) {
            DeleteCachedBufferById(uniqueId);
            return Status::OK;
        }

        InsertFreeBufferInOrder(uniqueId);

        requestCondition.notify_all();
    }

    // 注意：此时通知生产者有buffer可用，但实际有可能已经被request wait的生产者获取
    std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
    if (producerListener_ != nullptr) {
        producerListener_->OnBufferAvailable();
    }

    return Status::OK;
}

Status AVBufferQueueImpl::ReleaseBuffer(const std::shared_ptr<AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);

    return ReleaseBuffer(buffer->GetUniqueId());
}

Status AVBufferQueueImpl::Clear()
{
    MEDIA_LOG_E("AVBufferQueueImpl Clear");
    std::lock_guard<std::mutex> lockGuard(queueMutex_);
    dirtyBufferList_.clear();
    for (auto it = cachedBufferMap_.begin(); it != cachedBufferMap_.end(); it++) {
        if (it->second.state == AVBUFFER_STATE_PUSHED || it->second.state == AVBUFFER_STATE_RETURNED) {
            it->second.state = AVBUFFER_STATE_RELEASED;
            InsertFreeBufferInOrder(it->first);
        }
    }
    requestCondition.notify_all();
    return Status::OK;
}

Status AVBufferQueueImpl::SetBrokerListener(sptr<IBrokerListener>& listener)
{
    std::lock_guard<std::mutex> lockGuard(brokerListenerMutex_);
    brokerListeners_.push_back(listener);
    return Status::OK;
}

Status AVBufferQueueImpl::RemoveBrokerListener(sptr<IBrokerListener>& listener)
{
    std::lock_guard<std::mutex> lockGuard(brokerListenerMutex_);
    if (!brokerListeners_.empty() && listener == brokerListeners_.back()) {
        brokerListeners_.pop_back();
        MEDIA_LOG_I("RemoveBrokerListener success, size: %{public}d", brokerListeners_.size());
    } else {
        MEDIA_LOG_E("removed item is not the back one.");
    }
    return Status::OK;
}

Status AVBufferQueueImpl::SetProducerListener(sptr<IProducerListener>& listener)
{
    std::lock_guard<std::mutex> lockGuard(producerListenerMutex_);
    producerListener_ = listener;

    return Status::OK;
}

Status AVBufferQueueImpl::SetConsumerListener(sptr<IConsumerListener>& listener)
{
    std::lock_guard<std::mutex> lockGuard(consumerListenerMutex_);
    consumerListener_ = listener;

    return Status::OK;
}

uint32_t AVBufferQueueImpl::GetFilledBufferSize()
{
    std::lock_guard<std::mutex> lockGuard(queueMutex_);
    return dirtyBufferList_.size();
}

} // namespace Media
} // namespace OHOS

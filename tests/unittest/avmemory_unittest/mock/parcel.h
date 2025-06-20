/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

 /**
  * @file parcel.h
  *
  * @brief Provide classes for data container implemented in c_utils.
  *
  * Including class Parcel, Parcelable and related memory Allocator.
  */

#ifndef OHOS_UTILS_PARCEL_H
#define OHOS_UTILS_PARCEL_H

#include <string>
#include <vector>
#include "gmock/gmock.h"
#include "nocopyable.h"
#include "refbase.h"
#include "flat_obj.h"

namespace OHOS {
const uint32_t NUM_TEST = 4;
class Parcel;

/**
 * @brief Interface for class whose Instance can be written into a Parcel.
 *
 * @note If this object is remote, its position intends to
 * use in kernel data transaction.
 */
class Parcelable : public virtual RefBase {
public:
    virtual ~Parcelable() = default;

    Parcelable();
    /**
     * @brief Construct a new Parcelable object.
     *
     * @param asRemote Specifies whether this object is remote.
     */
    explicit Parcelable(bool asRemote);

    /**
     * @brief Write a parcelable object to the given parcel.
     *
     * @param parcel Target parcel to write to.
     * @return Return true being written on success or false if any error occur.
     * @note If this object is remote, its position will be saved in the parcel.
     * @note A static Unmarshalling function must also be implemented, so that
     * you can get data from the given parcel into this parcelable object.
     * See `static TestParcelable *Unmarshalling(Parcel &parcel)` as an example.
     */
    virtual bool Marshalling(Parcel &parcel) const = 0;

    /**
     * @brief Enum flag to describe behaviors of the Parcelable object.
     *
     * @var IPC Indicate the object can be used in IPC.
     * @var RPC Indicate the object can be used in RPC.
     * @var HOLD_OBJECT Indicate the object will ensure
     * alive during data transaction.
     *
     */
    enum BehaviorFlag { IPC = 0x01, RPC = 0x02, HOLD_OBJECT = 0x10 };

    /**
     * @brief Enable specified behavior.
     *
     * @param b Specific flag value.
     * @see BehaviorFlag.
     */
    inline void SetBehavior(BehaviorFlag b) const
    {
        behavior_ |= static_cast<uint8_t>(b);
    }

    /**
     * @brief Disable specified behavior.
     *
     * @param b Specific flag value. See BehaviorFlag.
     */
    inline void ClearBehavior(BehaviorFlag b) const
    {
        behavior_ &= static_cast<uint8_t>(~b);
    }

    /**
     * @brief Check whether specified behavior is enabled.
     *
     * @param b Specific flag value.
     * @see BehaviorFlag.
     */
    inline bool TestBehavior(BehaviorFlag b) const
    {
        return behavior_ & (static_cast<uint8_t>(b));
    }

public:
    bool asRemote_; // if the object is remote
    mutable uint8_t behavior_; // value of flag of behaviors
};

/**
 * @brief Interface for memory allocator of the data in Parcel.
 */
class Allocator {
public:
    virtual ~Allocator() = default;

    virtual void *Realloc(void *data, size_t newSize) = 0;

    virtual void *Alloc(size_t size) = 0;

    virtual void Dealloc(void *data) = 0;
};

/**
 * @brief Default implement of Allocator.
 *
 * @note Allocator defined by user for a parcel need be specified manually.
 */
class DefaultAllocator : public Allocator {
public:
    /**
     * @brief Use `malloc()` to allocate memory for a parcel.
     *
     * @param size Size of desired memory region.
     * @return Void-type pointer to the memory region.
     */
    void *Alloc(size_t size) override;

    /**
     * @brief Use `free()` to deallocate memory for a parcel.
     *
     * @param data Void-type pointer to the memory region.
     */
    void Dealloc(void *data) override;
private:
    /**
     * @brief Use `realloc()` to reallocate memory for a parcel.
     *
     * @param data Void-type pointer to the existed memory region.
     * @param newSize New desired size of memory region.
     * @return Void-type pointer to the new memory region.
     */
    void *Realloc(void *data, size_t newSize) override;
};

/**
 * @brief A data/message container.
 *
 * Contains interfaces for writing and reading data of various types,
 * including primitives, Parcelable objects etc.
 *
 * @note Usually used in IPC, RPC.
 */
class Parcel {
public:
    Parcel() {}

    /**
     * @brief Construct a new Parcel object with specified Allocator.
     *
     * @param allocator Specified Allocator.
     */
    explicit Parcel(Allocator *allocator)
    {
        (void)allocator;
    }

    ~Parcel();
    size_t GetDataSize() const;
    uintptr_t GetData() const;
    binder_size_t GetObjectOffsets() const;
    size_t GetOffsetsSize() const;
    size_t GetWritableBytes() const;
    size_t GetReadableBytes() const;
    size_t GetDataCapacity() const;
    size_t GetMaxCapacity() const;
    bool SetDataCapacity(size_t newCapacity);
    bool SetDataSize(size_t dataSize);
    bool SetMaxCapacity(size_t maxCapacity);
    bool WriteBool(bool value);
    bool WriteInt8(int8_t value);
    bool WriteInt16(int16_t value);
    bool WriteInt32(int32_t value);
    bool WriteInt64(int64_t value);
    bool WriteUint8(uint8_t value);
    bool WriteUint16(uint16_t value);
    bool WriteUint32(uint32_t value);
    bool WriteUint64(uint64_t value);
    bool WriteFloat(float value);
    bool WriteDouble(double value);
    bool WritePointer(uintptr_t value);
    bool WriteBuffer(const void *data, size_t size);
    bool WriteBufferAddTerminator(const void *data, size_t size, size_t typeSize);
    bool WriteUnpadBuffer(const void *data, size_t size);
    bool WriteCString(const char *value);
    bool WriteString(const std::string &value);
    bool WriteString16(const std::u16string &value);
    bool WriteString16WithLength(const char16_t *value, size_t len);
    bool WriteString8WithLength(const char *value, size_t len);
    bool WriteParcelable(const Parcelable *object);
    bool WriteStrongParcelable(const sptr<Parcelable> &object);
    bool WriteRemoteObject(const Parcelable *object);
    template<typename T>
    bool WriteObject(const sptr<T> &object);
    bool ParseFrom(uintptr_t data, size_t size);
    bool ReadBool();
    int8_t ReadInt8();
    int16_t ReadInt16();
    int32_t ReadInt32();
    int64_t ReadInt64();
    uint8_t ReadUint8()
    {
        if (writable_ == true) {
            return 0;
        }
        return NUM_TEST;
    }
    uint16_t ReadUint16();
    uint32_t ReadUint32();
    uint64_t ReadUint64();
    float ReadFloat();
    double ReadDouble();
    uintptr_t ReadPointer();
    bool ReadBool(bool &value);
    bool ReadInt8(int8_t &value);
    bool ReadInt16(int16_t &value);
    bool ReadInt32(int32_t &value);
    bool ReadInt64(int64_t &value);
    bool ReadUint8(uint8_t &value);
    bool ReadUint16(uint16_t &value);
    bool ReadUint32(uint32_t &value);
    bool ReadUint64(uint64_t &value);
    bool ReadFloat(float &value);
    bool ReadDouble(double &value);
    const uint8_t *ReadBuffer(size_t length);
    const uint8_t *ReadUnpadBuffer(size_t length);
    void SkipBytes(size_t bytes);
    const char *ReadCString();
    const std::string ReadString();
    bool ReadString(std::string &value);
    const std::u16string ReadString16();
    bool ReadString16(std::u16string &value);
    const std::u16string ReadString16WithLength(int32_t &len);
    const std::string ReadString8WithLength(int32_t &len);
    bool RewindRead(size_t newPosition);
    bool RewindWrite(size_t offsets);
    size_t GetReadPosition();
    size_t GetWritePosition();
    template <typename T>
    T *ReadParcelable();

    template <typename T>
    sptr<T> ReadStrongParcelable();

    bool CheckOffsets();

    template<typename T>
    sptr<T> ReadObject();

    bool SetAllocator(Allocator *allocator);

    void InjectOffsets(binder_size_t offsets, size_t offsetSize);
    void FlushBuffer();
    template <typename T1, typename T2>
    bool WriteVector(const std::vector<T1> &val, bool (Parcel::*write)(T2));
    bool WriteBoolVector(const std::vector<bool> &val);
    bool WriteInt8Vector(const std::vector<int8_t> &val);
    bool WriteInt16Vector(const std::vector<int16_t> &val);
    bool WriteInt32Vector(const std::vector<int32_t> &val);
    bool WriteInt64Vector(const std::vector<int64_t> &val);
    bool WriteUInt8Vector(const std::vector<uint8_t> &val);
    bool WriteUInt16Vector(const std::vector<uint16_t> &val);
    bool WriteUInt32Vector(const std::vector<uint32_t> &val);
    bool WriteUInt64Vector(const std::vector<uint64_t> &val);
    bool WriteFloatVector(const std::vector<float> &val);
    bool WriteDoubleVector(const std::vector<double> &val);
    bool WriteStringVector(const std::vector<std::string> &val);
    bool WriteString16Vector(const std::vector<std::u16string> &val);
    template <typename T>
    bool ReadVector(std::vector<T> *val, bool (Parcel::*read)(T &));
    bool ReadBoolVector(std::vector<bool> *val);
    bool ReadInt8Vector(std::vector<int8_t> *val);
    bool ReadInt16Vector(std::vector<int16_t> *val);
    bool ReadInt32Vector(std::vector<int32_t> *val);
    bool ReadInt64Vector(std::vector<int64_t> *val);
    bool ReadUInt8Vector(std::vector<uint8_t> *val);
    bool ReadUInt16Vector(std::vector<uint16_t> *val);
    bool ReadUInt32Vector(std::vector<uint32_t> *val);
    bool ReadUInt64Vector(std::vector<uint64_t> *val);
    bool ReadFloatVector(std::vector<float> *val);
    bool ReadDoubleVector(std::vector<double> *val);
    bool ReadStringVector(std::vector<std::string> *val);
    bool ReadString16Vector(std::vector<std::u16string> *val);
    bool WriteBoolUnaligned(bool value);
    bool WriteInt8Unaligned(int8_t value);
    bool WriteInt16Unaligned(int16_t value);
    bool WriteUint8Unaligned(uint8_t value);
    bool WriteUint16Unaligned(uint16_t value);
    bool ReadBoolUnaligned();
    bool ReadInt8Unaligned(int8_t &value);
    bool ReadInt16Unaligned(int16_t &value);
    bool ReadUint8Unaligned(uint8_t &value);
    bool ReadUint16Unaligned(uint16_t &value);

protected:
    bool WriteObjectOffset(binder_size_t offset);
    bool EnsureObjectsCapacity();
private:
    DISALLOW_COPY_AND_MOVE(Parcel);
    template <typename T>
    bool Write(T value);

    template <typename T>
    bool Read(T &value);

    template <typename T>
    T Read();

    template <typename T>
    bool ReadPadded(T &value);

    inline size_t GetPadSize(size_t size)
    {
        const size_t sizeOffset = 3;
        return (((size + sizeOffset) & (~sizeOffset)) - size);
    }

    size_t CalcNewCapacity(size_t minCapacity);

    bool WriteDataBytes(const void *data, size_t size);

    void WritePadBytes(size_t padded);

    bool EnsureWritableCapacity(size_t desireCapacity);

    bool WriteParcelableOffset(size_t offset);

private:
    uint8_t *data_;
    size_t readCursor_;
    size_t writeCursor_;
    size_t dataSize_;
    size_t dataCapacity_;
    size_t maxDataCapacity_;
    binder_size_t *objectOffsets_;
    size_t objectCursor_;
    size_t objectsCapacity_;
    Allocator *allocator_;
    std::vector<sptr<Parcelable>> objectHolder_;
    bool writable_ = true;
};

template <typename T>
bool Parcel::WriteObject(const sptr<T> &object)
{
    if (object == nullptr) {
        return T::Marshalling(*this, object);
    }
    return WriteRemoteObject(object.GetRefPtr());
}

template <typename T>
sptr<T> Parcel::ReadObject()
{
    if (!this->CheckOffsets()) {
        return nullptr;
    }
    sptr<T> res(T::Unmarshalling(*this));
    return res;
}

// Read data from the given parcel into this parcelable object.
template <typename T>
T *Parcel::ReadParcelable()
{
    int32_t size = this->ReadInt32();
    if (size == 0) {
        return nullptr;
    }
    return T::Unmarshalling(*this);
}

// Read data from the given parcel into this parcelable object, return sptr.
template <typename T>
sptr<T> Parcel::ReadStrongParcelable()
{
    int32_t size = this->ReadInt32();
    if (size == 0) {
        return nullptr;
    }
    sptr<T> res(T::Unmarshalling(*this));
    return res;
}

} // namespace OHOS
#endif

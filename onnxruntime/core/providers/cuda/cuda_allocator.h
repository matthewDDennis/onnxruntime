// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <unordered_set>
#include "core/framework/allocator.h"
#include "core/platform/ort_mutex.h"

namespace onnxruntime {

class CUDAAllocator : public IAllocator {
 public:
  CUDAAllocator(OrtDevice::DeviceId device_id, const char* name)
      : IAllocator(
            OrtMemoryInfo(name, OrtAllocatorType::OrtDeviceAllocator,
                          OrtDevice(OrtDevice::GPU, OrtDevice::MemType::DEFAULT, device_id),
                          device_id, OrtMemTypeDefault)) {}
  void* Alloc(size_t size) override;
  void Free(void* p) override;
  FencePtr CreateFence(const SessionState* session_state) override;

 private:
  void CheckDevice(bool throw_when_fail) const;
  void SetDevice(bool throw_when_fail) const;
};

class CUDAExternalAllocator : public CUDAAllocator {
  typedef void* (*ExternalAlloc)(size_t size);
  typedef void (*ExternalFree)(void* p);
  typedef void (*ExternalEmptyCache)();

 public:
  CUDAExternalAllocator(OrtDevice::DeviceId device_id, const char* name, void* alloc, void* free, void* empty_cache)
      : CUDAAllocator(device_id, name) {
    alloc_ = reinterpret_cast<ExternalAlloc>(alloc);
    free_ = reinterpret_cast<ExternalFree>(free);
    empty_cache_ = reinterpret_cast<ExternalEmptyCache>(empty_cache);
  }

  CUDAExternalAllocator(OrtDevice::DeviceId device_id, const char* name, ExternalAlloc alloc, ExternalFree free, ExternalEmptyCache empty_cache)
      : CUDAAllocator(device_id, name) {
    alloc_ = alloc;
    free_ = free;
    empty_cache_ = empty_cache;
  }

  void* Alloc(size_t size) override;
  void Free(void* p) override;
  void* Reserve(size_t size) override;

 private:
  mutable OrtMutex lock_;
  ExternalAlloc alloc_;
  ExternalFree free_;
  ExternalEmptyCache empty_cache_;
  std::unordered_set<void*> reserved_;
};

//TODO: add a default constructor
class CUDAPinnedAllocator : public IAllocator {
 public:
  CUDAPinnedAllocator(OrtDevice::DeviceId device_id, const char* name)
      : IAllocator(
            OrtMemoryInfo(name, OrtAllocatorType::OrtDeviceAllocator,
                          OrtDevice(OrtDevice::CPU, OrtDevice::MemType::CUDA_PINNED, device_id),
                          device_id, OrtMemTypeCPUOutput)) {}

  void* Alloc(size_t size) override;
  void Free(void* p) override;
  FencePtr CreateFence(const SessionState* session_state) override;
};
}  // namespace onnxruntime

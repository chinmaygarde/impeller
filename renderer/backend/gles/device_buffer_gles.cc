// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/gles/device_buffer_gles.h"

#include "impeller/base/allocation.h"
#include "impeller/base/config.h"

namespace impeller {

DeviceBufferGLES::DeviceBufferGLES(ReactorGLES::Ref reactor,
                                   size_t size,
                                   StorageMode mode)
    : DeviceBuffer(size, mode),
      reactor_(std::move(reactor)),
      handle_(reactor_ ? reactor_->CreateHandle(HandleType::kBuffer)
                       : GLESHandle::DeadHandle()) {}

// |DeviceBuffer|
DeviceBufferGLES::~DeviceBufferGLES() {
  if (!reactor_) {
    reactor_->CollectHandle(handle_);
  }
}

// |DeviceBuffer|
bool DeviceBufferGLES::CopyHostBuffer(const uint8_t* source,
                                      Range source_range,
                                      size_t offset) {
  if (mode_ != StorageMode::kHostVisible) {
    // One of the storage modes where a transfer queue must be used.
    return false;
  }

  if (offset + source_range.length > size_) {
    // Out of bounds of this buffer.
    return false;
  }

  if (!reactor_) {
    return false;
  }

  auto mapping =
      CreateMappingWithCopy(source + source_range.offset, source_range.length);
  if (!mapping) {
    return false;
  }

  return reactor_->AddOperation([mapping](const auto& reactor) {});
}

// |DeviceBuffer|
bool DeviceBufferGLES::SetLabel(const std::string& label) {
  // Cannot support.
  return true;
}

// |DeviceBuffer|
bool DeviceBufferGLES::SetLabel(const std::string& label, Range range) {
  // Cannot support.
  return true;
}

}  // namespace impeller

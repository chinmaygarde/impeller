// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "flutter/fml/macros.h"
#include "impeller/base/backend_cast.h"
#include "impeller/renderer/backend/gles/gles_handle.h"
#include "impeller/renderer/backend/gles/reactor_gles.h"
#include "impeller/renderer/backend/gles/vertex_descriptor_gles.h"
#include "impeller/renderer/pipeline.h"

namespace impeller {

class PipelineLibraryGLES;

class PipelineGLES final : public Pipeline,
                           public BackendCast<PipelineGLES, Pipeline> {
 public:
  // |Pipeline|
  ~PipelineGLES() override;

  const GLESHandle& GetProgramHandle() const;

  const VertexDescriptorGLES* GetVertexDescriptorGLES() const;

  [[nodiscard]] bool BuildVertexDescriptor(const ProcTableGLES& gl,
                                           GLuint program);

 private:
  friend PipelineLibraryGLES;

  ReactorGLES::Ref reactor_;
  GLESHandle handle_;
  std::unique_ptr<VertexDescriptorGLES> vertex_descriptor_gles_;
  bool is_valid_ = false;

  // |Pipeline|
  bool IsValid() const override;

  PipelineGLES(ReactorGLES::Ref reactor,
               std::weak_ptr<PipelineLibrary> library,
               PipelineDescriptor desc);

  FML_DISALLOW_COPY_AND_ASSIGN(PipelineGLES);
};

}  // namespace impeller

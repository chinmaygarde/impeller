// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/gles/pipeline_gles.h"

namespace impeller {

PipelineGLES::PipelineGLES(ReactorGLES::Ref reactor,
                           std::weak_ptr<PipelineLibrary> library,
                           PipelineDescriptor desc)
    : Pipeline(std::move(library), std::move(desc)),
      reactor_(std::move(reactor)),
      handle_(reactor_ ? reactor_->CreateHandle(HandleType::kProgram)
                       : GLESHandle::DeadHandle()),
      is_valid_(!handle_.IsDead()) {}

// |Pipeline|
PipelineGLES::~PipelineGLES() {
  if (!handle_.IsDead()) {
    reactor_->CollectHandle(std::move(handle_));
  }
}

// |Pipeline|
bool PipelineGLES::IsValid() const {
  return is_valid_;
}

const GLESHandle& PipelineGLES::GetProgramHandle() const {
  return handle_;
}

const VertexDescriptorGLES* PipelineGLES::GetVertexDescriptorGLES() const {
  return vertex_descriptor_gles_.get();
}

bool PipelineGLES::BuildVertexDescriptor(const ProcTableGLES& gl,
                                         GLuint program) {
  if (vertex_descriptor_gles_) {
    return false;
  }
  auto vtx_desc = std::make_unique<VertexDescriptorGLES>();
  if (!vtx_desc->RegisterVertexStageInput(
          gl, GetDescriptor().GetVertexDescriptor()->GetStageInputs())) {
    return false;
  }
  if (!vtx_desc->ReadUniformsBindings(gl, program)) {
    return false;
  }
  vertex_descriptor_gles_ = std::move(vtx_desc);
  return true;
}

}  // namespace impeller

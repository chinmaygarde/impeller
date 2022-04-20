// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/gles/vertex_descriptor_gles.h"

#include "impeller/base/config.h"
#include "impeller/renderer/backend/gles/formats_gles.h"

namespace impeller {

VertexDescriptorGLES::VertexDescriptorGLES() = default;

VertexDescriptorGLES::~VertexDescriptorGLES() = default;

bool VertexDescriptorGLES::RegisterVertexStageInput(
    const ProcTableGLES& gl,
    const std::vector<ShaderStageIOSlot>& inputs) {
  std::vector<VertexAttribPointer> vertex_attrib_arrays;
  size_t stride = 0u;
  for (const auto& input : inputs) {
    VertexAttribPointer attrib;
    attrib.index = input.location;
    // Component counts must be 1, 2, 3 or 4. Do that validation now.
    if (input.vec_size < 1u || input.vec_size > 4u) {
      return false;
    }
    attrib.size = input.vec_size;
    auto type = ToVertexAttribType(input.type);
    if (!type.has_value()) {
      return false;
    }
    attrib.type = type.value();
    attrib.normalized = GL_FALSE;
    attrib.stride = stride;
    stride += (input.bit_width * input.vec_size) / 8;
    attrib.offset = 0;  // FIXME. This must be removed.
    vertex_attrib_arrays.emplace_back(std::move(attrib));
  }
  vertex_attrib_arrays_ = std::move(vertex_attrib_arrays);
  return true;
}

bool VertexDescriptorGLES::ReadUniformsBindings(const ProcTableGLES& gl,
                                                GLuint program) {
  if (!gl.IsProgram(program)) {
    return false;
  }
  return true;
}

bool VertexDescriptorGLES::Bind(const ProcTableGLES& gl) const {
  for (const auto& array : vertex_attrib_arrays_) {
    gl.EnableVertexAttribArray(array.index);
    gl.VertexAttribPointer(
        array.index,       // index
        array.size,        // size (must be 1, 2, 3, or 4)
        array.type,        // type
        array.normalized,  // normalized
        array.stride,      // stride
        reinterpret_cast<const GLvoid*>(array.offset)  // pointer
    );
  }
  return true;
}

bool VertexDescriptorGLES::Unbind(const ProcTableGLES& gl) const {
  for (const auto& array : vertex_attrib_arrays_) {
    gl.DisableVertexAttribArray(array.index);
  }
  return true;
}

}  // namespace impeller

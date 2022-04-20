// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vector>

#include "flutter/fml/macros.h"
#include "impeller/renderer/backend/gles/gles.h"
#include "impeller/renderer/backend/gles/proc_table_gles.h"
#include "impeller/renderer/vertex_descriptor.h"

namespace impeller {

class VertexDescriptorGLES {
 public:
  VertexDescriptorGLES();

  ~VertexDescriptorGLES();

  bool RegisterVertexStageInput(const ProcTableGLES& gl,
                                const std::vector<ShaderStageIOSlot>& inputs);

  bool ReadUniformsBindings(const ProcTableGLES& gl, GLuint program);

  bool Bind(const ProcTableGLES& gl) const;

  bool Unbind(const ProcTableGLES& gl) const;

 private:
  //----------------------------------------------------------------------------
  /// @brief      The arguments to glVertexAttribPointer.
  ///
  struct VertexAttribPointer {
    GLuint index = 0u;
    GLint size = 4;
    GLenum type = GL_FLOAT;
    GLenum normalized = GL_FALSE;
    GLsizei stride = 0u;
    GLsizei offset = 0u;
  };
  std::vector<VertexAttribPointer> vertex_attrib_arrays_;
  FML_DISALLOW_COPY_AND_ASSIGN(VertexDescriptorGLES);
};

}  // namespace impeller

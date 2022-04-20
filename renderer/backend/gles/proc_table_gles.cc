// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/gles/proc_table_gles.h"

#include "impeller/base/validation.h"

namespace impeller {

const char* GLErrorToString(GLenum value) {
  switch (value) {
    case GL_NO_ERROR:
      return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_FRAMEBUFFER_COMPLETE:
      return "GL_FRAMEBUFFER_COMPLETE";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
  }
  return "Unknown.";
}

ProcTableGLES::ProcTableGLES(Resolver resolver) {
  if (!resolver) {
    return;
  }

  auto error_fn = reinterpret_cast<PFNGLGETERRORPROC>(resolver("glGetError"));
  if (!error_fn) {
    VALIDATION_LOG << "Could not resolve "
                   << "glGetError";
    return;
  }

#define IMPELLER_PROC(proc_ivar)                                \
  if (auto fn_ptr = resolver(proc_ivar.name)) {                 \
    proc_ivar.function =                                        \
        reinterpret_cast<decltype(proc_ivar.function)>(fn_ptr); \
    proc_ivar.error_fn = error_fn;                              \
  } else {                                                      \
    VALIDATION_LOG << "Could not resolve " << proc_ivar.name;   \
    return;                                                     \
  }

  FOR_EACH_IMPELLER_PROC(IMPELLER_PROC);

#undef IMPELLER_PROC

  is_valid_ = true;

  description_ = std::make_unique<GLDescription>(*this);
}

ProcTableGLES::~ProcTableGLES() = default;

bool ProcTableGLES::IsValid() const {
  return is_valid_;
}

void ProcTableGLES::ShaderSourceMapping(GLuint shader,
                                        const fml::Mapping& mapping) const {
  const GLchar* sources[] = {
      reinterpret_cast<const GLchar*>(mapping.GetMapping())};
  const GLint lengths[] = {static_cast<GLint>(mapping.GetSize())};
  ShaderSource(shader, 1u, sources, lengths);
}

const GLDescription* ProcTableGLES::GetDescription() const {
  return description_.get();
}

}  // namespace impeller

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <vector>

#include "flutter/fml/logging.h"
#include "flutter/fml/macros.h"
#include "flutter/fml/mapping.h"
#include "impeller/renderer/backend/gles/gl_description.h"
#include "impeller/renderer/backend/gles/gles.h"

namespace impeller {

const char* GLErrorToString(GLenum value);

struct AutoErrorCheck {
  const PFNGLGETERRORPROC error_fn;
  const char* name;

  AutoErrorCheck(PFNGLGETERRORPROC error, const char* name)
      : error_fn(error), name(name) {}

  ~AutoErrorCheck() {
    if (error_fn) {
      auto error = error_fn();
      FML_CHECK(error == GL_NO_ERROR)
          << "GL Error " << GLErrorToString(error) << "(" << error << ")"
          << " encountered on call to " << name;
    }
  }
};

template <class T>
struct GLProc {
  using GLFunctionType = T;

  //----------------------------------------------------------------------------
  /// The name of the GL function.
  ///
  const char* name = nullptr;

  //----------------------------------------------------------------------------
  /// The pointer to the GL function.
  ///
  GLFunctionType* function = nullptr;

  //----------------------------------------------------------------------------
  /// An optional error function. If present, all calls will be followed by an
  /// error check.
  ///
  PFNGLGETERRORPROC error_fn = nullptr;

  //----------------------------------------------------------------------------
  /// @brief      Call the GL function with the appropriate parameters. Lookup
  ///             the documentation for the GL function being called to
  ///             understand the arguments and return types. The arguments
  ///             types must match and will be type checked.
  ///
  template <class... Args>
  auto operator()(Args&&... args) const {
    FML_DCHECK(function) << "Function named "
                         << (name != nullptr ? name : "<null>")
                         << " unavailable.";
    AutoErrorCheck error(error_fn, name);
    return function(std::forward<Args>(args)...);
  }
};

#define FOR_EACH_IMPELLER_PROC(PROC)         \
  PROC(GenTextures);                         \
  PROC(DeleteTextures);                      \
  PROC(GetFramebufferAttachmentParameteriv); \
  PROC(GetBooleanv);                         \
  PROC(GetFloatv);                           \
  PROC(GetIntegerv);                         \
  PROC(GetString);                           \
  PROC(GenBuffers);                          \
  PROC(DeleteBuffers);                       \
  PROC(CullFace);                            \
  PROC(Enable);                              \
  PROC(Disable);                             \
  PROC(FrontFace);                           \
  PROC(DrawElements);                        \
  PROC(Viewport);                            \
  PROC(DepthRangef);                         \
  PROC(Scissor);                             \
  PROC(StencilFuncSeparate);                 \
  PROC(StencilOpSeparate);                   \
  PROC(StencilMaskSeparate);                 \
  PROC(DepthFunc);                           \
  PROC(DepthMask);                           \
  PROC(BlendFuncSeparate);                   \
  PROC(BlendEquationSeparate);               \
  PROC(ColorMask);                           \
  PROC(CreateProgram);                       \
  PROC(DeleteProgram);                       \
  PROC(CreateShader);                        \
  PROC(DeleteShader);                        \
  PROC(ShaderSource);                        \
  PROC(ShaderBinary);                        \
  PROC(CompileShader);                       \
  PROC(AttachShader);                        \
  PROC(DetachShader);                        \
  PROC(GetShaderInfoLog);                    \
  PROC(GetShaderiv);                         \
  PROC(GetProgramiv);                        \
  PROC(BindAttribLocation);                  \
  PROC(LinkProgram);                         \
  PROC(EnableVertexAttribArray);             \
  PROC(DisableVertexAttribArray);            \
  PROC(VertexAttribPointer);                 \
  PROC(IsProgram);                           \
  PROC(IsFramebuffer);

class ProcTableGLES {
 public:
  using Resolver = std::function<void*(const char* function_name)>;
  ProcTableGLES(Resolver resolver);

  ~ProcTableGLES();

#define IMPELLER_PROC(name) \
  GLProc<decltype(gl##name)> name = {"gl" #name, nullptr};

  FOR_EACH_IMPELLER_PROC(IMPELLER_PROC);

#undef IMPELLER_PROC

  bool IsValid() const;

  void ShaderSourceMapping(GLuint shader, const fml::Mapping& mapping) const;

  const GLDescription* GetDescription() const;

 private:
  bool is_valid_ = false;
  std::unique_ptr<GLDescription> description_;

  FML_DISALLOW_COPY_AND_ASSIGN(ProcTableGLES);
};

}  // namespace impeller

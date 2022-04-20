// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/gles/render_pass_gles.h"

#include <algorithm>

#include "flutter/fml/trace_event.h"
#include "impeller/base/config.h"
#include "impeller/base/validation.h"
#include "impeller/renderer/backend/gles/formats_gles.h"
#include "impeller/renderer/backend/gles/pipeline_gles.h"

namespace impeller {

RenderPassGLES::RenderPassGLES(RenderTarget target, ReactorGLES::Ref reactor)
    : RenderPass(std::move(target)),
      reactor_(std::move(reactor)),
      is_valid_(reactor_ && reactor_->IsValid()) {}

// |RenderPass|
RenderPassGLES::~RenderPassGLES() = default;

// |RenderPass|
bool RenderPassGLES::IsValid() const {
  return is_valid_;
}

// |RenderPass|
void RenderPassGLES::SetLabel(std::string label) {
  // Cannot support.
}

static void ConfigureBlending(const ProcTableGLES& gl,
                              const ColorAttachmentDescriptor* color) {
  if (!color->blending_enabled) {
    gl.Disable(GL_BLEND);
    return;
  }

  gl.Enable(GL_BLEND);
  gl.BlendFuncSeparate(
      ToBlendFactor(color->src_color_blend_factor),  // src color
      ToBlendFactor(color->dst_color_blend_factor),  // dst color
      ToBlendFactor(color->src_alpha_blend_factor),  // src alpha
      ToBlendFactor(color->dst_alpha_blend_factor)   // dst alpha
  );
  gl.BlendEquationSeparate(
      ToBlendOperation(color->color_blend_op),  // mode color
      ToBlendOperation(color->alpha_blend_op)   // mode alpha
  );
  {
    const auto is_set = [](std::underlying_type_t<ColorWriteMask> mask,
                           ColorWriteMask check) -> GLboolean {
      using RawType = decltype(mask);
      return (static_cast<RawType>(mask) & static_cast<RawType>(mask))
                 ? GL_TRUE
                 : GL_FALSE;
    };

    gl.ColorMask(is_set(color->write_mask, ColorWriteMask::kRed),    // red
                 is_set(color->write_mask, ColorWriteMask::kGreen),  // green
                 is_set(color->write_mask, ColorWriteMask::kBlue),   // blue
                 is_set(color->write_mask, ColorWriteMask::kAlpha)   // alpha
    );
  }
}

static void ConfigureStencil(GLenum face,
                             const ProcTableGLES& gl,
                             const StencilAttachmentDescriptor& stencil,
                             uint32_t stencil_reference) {
  gl.StencilOpSeparate(
      face,                                    // face
      ToStencilOp(stencil.stencil_failure),    // stencil fail
      ToStencilOp(stencil.depth_failure),      // depth fail
      ToStencilOp(stencil.depth_stencil_pass)  // depth stencil pass
  );
  gl.StencilFuncSeparate(face,                                        // face
                         ToCompareFunction(stencil.stencil_compare),  // func
                         stencil_reference,                           // ref
                         stencil.read_mask                            // mask
  );
  gl.StencilMaskSeparate(face, stencil.write_mask);
}

static void ConfigureStencil(const ProcTableGLES& gl,
                             const PipelineDescriptor& pipeline,
                             uint32_t stencil_reference) {
  if (!pipeline.HasStencilAttachmentDescriptors()) {
    gl.Disable(GL_STENCIL_TEST);
    return;
  }

  gl.Enable(GL_STENCIL_TEST);
  const auto& front = pipeline.GetFrontStencilAttachmentDescriptor();
  const auto& back = pipeline.GetBackStencilAttachmentDescriptor();
  if (front == back) {
    ConfigureStencil(GL_FRONT_AND_BACK, gl, *front, stencil_reference);
  } else if (front.has_value()) {
    ConfigureStencil(GL_FRONT, gl, *front, stencil_reference);
  } else if (back.has_value()) {
    ConfigureStencil(GL_BACK, gl, *back, stencil_reference);
  } else {
    FML_UNREACHABLE();
  }
}

static bool EncodeCommandsInReactor(const ReactorGLES& reactor,
                                    const std::vector<Command>& commands) {
  TRACE_EVENT0("impeller", __FUNCTION__);

  const auto& gl = reactor.GetProcTable();

  for (const auto& command : commands) {
    if (command.instance_count != 1u) {
      VALIDATION_LOG << "GLES backend does not support instanced rendering.";
      return false;
    }

    if (!command.pipeline) {
      VALIDATION_LOG << "Command has no pipeline specified.";
      return false;
    }

    const auto& pipeline = PipelineGLES::Cast(*command.pipeline);

    const auto* color_attachment =
        pipeline.GetDescriptor().GetLegacyCompatibleColorAttachment();
    if (!color_attachment) {
      VALIDATION_LOG
          << "Color attachment is too complicated for a legacy renderer.";
      return false;
    }

    //--------------------------------------------------------------------------
    /// Configure blending.
    ///
    ConfigureBlending(gl, color_attachment);

    //--------------------------------------------------------------------------
    /// Setup stencil.
    ///
    ConfigureStencil(gl, pipeline.GetDescriptor(), command.stencil_reference);

    //--------------------------------------------------------------------------
    /// Configure depth.
    ///
    if (auto depth =
            pipeline.GetDescriptor().GetDepthStencilAttachmentDescriptor();
        depth.has_value()) {
      gl.Enable(GL_DEPTH_TEST);
      gl.DepthFunc(ToCompareFunction(depth->depth_compare));
      gl.DepthMask(depth->depth_write_enabled ? GL_TRUE : GL_FALSE);
    } else {
      gl.Disable(GL_DEPTH_TEST);
    }

    //--------------------------------------------------------------------------
    /// Setup the viewport.
    ///
    if (command.viewport.has_value()) {
      const auto& viewport = command.viewport.value();
      gl.Viewport(viewport.rect.origin.x,    // x
                  viewport.rect.origin.y,    // y
                  viewport.rect.size.width,  // width
                  viewport.rect.size.height  // height
      );
      // TODO(csg): Invalid operation on Mac.
      // gl.DepthRangef(viewport.znear, viewport.zfar);
    } else {
      // TODO(csg): This needs to be reset per call but we need the render
      // target size.
    }

    //--------------------------------------------------------------------------
    /// Setup the scissor rect.
    ///
    if (command.scissor.has_value()) {
      const auto& scissor = command.scissor.value();
      gl.Enable(GL_SCISSOR_TEST);
      gl.Scissor(scissor.origin.x,    // x
                 scissor.origin.y,    // y
                 scissor.size.width,  // width
                 scissor.size.width   // height
      );
    } else {
      gl.Disable(GL_SCISSOR_TEST);
    }

    //--------------------------------------------------------------------------
    /// Setup culling
    ///
    switch (command.cull_mode) {
      case CullMode::kNone:
        gl.Disable(GL_CULL_FACE);
        break;
      case CullMode::kFrontFace:
        gl.Enable(GL_CULL_FACE);
        gl.CullFace(GL_FRONT);
        break;
      case CullMode::kBackFace:
        gl.Enable(GL_CULL_FACE);
        gl.CullFace(GL_BACK);
        break;
    }
    //--------------------------------------------------------------------------
    /// Setup winding order.
    ///
    switch (command.winding) {
      case WindingOrder::kClockwise:
        gl.FrontFace(GL_CW);
        break;
      case WindingOrder::kCounterClockwise:
        gl.FrontFace(GL_CCW);
        break;
    }

    if (command.index_type == IndexType::kUnknown) {
      return false;
    }

    const auto& vertex_desc_gles = pipeline.GetVertexDescriptorGLES();

    //--------------------------------------------------------------------------
    /// Bind vertex and index buffers.
    ///
    auto vertex_buffer_view = command.GetVertexBuffer();
    auto index_buffer_view = command.index_buffer;

    if (!vertex_buffer_view || !index_buffer_view) {
      return false;
    }

    //--------------------------------------------------------------------------
    /// Bind vertex attrib arrays
    ///
    if (!vertex_desc_gles->Bind(gl)) {
      return false;
    }

    //--------------------------------------------------------------------------
    /// Finally! Invoke the draw call.
    ///
    gl.DrawElements(ToMode(command.primitive_type),   //  mode
                    command.index_count,              // count
                    ToIndexType(command.index_type),  // type
                    nullptr                           // indices
    );

    //--------------------------------------------------------------------------
    /// Unbind vertex attrib arrays.
    ///
    if (!vertex_desc_gles->Unbind(gl)) {
      return false;
    }
  }

  return true;
}

// |RenderPass|
bool RenderPassGLES::EncodeCommands(Allocator& transients_allocator) const {
  if (!IsValid()) {
    return false;
  }
  if (commands_.empty()) {
    return true;
  }
  return reactor_->AddOperation([commands = commands_](const auto& reactor) {
    EncodeCommandsInReactor(reactor, commands);
  });
}

}  // namespace impeller

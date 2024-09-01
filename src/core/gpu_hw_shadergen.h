// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#pragma once

#include "gpu_hw.h"

#include "util/shadergen.h"

class GPU_HW_ShaderGen : public ShaderGen
{
public:
  GPU_HW_ShaderGen(RenderAPI render_api, u32 resolution_scale, u32 multisamples, bool per_sample_shading,
                   bool true_color, bool scaled_dithering, bool write_mask_as_depth, bool disable_color_perspective,
                   bool supports_dual_source_blend, bool supports_framebuffer_fetch, bool debanding);
  ~GPU_HW_ShaderGen();

  std::string GenerateBatchVertexShader(bool textured, bool palette, bool uv_limits, bool force_round_texcoords,
                                        bool pgxp_depth);
  std::string GenerateBatchFragmentShader(GPU_HW::BatchRenderMode render_mode, GPUTransparencyMode transparency,
                                          GPU_HW::BatchTextureMode texture_mode, GPUTextureFilter texture_filtering,
                                          bool uv_limits, bool force_round_texcoords, bool dithering, bool interlacing,
                                          bool check_mask, bool use_rov, bool use_rov_depth, bool rov_depth_test);
  std::string GenerateWireframeGeometryShader();
  std::string GenerateWireframeFragmentShader();
  std::string GenerateVRAMReadFragmentShader();
  std::string GenerateVRAMWriteFragmentShader(bool use_buffer, bool use_ssbo);
  std::string GenerateVRAMCopyFragmentShader();
  std::string GenerateVRAMFillFragmentShader(bool wrapped, bool interlaced);
  std::string GenerateVRAMUpdateDepthFragmentShader();
  std::string GenerateVRAMExtractFragmentShader(bool color_24bit, bool depth_buffer);

  std::string GenerateAdaptiveDownsampleVertexShader();
  std::string GenerateAdaptiveDownsampleMipFragmentShader(bool first_pass);
  std::string GenerateAdaptiveDownsampleBlurFragmentShader();
  std::string GenerateAdaptiveDownsampleCompositeFragmentShader();
  std::string GenerateBoxSampleDownsampleFragmentShader(u32 factor);

private:
  ALWAYS_INLINE bool UsingMSAA() const { return m_multisamples > 1; }
  ALWAYS_INLINE bool UsingPerSampleShading() const { return m_multisamples > 1 && m_per_sample_shading; }

  void WriteCommonFunctions(std::stringstream& ss);
  void WriteBatchUniformBuffer(std::stringstream& ss);
  void WriteBatchTextureFilter(std::stringstream& ss, GPUTextureFilter texture_filter);
  void WriteAdaptiveDownsampleUniformBuffer(std::stringstream& ss);

  u32 m_resolution_scale;
  u32 m_multisamples;
  bool m_per_sample_shading;
  bool m_true_color;
  bool m_scaled_dithering;
  bool m_write_mask_as_depth;
  bool m_disable_color_perspective;
  bool m_debanding;
};

// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#include "audio_stream.h"

#include "common/assert.h"
#include "common/error.h"
#include "common/log.h"

#include <SDL.h>

Log_SetChannel(SDLAudioStream);

namespace {
class SDLAudioStream final : public AudioStream
{
public:
  SDLAudioStream(u32 sample_rate, const AudioStreamParameters& parameters);
  ~SDLAudioStream();

  void SetPaused(bool paused) override;

  bool OpenDevice(Error* error);
  void CloseDevice();

protected:
  ALWAYS_INLINE bool IsOpen() const { return (m_device_id != 0); }

  static void AudioCallback(void* userdata, uint8_t* stream, int len);

  u32 m_device_id = 0;
};
} // namespace

static bool InitializeSDLAudio(Error* error)
{
  static bool initialized = false;
  if (initialized)
    return true;

  // May as well keep it alive until the process exits.
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
  {
    Error::SetStringFmt(error, "SDL_InitSubSystem(SDL_INIT_AUDIO) failed: {}", SDL_GetError());
    return false;
  }

  std::atexit([]() { SDL_QuitSubSystem(SDL_INIT_AUDIO); });

  initialized = true;
  return true;
}

SDLAudioStream::SDLAudioStream(u32 sample_rate, const AudioStreamParameters& parameters)
  : AudioStream(sample_rate, parameters)
{
}

SDLAudioStream::~SDLAudioStream()
{
  if (IsOpen())
    SDLAudioStream::CloseDevice();
}

std::unique_ptr<AudioStream> AudioStream::CreateSDLAudioStream(u32 sample_rate, const AudioStreamParameters& parameters,
                                                               Error* error)
{
  if (!InitializeSDLAudio(error))
    return {};

  std::unique_ptr<SDLAudioStream> stream = std::make_unique<SDLAudioStream>(sample_rate, parameters);
  if (!stream->OpenDevice(error))
    stream.reset();

  return stream;
}

bool SDLAudioStream::OpenDevice(Error* error)
{
  DebugAssert(!IsOpen());

  SDL_AudioSpec spec = {};
  spec.freq = m_sample_rate;
  spec.channels = NUM_CHANNELS;
  spec.format = AUDIO_S16;
  spec.samples = static_cast<Uint16>(GetBufferSizeForMS(
    m_sample_rate, (m_parameters.output_latency_ms == 0) ? m_parameters.buffer_ms : m_parameters.output_latency_ms));
  spec.callback = AudioCallback;
  spec.userdata = static_cast<void*>(this);

  SDL_AudioSpec obtained_spec = {};
  m_device_id = SDL_OpenAudioDevice(nullptr, 0, &spec, &obtained_spec, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
  if (m_device_id == 0)
  {
    Error::SetStringFmt(error, "SDL_OpenAudioDevice() failed: {}", SDL_GetError());
    return false;
  }

  DEV_LOG("Requested {} frame buffer, got {} frame buffer", spec.samples, obtained_spec.samples);

  BaseInitialize();
  SDL_PauseAudioDevice(m_device_id, 0);

  return true;
}

void SDLAudioStream::SetPaused(bool paused)
{
  if (m_paused == paused)
    return;

  SDL_PauseAudioDevice(m_device_id, paused ? 1 : 0);
  m_paused = paused;
}

void SDLAudioStream::CloseDevice()
{
  SDL_CloseAudioDevice(m_device_id);
  m_device_id = 0;
}

void SDLAudioStream::AudioCallback(void* userdata, uint8_t* stream, int len)
{
  SDLAudioStream* const this_ptr = static_cast<SDLAudioStream*>(userdata);
  const u32 num_frames = len / sizeof(SampleType) / NUM_CHANNELS;

  this_ptr->ReadFrames(reinterpret_cast<SampleType*>(stream), num_frames);
}

// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#include "host.h"
#include "imgui_manager.h"

#include "core/settings.h"

#include "common/assert.h"
#include "common/error.h"
#include "common/log.h"
#include "common/scoped_guard.h"
#include "common/string_util.h"

#include "cubeb/cubeb.h"
#include "fmt/format.h"

Log_SetChannel(CubebAudioStream);

namespace {

class CubebAudioStream : public AudioStream
{
public:
  CubebAudioStream(u32 sample_rate, const AudioStreamParameters& parameters);
  ~CubebAudioStream();

  void SetPaused(bool paused) override;

  bool Initialize(const char* driver_name, const char* device_name, Error* error);

private:
  static void LogCallback(const char* fmt, ...);
  static long DataCallback(cubeb_stream* stm, void* user_ptr, const void* input_buffer, void* output_buffer,
                           long nframes);
  static void StateCallback(cubeb_stream* stream, void* user_ptr, cubeb_state state);

  void DestroyContextAndStream();

  cubeb* m_context = nullptr;
  cubeb_stream* stream = nullptr;
};
} // namespace

static TinyString GetCubebErrorString(int rv)
{
  TinyString ret;
  switch (rv)
  {
    // clang-format off
#define C(e) case e: ret.assign(#e); break
    // clang-format on

    C(CUBEB_OK);
    C(CUBEB_ERROR);
    C(CUBEB_ERROR_INVALID_FORMAT);
    C(CUBEB_ERROR_INVALID_PARAMETER);
    C(CUBEB_ERROR_NOT_SUPPORTED);
    C(CUBEB_ERROR_DEVICE_UNAVAILABLE);

    default:
      return "CUBEB_ERROR_UNKNOWN";

#undef C
  }

  ret.append_format(" ({})", rv);
  return ret;
}

CubebAudioStream::CubebAudioStream(u32 sample_rate, const AudioStreamParameters& parameters)
  : AudioStream(sample_rate, parameters)
{
}

CubebAudioStream::~CubebAudioStream()
{
  DestroyContextAndStream();
}

void CubebAudioStream::LogCallback(const char* fmt, ...)
{
  LargeString str;
  std::va_list ap;
  va_start(ap, fmt);
  str.vsprintf(fmt, ap);
  va_end(ap);
  DEV_LOG(str);
}

void CubebAudioStream::DestroyContextAndStream()
{
  if (stream)
  {
    cubeb_stream_stop(stream);
    cubeb_stream_destroy(stream);
    stream = nullptr;
  }

  if (m_context)
  {
    cubeb_destroy(m_context);
    m_context = nullptr;
  }
}

bool CubebAudioStream::Initialize(const char* driver_name, const char* device_name, Error* error)
{
  cubeb_set_log_callback(CUBEB_LOG_NORMAL, LogCallback);

  int rv =
    cubeb_init(&m_context, "DuckStation", g_settings.audio_driver.empty() ? nullptr : g_settings.audio_driver.c_str());
  if (rv != CUBEB_OK)
  {
    Error::SetStringFmt(error, "Could not initialize cubeb context: {}", GetCubebErrorString(rv));
    return false;
  }

  cubeb_stream_params params = {};
  params.format = CUBEB_SAMPLE_S16LE;
  params.rate = m_sample_rate;
  params.channels = NUM_CHANNELS;
  params.layout = CUBEB_LAYOUT_STEREO;
  params.prefs = CUBEB_STREAM_PREF_NONE;

  u32 latency_frames = GetBufferSizeForMS(
    m_sample_rate, (m_parameters.output_latency_ms == 0) ? m_parameters.buffer_ms : m_parameters.output_latency_ms);
  u32 min_latency_frames = 0;
  rv = cubeb_get_min_latency(m_context, &params, &min_latency_frames);
  if (rv == CUBEB_ERROR_NOT_SUPPORTED)
  {
    DEV_LOG("Cubeb backend does not support latency queries, using latency of {} ms ({} frames).",
            m_parameters.buffer_ms, latency_frames);
  }
  else
  {
    if (rv != CUBEB_OK)
    {
      Error::SetStringFmt(error, "cubeb_get_min_latency() failed: {}", GetCubebErrorString(rv));
      DestroyContextAndStream();
      return false;
    }

    const u32 minimum_latency_ms = GetMSForBufferSize(m_sample_rate, min_latency_frames);
    DEV_LOG("Minimum latency: {} ms ({} audio frames)", minimum_latency_ms, min_latency_frames);
    if (m_parameters.output_latency_minimal)
    {
      // use minimum
      latency_frames = min_latency_frames;
    }
    else if (minimum_latency_ms > m_parameters.output_latency_ms)
    {
      WARNING_LOG("Minimum latency is above requested latency: {} vs {}, adjusting to compensate.", min_latency_frames,
                  latency_frames);
      latency_frames = min_latency_frames;
    }
  }

  cubeb_devid selected_device = nullptr;
  const std::string& selected_device_name = g_settings.audio_output_device;
  cubeb_device_collection devices;
  bool devices_valid = false;
  if (!selected_device_name.empty())
  {
    rv = cubeb_enumerate_devices(m_context, CUBEB_DEVICE_TYPE_OUTPUT, &devices);
    devices_valid = (rv == CUBEB_OK);
    if (rv == CUBEB_OK)
    {
      for (size_t i = 0; i < devices.count; i++)
      {
        const cubeb_device_info& di = devices.device[i];
        if (di.device_id && selected_device_name == di.device_id)
        {
          INFO_LOG("Using output device '{}' ({}).", di.device_id, di.friendly_name ? di.friendly_name : di.device_id);
          selected_device = di.devid;
          break;
        }
      }

      if (!selected_device)
      {
        Host::AddOSDMessage(
          fmt::format("Requested audio output device '{}' not found, using default.", selected_device_name), 10.0f);
      }
    }
    else
    {
      WARNING_LOG("cubeb_enumerate_devices() returned {}, using default device.", GetCubebErrorString(rv));
    }
  }

  BaseInitialize();

  char stream_name[32];
  std::snprintf(stream_name, sizeof(stream_name), "%p", this);

  rv = cubeb_stream_init(m_context, &stream, stream_name, nullptr, nullptr, selected_device, &params, latency_frames,
                         &CubebAudioStream::DataCallback, StateCallback, this);

  if (devices_valid)
    cubeb_device_collection_destroy(m_context, &devices);

  if (rv != CUBEB_OK)
  {
    Error::SetStringFmt(error, "cubeb_stream_init() failed: {}", GetCubebErrorString(rv));
    DestroyContextAndStream();
    return false;
  }

  rv = cubeb_stream_start(stream);
  if (rv != CUBEB_OK)
  {
    Error::SetStringFmt(error, "cubeb_stream_start() failed: {}", GetCubebErrorString(rv));
    DestroyContextAndStream();
    return false;
  }

  return true;
}

void CubebAudioStream::StateCallback(cubeb_stream* stream, void* user_ptr, cubeb_state state)
{
  // noop
}

long CubebAudioStream::DataCallback(cubeb_stream* stm, void* user_ptr, const void* input_buffer, void* output_buffer,
                                    long nframes)
{
  static_cast<CubebAudioStream*>(user_ptr)->ReadFrames(static_cast<s16*>(output_buffer), static_cast<u32>(nframes));
  return nframes;
}

void CubebAudioStream::SetPaused(bool paused)
{
  if (paused == m_paused || !stream)
    return;

  const int rv = paused ? cubeb_stream_stop(stream) : cubeb_stream_start(stream);
  if (rv != CUBEB_OK)
  {
    ERROR_LOG("Could not {} stream: {}", paused ? "pause" : "resume", rv);
    return;
  }

  m_paused = paused;
}

std::unique_ptr<AudioStream> AudioStream::CreateCubebAudioStream(u32 sample_rate,
                                                                 const AudioStreamParameters& parameters,
                                                                 const char* driver_name, const char* device_name,
                                                                 Error* error)
{
  std::unique_ptr<CubebAudioStream> stream = std::make_unique<CubebAudioStream>(sample_rate, parameters);
  if (!stream->Initialize(driver_name, device_name, error))
    stream.reset();
  return stream;
}

std::vector<std::pair<std::string, std::string>> AudioStream::GetCubebDriverNames()
{
  std::vector<std::pair<std::string, std::string>> names;
  names.emplace_back(std::string(), TRANSLATE_STR("AudioStream", "Default"));

  const char** cubeb_names = cubeb_get_backend_names();
  for (u32 i = 0; cubeb_names[i] != nullptr; i++)
    names.emplace_back(cubeb_names[i], cubeb_names[i]);
  return names;
}

std::vector<AudioStream::DeviceInfo> AudioStream::GetCubebOutputDevices(const char* driver, u32 sample_rate)
{
  std::vector<AudioStream::DeviceInfo> ret;
  ret.emplace_back(std::string(), TRANSLATE_STR("AudioStream", "Default"), 0);

  cubeb* context;
  int rv = cubeb_init(&context, "DuckStation", (driver && *driver) ? driver : nullptr);
  if (rv != CUBEB_OK)
  {
    ERROR_LOG("cubeb_init() failed: {}", GetCubebErrorString(rv));
    return ret;
  }

  ScopedGuard context_cleanup([context]() { cubeb_destroy(context); });

  cubeb_device_collection devices;
  rv = cubeb_enumerate_devices(context, CUBEB_DEVICE_TYPE_OUTPUT, &devices);
  if (rv != CUBEB_OK)
  {
    ERROR_LOG("cubeb_enumerate_devices() failed: {}", GetCubebErrorString(rv));
    return ret;
  }

  ScopedGuard devices_cleanup([context, &devices]() { cubeb_device_collection_destroy(context, &devices); });

  // we need stream parameters to query latency
  cubeb_stream_params params = {};
  params.format = CUBEB_SAMPLE_S16LE;
  params.rate = sample_rate;
  params.channels = 2;
  params.layout = CUBEB_LAYOUT_UNDEFINED;
  params.prefs = CUBEB_STREAM_PREF_NONE;

  u32 min_latency = 0;
  cubeb_get_min_latency(context, &params, &min_latency);
  ret[0].minimum_latency_frames = min_latency;

  for (size_t i = 0; i < devices.count; i++)
  {
    const cubeb_device_info& di = devices.device[i];
    if (!di.device_id)
      continue;

    ret.emplace_back(di.device_id, di.friendly_name ? di.friendly_name : di.device_id, min_latency);
  }

  return ret;
}

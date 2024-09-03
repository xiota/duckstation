// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#include "cd_image_hasher.h"
#include "cd_image.h"
#include "host.h"

#include "common/md5_digest.h"
#include "common/string_util.h"

#include "fmt/format.h"

namespace CDImageHasher {

static bool ReadIndex(CDImage* image, u8 track, u8 index, MD5Digest* digest, ProgressCallback* progress_callback);
static bool ReadTrack(CDImage* image, u8 track, MD5Digest* digest, ProgressCallback* progress_callback);

} // namespace CDImageHasher

bool CDImageHasher::ReadIndex(CDImage* image, u8 track, u8 index, MD5Digest* digest,
                              ProgressCallback* progress_callback)
{
  const CDImage::LBA index_start = image->GetTrackIndexPosition(track, index);
  const u32 index_length = image->GetTrackIndexLength(track, index);
  const u32 update_interval = std::max<u32>(index_length / 100u, 1u);

  progress_callback->SetStatusText(
    fmt::format(TRANSLATE_FS("CDImageHasher", "Computing hash for Track {}/Index {}..."), track, index).c_str());
  progress_callback->SetProgressRange(index_length);

  if (!image->Seek(index_start))
  {
    progress_callback->FormatModalError("Failed to seek to sector {} for track {} index {}", index_start, track, index);
    return false;
  }

  std::array<u8, CDImage::RAW_SECTOR_SIZE> sector;
  for (u32 lba = 0; lba < index_length; lba++)
  {
    if ((lba % update_interval) == 0)
      progress_callback->SetProgressValue(lba);

    if (!image->ReadRawSector(sector.data(), nullptr))
    {
      progress_callback->FormatModalError("Failed to read sector {} from image", image->GetPositionOnDisc());
      return false;
    }

    digest->Update(sector);
  }

  progress_callback->SetProgressValue(index_length);
  return true;
}

bool CDImageHasher::ReadTrack(CDImage* image, u8 track, MD5Digest* digest, ProgressCallback* progress_callback)
{
  static constexpr u8 INDICES_TO_READ = 2;

  progress_callback->PushState();

  const bool dataTrack = track == 1;
  progress_callback->SetProgressRange(dataTrack ? 1 : 2);

  u8 progress = 0;
  for (u8 index = 0; index < INDICES_TO_READ; index++)
  {
    progress_callback->SetProgressValue(progress);

    // skip index 0 if data track
    if (dataTrack && index == 0)
      continue;

    progress++;
    progress_callback->PushState();
    if (!ReadIndex(image, track, index, digest, progress_callback))
    {
      progress_callback->PopState();
      progress_callback->PopState();
      return false;
    }

    progress_callback->PopState();
  }

  progress_callback->SetProgressValue(progress);
  progress_callback->PopState();
  return true;
}

std::string CDImageHasher::HashToString(const Hash& hash)
{
  return fmt::format("{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                     hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], hash[8], hash[9], hash[10],
                     hash[11], hash[12], hash[13], hash[14], hash[15]);
}

std::optional<CDImageHasher::Hash> CDImageHasher::HashFromString(std::string_view str)
{
  auto decoded = StringUtil::DecodeHex(str);
  if (decoded && decoded->size() == std::tuple_size_v<Hash>)
  {
    Hash result;
    std::copy(decoded->begin(), decoded->end(), result.begin());
    return result;
  }
  return std::nullopt;
}

bool CDImageHasher::GetImageHash(CDImage* image, Hash* out_hash,
                                 ProgressCallback* progress_callback /*= ProgressCallback::NullProgressCallback*/)
{
  MD5Digest digest;

  progress_callback->SetProgressRange(image->GetTrackCount());
  progress_callback->SetProgressValue(0);
  progress_callback->PushState();

  for (u32 i = 1; i <= image->GetTrackCount(); i++)
  {
    progress_callback->SetProgressValue(i - 1);
    if (!ReadTrack(image, static_cast<u8>(i), &digest, progress_callback))
    {
      progress_callback->PopState();
      return false;
    }
  }

  progress_callback->SetProgressValue(image->GetTrackCount());
  digest.Final(*out_hash);
  return true;
}

bool CDImageHasher::GetTrackHash(CDImage* image, u8 track, Hash* out_hash,
                                 ProgressCallback* progress_callback /*= ProgressCallback::NullProgressCallback*/)
{
  MD5Digest digest;
  if (!ReadTrack(image, track, &digest, progress_callback))
    return false;

  digest.Final(*out_hash);
  return true;
}

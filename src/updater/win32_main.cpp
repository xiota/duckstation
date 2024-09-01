// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#include "updater.h"
#include "win32_progress_callback.h"

#include "common/file_system.h"
#include "common/log.h"
#include "common/path.h"
#include "common/scoped_guard.h"
#include "common/string_util.h"
#include "common/windows_headers.h"

#include <combaseapi.h>
#include <shellapi.h>

static void WaitForProcessToExit(int process_id)
{
  HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, process_id);
  if (!hProcess)
    return;

  WaitForSingleObject(hProcess, INFINITE);
  CloseHandle(hProcess);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
  Win32ProgressCallback progress;

  const bool com_initialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
  const ScopedGuard com_guard = [com_initialized]() {
    if (com_initialized)
      CoUninitialize();
  };

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
  if (!argv || argc <= 0)
  {
    progress.ModalError("Failed to parse command line.");
    return 1;
  }
  if (argc != 4)
  {
    progress.ModalError("Expected 4 arguments: parent process id, output directory, update zip, program to "
                        "launch.\n\nThis program is not intended to be run manually, please use the Qt frontend and "
                        "click Help->Check for Updates.");
    LocalFree(argv);
    return 1;
  }

  const int parent_process_id = StringUtil::FromChars<int>(StringUtil::WideStringToUTF8String(argv[0])).value_or(0);
  std::string destination_directory = Path::ToNativePath(StringUtil::WideStringToUTF8String(argv[1]));
  std::string staging_directory = Path::Combine(destination_directory, "UPDATE_STAGING");
  std::string zip_path = Path::ToNativePath(StringUtil::WideStringToUTF8String(argv[2]));
  std::wstring program_to_launch(argv[3]);
  LocalFree(argv);

  if (parent_process_id <= 0 || destination_directory.empty() || zip_path.empty() || program_to_launch.empty())
  {
    progress.ModalError("One or more parameters is empty.");
    return 1;
  }

  Log::SetFileOutputParams(true, Path::Combine(destination_directory, "updater.log").c_str());

  progress.FormatStatusText("Waiting for parent process {} to exit...", parent_process_id);
  WaitForProcessToExit(parent_process_id);

  Updater updater(&progress);
  if (!updater.Initialize(std::move(staging_directory), std::move(destination_directory)))
  {
    progress.ModalError("Failed to initialize updater.");
    return 1;
  }

  if (!updater.OpenUpdateZip(zip_path.c_str()))
  {
    progress.FormatModalError("Could not open update zip '{}'. Update not installed.", zip_path);
    return 1;
  }

  if (!updater.PrepareStagingDirectory())
  {
    progress.ModalError("Failed to prepare staging directory. Update not installed.");
    return 1;
  }

  if (!updater.StageUpdate())
  {
    progress.ModalError("Failed to stage update. Update not installed.");
    return 1;
  }

  if (!updater.CommitUpdate())
  {
    progress.ModalError(
      "Failed to commit update. Your installation may be corrupted, please re-download a fresh version from GitHub.");
    return 1;
  }

  updater.CleanupStagingDirectory();
  updater.RemoveUpdateZip();

  progress.FormatInformation("Launching '{}'...", StringUtil::WideStringToUTF8String(program_to_launch));
  ShellExecuteW(nullptr, L"open", program_to_launch.c_str(), L"-updatecleanup", nullptr, SW_SHOWNORMAL);
  return 0;
}

// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#include "common/file_system.h"

#include <gtest/gtest.h>

#ifdef _WIN32

TEST(FileSystem, GetWin32Path)
{
  ASSERT_EQ(FileSystem::GetWin32Path("test.txt"), L"test.txt");
  ASSERT_EQ(FileSystem::GetWin32Path("D:\\test.txt"), L"\\\\?\\D:\\test.txt");
  ASSERT_EQ(FileSystem::GetWin32Path("C:\\foo"), L"\\\\?\\C:\\foo");
  ASSERT_EQ(FileSystem::GetWin32Path("C:\\foo\\bar\\..\\baz"), L"\\\\?\\C:\\foo\\baz");
  ASSERT_EQ(FileSystem::GetWin32Path("\\\\foo\\bar\\baz"), L"\\\\?\\UNC\\foo\\bar\\baz");
  ASSERT_EQ(FileSystem::GetWin32Path("\\\\foo\\bar\\baz\\sub\\.."), L"\\\\?\\UNC\\foo\\bar\\baz");
  ASSERT_EQ(FileSystem::GetWin32Path("ŻąłóРстуぬねのはen🍪⟑η∏☉ⴤℹ︎∩₲ ₱⟑♰⫳🐱"), L"ŻąłóРстуぬねのはen🍪⟑η∏☉ⴤℹ︎∩₲ ₱⟑♰⫳🐱");
  ASSERT_EQ(FileSystem::GetWin32Path("C:\\ŻąłóРстуぬねのはen🍪⟑η∏☉ⴤℹ︎∩₲ ₱⟑♰⫳🐱"),
            L"\\\\?\\C:\\ŻąłóРстуぬねのはen🍪⟑η∏☉ⴤℹ︎∩₲ ₱⟑♰⫳🐱");
  ASSERT_EQ(FileSystem::GetWin32Path("\\\\foo\\bar\\ŻąłóРстуぬねのはen🍪⟑η∏☉ⴤℹ︎∩₲ ₱⟑♰⫳🐱"),
            L"\\\\?\\UNC\\foo\\bar\\ŻąłóРстуぬねのはen🍪⟑η∏☉ⴤℹ︎∩₲ ₱⟑♰⫳🐱");
  ASSERT_EQ(FileSystem::GetWin32Path("C:\\ŻąłóРстуぬね\\のはen🍪\\⟑η∏☉ⴤ\\..\\ℹ︎∩₲ ₱⟑♰⫳🐱"),
            L"\\\\?\\C:\\ŻąłóРстуぬね\\のはen🍪\\ℹ︎∩₲ ₱⟑♰⫳🐱");
  ASSERT_EQ(FileSystem::GetWin32Path("\\\\foo\\bar\\ŻąłóРстуぬねのはen🍪\\⟑η∏☉ⴤ\\..\\ℹ︎∩₲ ₱⟑♰⫳🐱"),
            L"\\\\?\\UNC\\foo\\bar\\ŻąłóРстуぬねのはen🍪\\ℹ︎∩₲ ₱⟑♰⫳🐱");
}

#endif

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/picker/picker_copy_media.h"

#include "ash/picker/picker_rich_media.h"
#include "ash/picker/picker_test_util.h"
#include "ash/public/cpp/system/toast_manager.h"
#include "ash/test/ash_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/gfx/geometry/size.h"

namespace ash {
namespace {

class PickerCopyMediaTest : public AshTestBase {};

TEST_F(PickerCopyMediaTest, CopiesText) {
  CopyMediaToClipboard(PickerTextMedia(u"hello"));

  EXPECT_EQ(ReadTextFromClipboard(ui::Clipboard::GetForCurrentThread()),
            u"hello");
}

TEST_F(PickerCopyMediaTest, CopiesImageWithKnownDimensionsAsHtml) {
  CopyMediaToClipboard(
      PickerImageMedia(GURL("https://foo.com"), gfx::Size(30, 20)));

  EXPECT_EQ(
      ReadHtmlFromClipboard(ui::Clipboard::GetForCurrentThread()),
      uR"html(<img src="https://foo.com/" referrerpolicy="no-referrer" width="30" height="20"/>)html");
}

TEST_F(PickerCopyMediaTest, CopiesImageWithUnknownDimensionsAsHtml) {
  CopyMediaToClipboard(PickerImageMedia(GURL("https://foo.com")));

  EXPECT_EQ(
      ReadHtmlFromClipboard(ui::Clipboard::GetForCurrentThread()),
      uR"html(<img src="https://foo.com/" referrerpolicy="no-referrer"/>)html");
}

TEST_F(PickerCopyMediaTest, CopiesImagesWithBothAltTextAndDimensionsAsHtml) {
  CopyMediaToClipboard(PickerImageMedia(GURL("https://foo.com"),
                                        gfx::Size(30, 20),
                                        /*content_description=*/u"img"));

  EXPECT_EQ(
      ReadHtmlFromClipboard(ui::Clipboard::GetForCurrentThread()),
      uR"html(<img src="https://foo.com/" referrerpolicy="no-referrer" alt="img" width="30" height="20"/>)html");
}

TEST_F(PickerCopyMediaTest, EscapesAltTextForImages) {
  CopyMediaToClipboard(PickerImageMedia(GURL("https://foo.com"),
                                        /*dimensions=*/std::nullopt,
                                        /*content_description=*/u"\"img\""));

  EXPECT_EQ(
      ReadHtmlFromClipboard(ui::Clipboard::GetForCurrentThread()),
      uR"html(<img src="https://foo.com/" referrerpolicy="no-referrer" alt="&quot;img&quot;"/>)html");
}

TEST_F(PickerCopyMediaTest, CopiesLinks) {
  CopyMediaToClipboard(PickerLinkMedia(GURL("https://foo.com")));

  EXPECT_EQ(ReadTextFromClipboard(ui::Clipboard::GetForCurrentThread()),
            u"https://foo.com/");
}

TEST_F(PickerCopyMediaTest, CopiesFiles) {
  CopyMediaToClipboard(PickerLocalFileMedia(base::FilePath("/foo.txt")));

  EXPECT_EQ(ReadFilenameFromClipboard(ui::Clipboard::GetForCurrentThread()),
            base::FilePath("/foo.txt"));
}

class PickerCopyMediaToastTest
    : public AshTestBase,
      public testing::WithParamInterface<PickerRichMedia> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    PickerCopyMediaToastTest,
    ::testing::Values(PickerTextMedia(u"hello"),
                      PickerImageMedia(GURL("https://foo.com"),
                                       gfx::Size(30, 20)),
                      PickerLinkMedia(GURL("https://foo.com")),
                      PickerLocalFileMedia(base::FilePath("/foo.txt"))));

TEST_P(PickerCopyMediaToastTest, ShowsToastAfterCopyingLink) {
  CopyMediaToClipboard(GetParam());

  EXPECT_TRUE(
      ash::ToastManager::Get()->IsToastShown("picker_copy_to_clipboard"));
}

}  // namespace
}  // namespace ash

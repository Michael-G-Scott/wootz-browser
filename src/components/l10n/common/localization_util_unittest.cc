/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/l10n/common/localization_util.h"

#include "components/grit/COMPONENTS_strings.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- wootz_unit_tests --filter=LocalizationUtilTest*

namespace wootz_l10n {

namespace {
constexpr int kInvalidResourceId = -1;
}  // namespace

TEST(LocalizationUtilTest, GetLocalizedResourceUTF16String) {
  const std::u16string localized_resource =
      GetLocalizedResourceUTF16String(IDS_WOOTZ_NEW_TAB_STATS);

  EXPECT_EQ(u"Wootz Stats", localized_resource);
}

TEST(LocalizationUtilTest,
     GetLocalizedResourceUTF16StringWithInvalidResourceId) {
  const std::u16string localized_resource =
      GetLocalizedResourceUTF16String(kInvalidResourceId);

  EXPECT_TRUE(localized_resource.empty());
}

}  // namespace wootz_l10n

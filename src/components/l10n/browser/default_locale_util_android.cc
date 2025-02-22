/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/l10n/browser/default_locale_util.h"

#include <optional>

#include "base/android/locale_utils.h"

namespace wootz_l10n {

std::optional<std::string> MaybeGetDefaultLocaleString() {
  return base::android::GetDefaultLocaleString();
}

}  // namespace wootz_l10n

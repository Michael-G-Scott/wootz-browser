// Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/wootz_wallet/browser/zcash/rust/authorized_orchard_bundle_impl.h"

namespace wootz_wallet::orchard {

AuthorizedOrchardBundleImpl::AuthorizedOrchardBundleImpl(
    ::rust::Box<orchard::OrchardAuthorizedBundle> orchard_authorized_bundle)
    : orchard_authorized_bundle_(std::move(orchard_authorized_bundle)) {}

AuthorizedOrchardBundleImpl::~AuthorizedOrchardBundleImpl() = default;

std::vector<uint8_t> AuthorizedOrchardBundleImpl::GetOrchardRawTxPart() {
  auto data = orchard_authorized_bundle_->raw_tx();
  return std::vector<uint8_t>(data.begin(), data.end());
}

}  // namespace wootz_wallet::orchard

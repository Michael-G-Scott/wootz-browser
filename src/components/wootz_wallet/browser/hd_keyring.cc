/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/wootz_wallet/browser/hd_keyring.h"

#include "base/notreached.h"
#include "components/wootz_wallet/common/wootz_wallet.mojom.h"

namespace wootz_wallet {

HDKeyring::HDKeyring() = default;
HDKeyring::~HDKeyring() = default;

// static
std::string HDKeyring::GetRootPath(mojom::KeyringId keyring_id) {
  if (keyring_id == mojom::KeyringId::kDefault) {
    return "m/44'/60'/0'/0";
  } else if (keyring_id == mojom::KeyringId::kSolana) {
    return "m/44'/501'";
  } else if (keyring_id == mojom::KeyringId::kFilecoin) {
    return "m/44'/461'/0'/0";
  } else if (keyring_id == mojom::KeyringId::kFilecoinTestnet) {
    return "m/44'/1'/0'/0";
  } else if (keyring_id == mojom::KeyringId::kBitcoin84) {
    return "m/84'/0'";
  } else if (keyring_id == mojom::KeyringId::kBitcoin84Testnet) {
    return "m/84'/1'";
  } else if (keyring_id == mojom::KeyringId::kZCashMainnet) {
    return "m/44'/133'";
  } else if (keyring_id == mojom::KeyringId::kZCashTestnet) {
    return "m/44'/1'";
  }

  NOTREACHED_IN_MIGRATION() << keyring_id;
  return "";
}

}  // namespace wootz_wallet

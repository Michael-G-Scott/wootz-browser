/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_HD_KEYRING_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_HD_KEYRING_H_

#include <optional>
#include <string>
#include <vector>

#include "components/wootz_wallet/common/wootz_wallet.mojom.h"

namespace wootz_wallet {

struct AddedAccountInfo {
  uint32_t account_index;
  std::string address;
};

class HDKeyring {
 public:
  HDKeyring();
  virtual ~HDKeyring();
  HDKeyring(const HDKeyring&) = delete;
  HDKeyring& operator=(const HDKeyring&) = delete;

  static std::string GetRootPath(mojom::KeyringId keyring_id);

  virtual std::optional<AddedAccountInfo> AddNewHDAccount() = 0;
  virtual void RemoveLastHDAccount() = 0;

  // TODO(apaymyshev): imported accounts should be handled by separate keyring
  // class.
  // address will be returned
  virtual std::string ImportAccount(
      const std::vector<uint8_t>& private_key) = 0;
  virtual bool RemoveImportedAccount(const std::string& address) = 0;

  virtual std::string GetDiscoveryAddress(size_t index) const = 0;
  // Find private key by address and encode for export (it would be hex or
  // base58 depends on underlying hd key)
  virtual std::string EncodePrivateKeyForExport(const std::string& address) = 0;

  virtual std::vector<std::string> GetHDAccountsForTesting() const = 0;
  virtual std::vector<std::string> GetImportedAccountsForTesting() const = 0;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_HD_KEYRING_H_

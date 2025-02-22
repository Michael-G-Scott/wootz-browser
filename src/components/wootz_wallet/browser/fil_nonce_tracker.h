/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_FIL_NONCE_TRACKER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_FIL_NONCE_TRACKER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/wootz_wallet/browser/nonce_tracker.h"
#include "components/wootz_wallet/common/wootz_wallet.mojom.h"
#include "components/wootz_wallet/common/wootz_wallet_types.h"

namespace wootz_wallet {

class JsonRpcService;
class TxStateManager;

class FilNonceTracker : public NonceTracker {
 public:
  FilNonceTracker(TxStateManager* tx_state_manager,
                  JsonRpcService* json_rpc_service);
  ~FilNonceTracker() override;
  FilNonceTracker(const FilNonceTracker&) = delete;
  FilNonceTracker operator=(const FilNonceTracker&) = delete;

  // NonceTracker
  void GetNextNonce(const std::string& chain_id,
                    const mojom::AccountIdPtr& from,
                    GetNextNonceCallback callback) override;
  uint256_t GetHighestLocallyConfirmed(
      const std::vector<std::unique_ptr<TxMeta>>& metas) override;
  uint256_t GetHighestContinuousFrom(
      const std::vector<std::unique_ptr<TxMeta>>& metas,
      uint256_t start) override;

  void OnGetNetworkNonce(const std::string& chain_id,
                         const mojom::AccountIdPtr& from,
                         GetNextNonceCallback callback,
                         uint256_t network_nonce,
                         mojom::FilecoinProviderError error,
                         const std::string& error_message);

 private:
  base::WeakPtrFactory<FilNonceTracker> weak_factory_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_FIL_NONCE_TRACKER_H_

/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_NONCE_TRACKER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_NONCE_TRACKER_H_

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
class TxMeta;

class EthNonceTracker : public NonceTracker {
 public:
  EthNonceTracker(TxStateManager* tx_state_manager,
                  JsonRpcService* json_rpc_service);
  ~EthNonceTracker() override;
  EthNonceTracker(const EthNonceTracker&) = delete;
  EthNonceTracker operator=(const EthNonceTracker&) = delete;

  // NonceTracker
  void GetNextNonce(const std::string& chain_id,
                    const mojom::AccountIdPtr& from,
                    GetNextNonceCallback callback) override;
  uint256_t GetHighestLocallyConfirmed(
      const std::vector<std::unique_ptr<TxMeta>>& metas) override;
  uint256_t GetHighestContinuousFrom(
      const std::vector<std::unique_ptr<TxMeta>>& metas,
      uint256_t start) override;

 private:
  void OnGetNetworkNonce(const std::string& chain_id,
                         const mojom::AccountIdPtr& from,
                         GetNextNonceCallback callback,
                         uint256_t network_nonce,
                         mojom::ProviderError error,
                         const std::string& error_message);

  base::WeakPtrFactory<EthNonceTracker> weak_factory_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_NONCE_TRACKER_H_

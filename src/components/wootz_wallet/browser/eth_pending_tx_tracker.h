/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_PENDING_TX_TRACKER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_PENDING_TX_TRACKER_H_

#include <map>
#include <optional>
#include <set>
#include <string>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/wootz_wallet/browser/eth_tx_state_manager.h"
#include "components/wootz_wallet/common/wootz_wallet_types.h"

namespace wootz_wallet {

class TxMeta;
class EthTxMeta;
class EthNonceTracker;
class JsonRpcService;

class EthPendingTxTracker {
 public:
  EthPendingTxTracker(EthTxStateManager* tx_state_manager,
                      JsonRpcService* json_rpc_service,
                      EthNonceTracker* nonce_tracker);
  ~EthPendingTxTracker();
  EthPendingTxTracker(const EthPendingTxTracker&) = delete;
  EthPendingTxTracker operator=(const EthPendingTxTracker&) = delete;

  bool UpdatePendingTransactions(const std::optional<std::string>& chain_id,
                                 std::set<std::string>* pending_chain_ids);
  void Reset();

 private:
  FRIEND_TEST_ALL_PREFIXES(EthPendingTxTrackerUnitTest, IsNonceTaken);
  FRIEND_TEST_ALL_PREFIXES(EthPendingTxTrackerUnitTest, ShouldTxDropped);
  FRIEND_TEST_ALL_PREFIXES(EthPendingTxTrackerUnitTest, DropTransaction);

  void OnGetTxReceipt(const std::string& meta_id,
                      TransactionReceipt receipt,
                      mojom::ProviderError error,
                      const std::string& error_message);
  void OnGetNetworkNonce(const std::string& chain_id,
                         const std::string& address,
                         uint256_t result,
                         mojom::ProviderError error,
                         const std::string& error_message);
  void OnSendRawTransaction(const std::string& tx_hash,
                            mojom::ProviderError error,
                            const std::string& error_message);

  bool IsNonceTaken(const EthTxMeta&);
  bool ShouldTxDropped(const EthTxMeta&);

  void DropTransaction(TxMeta*);

  // (address, (chain_id, nonce))
  base::flat_map<std::string, std::map<std::string, uint256_t>>
      network_nonce_map_;
  // (txHash, count)
  base::flat_map<std::string, uint8_t> dropped_blocks_counter_;

  raw_ptr<EthTxStateManager> tx_state_manager_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<EthNonceTracker> nonce_tracker_ = nullptr;

  base::WeakPtrFactory<EthPendingTxTracker> weak_factory_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_PENDING_TX_TRACKER_H_

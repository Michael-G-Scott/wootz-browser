/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_TX_MANAGER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/scoped_observation.h"
#include "components/wootz_wallet/browser/bitcoin/bitcoin_block_tracker.h"
#include "components/wootz_wallet/browser/bitcoin/bitcoin_tx_meta.h"
#include "components/wootz_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "components/wootz_wallet/browser/tx_manager.h"

class PrefService;

namespace wootz_wallet {

class AccountResolverDelegate;
class TxService;
class KeyringService;
class BitcoinWalletService;
class BitcoinTxStateManager;

class BitcoinTxManager : public TxManager,
                         public BitcoinBlockTracker::Observer {
 public:
  BitcoinTxManager(TxService* tx_service,
                   BitcoinWalletService* bitcoin_wallet_service,
                   KeyringService* keyring_service,
                   PrefService* prefs,
                   TxStorageDelegate* delegate,
                   AccountResolverDelegate* account_resolver_delegate);
  ~BitcoinTxManager() override;
  BitcoinTxManager(const BitcoinTxManager&) = delete;
  BitcoinTxManager& operator=(const BitcoinTxManager&) = delete;
  std::unique_ptr<BitcoinTxMeta> GetTxForTesting(const std::string& tx_meta_id);

 private:
  friend class BitcoinTxManagerUnitTest;
  FRIEND_TEST_ALL_PREFIXES(BitcoinTxManagerUnitTest, SubmitTransaction);
  FRIEND_TEST_ALL_PREFIXES(BitcoinTxManagerUnitTest, SubmitTransactionError);
  FRIEND_TEST_ALL_PREFIXES(BitcoinTxManagerUnitTest, SubmitTransactionError);

  BitcoinTxStateManager* GetBitcoinTxStateManager();
  BitcoinBlockTracker* GetBitcoinBlockTracker();

  // BitcoinBlockTracker::Observer
  void OnLatestHeightUpdated(const std::string& chain_id,
                             uint32_t latest_height) override;

  // TxManager
  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::TxDataUnionPtr tx_data_union,
                                const mojom::AccountIdPtr& from,
                                const std::optional<url::Origin>& origin,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;
  mojom::CoinType GetCoinType() const override;
  void UpdatePendingTransactions(
      const std::optional<std::string>& chain_id) override;

  void ContinueAddUnapprovedTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
      AddUnapprovedTransactionCallback callback,
      base::expected<BitcoinTransaction, std::string> bitcoin_transaction);

  void ContinueApproveTransaction(const std::string& tx_meta_id,
                                  ApproveTransactionCallback callback,
                                  std::string tx_cid,
                                  BitcoinTransaction transaction,
                                  std::string error);

  void OnGetTransactionStatus(const std::string& tx_meta_id,
                              base::expected<bool, std::string> confirm_status);

  raw_ptr<BitcoinWalletService> bitcoin_wallet_service_ = nullptr;
  base::ScopedObservation<BitcoinBlockTracker, BitcoinBlockTracker::Observer>
      block_tracker_observation_{this};
  base::WeakPtrFactory<BitcoinTxManager> weak_factory_{this};
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_TX_MANAGER_H_

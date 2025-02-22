/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_TX_MANAGER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "components/wootz_wallet/browser/keyring_service_observer_base.h"
#include "components/wootz_wallet/browser/tx_state_manager.h"
#include "components/wootz_wallet/common/wootz_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

class PrefService;

namespace wootz_wallet {

class BlockTracker;
class KeyringService;
class TxService;

class TxManager : public TxStateManager::Observer,
                  public KeyringServiceObserverBase {
 public:
  TxManager(std::unique_ptr<TxStateManager> tx_state_manager,
            std::unique_ptr<BlockTracker> block_tracker,
            TxService* tx_service,
            KeyringService* keyring_service,
            PrefService* prefs);
  ~TxManager() override;

  using AddUnapprovedTransactionCallback =
      mojom::TxService::AddUnapprovedTransactionCallback;
  using ApproveTransactionCallback =
      mojom::TxService::ApproveTransactionCallback;
  using RejectTransactionCallback = mojom::TxService::RejectTransactionCallback;
  using GetTransactionInfoCallback =
      mojom::TxService::GetTransactionInfoCallback;
  using SpeedupOrCancelTransactionCallback =
      mojom::TxService::SpeedupOrCancelTransactionCallback;
  using RetryTransactionCallback = mojom::TxService::RetryTransactionCallback;
  using GetTransactionMessageToSignCallback =
      mojom::TxService::GetTransactionMessageToSignCallback;

  virtual void AddUnapprovedTransaction(
      const std::string& chain_id,
      mojom::TxDataUnionPtr tx_data_union,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
      AddUnapprovedTransactionCallback) = 0;
  virtual void ApproveTransaction(const std::string& tx_meta_id,
                                  ApproveTransactionCallback) = 0;
  virtual void RejectTransaction(const std::string& tx_meta_id,
                                 RejectTransactionCallback);
  virtual void GetTransactionInfo(const std::string& tx_meta_id,
                                  GetTransactionInfoCallback);
  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo(
      const std::optional<std::string>& chain_id,
      const std::optional<mojom::AccountIdPtr>& from);

  virtual void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) = 0;
  virtual void RetryTransaction(const std::string& tx_meta_id,
                                RetryTransactionCallback callback) = 0;

  virtual void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) = 0;

  virtual void Reset();

 protected:
  void CheckIfBlockTrackerShouldRun(
      const std::set<std::string>& new_pending_chain_ids);
  virtual void UpdatePendingTransactions(
      const std::optional<std::string>& chain_id) = 0;

  std::unique_ptr<TxStateManager> tx_state_manager_;
  std::unique_ptr<BlockTracker> block_tracker_;
  raw_ptr<TxService> tx_service_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  std::set<std::string> pending_chain_ids_;

 private:
  virtual mojom::CoinType GetCoinType() const = 0;

  // TxStateManager::Observer
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override;

  // mojom::KeyringServiceObserverBase:
  void WalletReset() override;
  void Locked() override;
  void Unlocked() override;

  mojo::Receiver<wootz_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_TX_MANAGER_H_

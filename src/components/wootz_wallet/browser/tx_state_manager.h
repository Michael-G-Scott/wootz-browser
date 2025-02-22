/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_TX_STATE_MANAGER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_TX_STATE_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "components/wootz_wallet/common/wootz_wallet.mojom.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace wootz_wallet {

class AccountResolverDelegate;
class TxMeta;
class TxStorageDelegate;

class TxStateManager {
 public:
  TxStateManager(PrefService* prefs,
                 TxStorageDelegate* delegate,
                 AccountResolverDelegate* account_resolver_delegate);
  virtual ~TxStateManager();
  TxStateManager(const TxStateManager&) = delete;

  bool AddOrUpdateTx(const TxMeta& meta);
  std::unique_ptr<TxMeta> GetTx(const std::string& meta_id);
  bool DeleteTx(const std::string& meta_id);

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      const std::optional<std::string>& chain_id,
      const std::optional<mojom::TransactionStatus>& status,
      const mojom::AccountIdPtr& from);

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      const std::optional<std::string>& chain_id,
      const std::optional<mojom::TransactionStatus>& status,
      const std::optional<mojom::AccountIdPtr>& from);

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
    }
    virtual void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void SetNoRetireForTesting(bool no_retire);

 protected:
  // For derived classes to call to fill TxMeta properties.
  bool ValueToBaseTxMeta(const base::Value::Dict& value, TxMeta* tx_meta);

  raw_ptr<PrefService> prefs_ = nullptr;

 private:
  FRIEND_TEST_ALL_PREFIXES(TxStateManagerUnitTest, ConvertFromAddress);
  FRIEND_TEST_ALL_PREFIXES(TxStateManagerUnitTest, TxOperations);
  FRIEND_TEST_ALL_PREFIXES(EthTxManagerUnitTest, Reset);

  void RetireTxByStatus(const std::string& chain_id,
                        mojom::TransactionStatus status,
                        size_t max_num);

  virtual mojom::CoinType GetCoinType() const = 0;

  // Each derived class should implement its own ValueToTxMeta to create a
  // specific type of tx meta (ex: EthTxMeta) from a value. TxMeta
  // properties can be filled via the protected ValueToTxMeta function above.
  virtual std::unique_ptr<TxMeta> ValueToTxMeta(
      const base::Value::Dict& value) = 0;

  bool no_retire_for_testing_ = false;
  raw_ptr<TxStorageDelegate> delegate_ = nullptr;
  raw_ptr<AccountResolverDelegate> account_resolver_delegate_ = nullptr;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<TxStateManager> weak_factory_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_TX_STATE_MANAGER_H_

/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/wootz_wallet/browser/bitcoin/bitcoin_tx_state_manager.h"

#include <optional>
#include <utility>

#include "base/values.h"
#include "components/wootz_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "components/wootz_wallet/browser/bitcoin/bitcoin_tx_meta.h"
#include "components/wootz_wallet/browser/wootz_wallet_utils.h"
#include "components/wootz_wallet/browser/tx_meta.h"

namespace wootz_wallet {

BitcoinTxStateManager::BitcoinTxStateManager(
    PrefService* prefs,
    TxStorageDelegate* delegate,
    AccountResolverDelegate* account_resolver_delegate)
    : TxStateManager(prefs, delegate, account_resolver_delegate) {}

BitcoinTxStateManager::~BitcoinTxStateManager() = default;

std::unique_ptr<BitcoinTxMeta> BitcoinTxStateManager::GetBitcoinTx(
    const std::string& id) {
  return std::unique_ptr<BitcoinTxMeta>{
      static_cast<BitcoinTxMeta*>(TxStateManager::GetTx(id).release())};
}

std::unique_ptr<BitcoinTxMeta> BitcoinTxStateManager::ValueToBitcoinTxMeta(
    const base::Value::Dict& value) {
  return std::unique_ptr<BitcoinTxMeta>{
      static_cast<BitcoinTxMeta*>(ValueToTxMeta(value).release())};
}

mojom::CoinType BitcoinTxStateManager::GetCoinType() const {
  return mojom::CoinType::BTC;
}

std::unique_ptr<TxMeta> BitcoinTxStateManager::ValueToTxMeta(
    const base::Value::Dict& value) {
  std::unique_ptr<BitcoinTxMeta> meta = std::make_unique<BitcoinTxMeta>();

  if (!ValueToBaseTxMeta(value, meta.get())) {
    return nullptr;
  }
  const base::Value::Dict* tx = value.FindDict("tx");
  if (!tx) {
    return nullptr;
  }
  std::optional<BitcoinTransaction> tx_from_value =
      BitcoinTransaction::FromValue(*tx);
  if (!tx_from_value) {
    return nullptr;
  }
  meta->set_tx(std::make_unique<BitcoinTransaction>(std::move(*tx_from_value)));
  return meta;
}

}  // namespace wootz_wallet

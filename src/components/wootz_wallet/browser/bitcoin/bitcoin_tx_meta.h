/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_TX_META_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_TX_META_H_

#include <memory>
#include <utility>

#include "components/wootz_wallet/browser/tx_meta.h"

namespace base {
class Value;
}  // namespace base

namespace wootz_wallet {

class BitcoinTransaction;

class BitcoinTxMeta : public TxMeta {
 public:
  BitcoinTxMeta();
  BitcoinTxMeta(const mojom::AccountIdPtr& from,
                std::unique_ptr<BitcoinTransaction> tx);
  ~BitcoinTxMeta() override;
  BitcoinTxMeta(const BitcoinTxMeta&) = delete;
  BitcoinTxMeta operator=(const BitcoinTxMeta&) = delete;
  bool operator==(const BitcoinTxMeta& other) const;

  // TxMeta
  base::Value::Dict ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;
  mojom::CoinType GetCoinType() const override;

  BitcoinTransaction* tx() const { return tx_.get(); }
  void set_tx(std::unique_ptr<BitcoinTransaction> tx) { tx_ = std::move(tx); }

 private:
  std::unique_ptr<BitcoinTransaction> tx_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_TX_META_H_

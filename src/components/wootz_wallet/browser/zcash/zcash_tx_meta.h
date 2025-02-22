/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_ZCASH_ZCASH_TX_META_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_ZCASH_ZCASH_TX_META_H_

#include <memory>
#include <utility>

#include "components/wootz_wallet/browser/tx_meta.h"

namespace base {
class Value;
}  // namespace base

namespace wootz_wallet {

class ZCashTransaction;

class ZCashTxMeta : public TxMeta {
 public:
  ZCashTxMeta();
  ZCashTxMeta(const mojom::AccountIdPtr& from,
              std::unique_ptr<ZCashTransaction> tx);
  ~ZCashTxMeta() override;
  ZCashTxMeta(const ZCashTxMeta&) = delete;
  ZCashTxMeta operator=(const ZCashTxMeta&) = delete;
  bool operator==(const ZCashTxMeta& other) const;

  // TxMeta
  base::Value::Dict ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;
  mojom::CoinType GetCoinType() const override;

  ZCashTransaction* tx() const { return tx_.get(); }
  void set_tx(std::unique_ptr<ZCashTransaction> tx) { tx_ = std::move(tx); }

 private:
  std::unique_ptr<ZCashTransaction> tx_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_ZCASH_ZCASH_TX_META_H_

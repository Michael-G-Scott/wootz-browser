/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_SOLANA_TX_META_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_SOLANA_TX_META_H_

#include <memory>
#include <utility>

#include "components/wootz_wallet/browser/solana_transaction.h"
#include "components/wootz_wallet/browser/tx_meta.h"
#include "components/wootz_wallet/common/wootz_wallet.mojom.h"
#include "components/wootz_wallet/common/wootz_wallet_types.h"

namespace base {
class Value;
}  // namespace base

namespace wootz_wallet {

class SolanaTransaction;

class SolanaTxMeta : public TxMeta {
 public:
  SolanaTxMeta();
  SolanaTxMeta(const mojom::AccountIdPtr& from,
               std::unique_ptr<SolanaTransaction> tx);
  SolanaTxMeta(const SolanaTxMeta&) = delete;
  ~SolanaTxMeta() override;
  bool operator==(const SolanaTxMeta&) const;

  // TxMeta
  base::Value::Dict ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;
  mojom::CoinType GetCoinType() const override;

  SolanaTransaction* tx() const { return tx_.get(); }
  SolanaSignatureStatus signature_status() const { return signature_status_; }

  void set_tx(std::unique_ptr<SolanaTransaction> tx) { tx_ = std::move(tx); }
  void set_signature_status(const SolanaSignatureStatus& signature_status) {
    signature_status_ = signature_status;
  }

  bool IsRetriable() const;

 private:
  std::unique_ptr<SolanaTransaction> tx_;
  // Status returned by getSignatureStatuses JSON-RPC call.
  SolanaSignatureStatus signature_status_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_SOLANA_TX_META_H_

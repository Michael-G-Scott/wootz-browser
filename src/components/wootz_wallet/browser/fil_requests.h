/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_FIL_REQUESTS_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_FIL_REQUESTS_H_

#include <optional>
#include <string>

#include "base/values.h"

namespace wootz_wallet {

namespace fil {

// Returns the balance of the account of given address.
std::string getBalance(const std::string& address);
// Returns the transactions count of given address.
std::string getTransactionCount(const std::string& address);
// Returns Gas estimation values.
// https://github.com/filecoin-project/lotus/blob/master/chain/types/message.go
std::string getEstimateGas(const std::string& from_address,
                           const std::string& to_address,
                           const std::string& gas_premium,
                           const std::string& gas_fee_cap,
                           int64_t gas_limit,
                           uint64_t nonce,
                           const std::string& max_fee,
                           const std::string& value);
// Returns Filecoin.ChainHead request.
std::string getChainHead();
// Returns Filecoin.StateSearchMsgLimited request.
std::string getStateSearchMsgLimited(const std::string& cid, uint64_t period);
// Returns Filecoin.MPoolPush request.
std::optional<std::string> getSendTransaction(const std::string& signed_tx);

}  // namespace fil

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_FIL_REQUESTS_H_

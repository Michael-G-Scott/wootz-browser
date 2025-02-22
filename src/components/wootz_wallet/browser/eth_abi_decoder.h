/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_ABI_DECODER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_ABI_DECODER_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "base/values.h"
#include "components/wootz_wallet/common/eth_abi_utils.h"

namespace wootz_wallet {

std::optional<std::vector<std::string>> UniswapEncodedPathDecode(
    const std::string& encoded_path);

std::optional<base::Value::List> ABIDecode(const eth_abi::Type& type,
                                           const std::vector<uint8_t>& data);

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_ETH_ABI_DECODER_H_

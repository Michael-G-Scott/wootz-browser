/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_SERIALIZER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_SERIALIZER_H_

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "components/wootz_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "components/wootz_wallet/common/hash_utils.h"

namespace wootz_wallet {

// TODO(apaymyshev): test with reference test vectors.
class BitcoinSerializer {
 public:
  static std::vector<uint8_t> AddressToScriptPubkey(const std::string& address,
                                                    bool testnet);

  static std::optional<SHA256HashArray> SerializeInputForSign(
      const BitcoinTransaction& tx,
      size_t input_index);

  static std::vector<uint8_t> SerializeWitness(
      const std::vector<uint8_t>& signature,
      const std::vector<uint8_t>& pubkey);

  static std::vector<uint8_t> SerializeSignedTransaction(
      const BitcoinTransaction& tx);

  static uint32_t CalcOutputVBytesInTransaction(
      const BitcoinTransaction::TxOutput& output);
  static uint32_t CalcInputVBytesInTransaction(
      const BitcoinTransaction::TxInput& input);

  static uint32_t CalcTransactionWeight(const BitcoinTransaction& tx,
                                        bool dummy_signatures);
  static uint32_t CalcTransactionVBytes(const BitcoinTransaction& tx,
                                        bool dummy_signatures);
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_BITCOIN_BITCOIN_SERIALIZER_H_

/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_COMMON_SOLANA_UTILS_H_
#define COMPONENTS_WOOTZ_WALLET_COMMON_SOLANA_UTILS_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "base/containers/span.h"
#include "base/values.h"
#include "components/wootz_wallet/common/wootz_wallet.mojom-forward.h"

namespace wootz_wallet {

// Encode uint16_t value into 1-3 bytes compact-u16.
// See
// https://docs.solana.com/developing/programming-model/transactions#compact-u16-format
void CompactU16Encode(uint16_t u16, std::vector<uint8_t>* compact_u16);

std::optional<std::tuple<uint16_t, size_t>> CompactU16Decode(
    const std::vector<uint8_t>& compact_u16,
    size_t start_index);

bool IsBase58EncodedSolanaPubkey(const std::string& key);

bool Uint8ArrayDecode(const std::string& str,
                      std::vector<uint8_t>* ret,
                      size_t len);

std::optional<uint8_t> GetUint8FromStringDict(const base::Value::Dict& dict,
                                              const std::string& key);

// A compact-array is serialized as the array length, followed by each array
// item. bytes_index will be increased by the bytes read (consumed) in this
// function.
std::optional<std::vector<uint8_t>> CompactArrayDecode(
    const std::vector<uint8_t>& bytes,
    size_t* bytes_index);

bool IsValidCommitmentString(const std::string& commitment);

bool IsValidEncodingString(const std::string& encoding);

bool IsSPLToken(const mojom::BlockchainTokenPtr& token);

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_COMMON_SOLANA_UTILS_H_

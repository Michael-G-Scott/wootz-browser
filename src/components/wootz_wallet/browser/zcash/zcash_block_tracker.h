/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_ZCASH_ZCASH_BLOCK_TRACKER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_ZCASH_ZCASH_BLOCK_TRACKER_H_

#include <map>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "components/wootz_wallet/browser/block_tracker.h"
#include "components/services/wootz_wallet/public/mojom/zcash_decoder.mojom.h"

namespace wootz_wallet {

class ZCashRpc;

class ZCashBlockTracker : public BlockTracker {
 public:
  explicit ZCashBlockTracker(ZCashRpc* zcash_rpc);
  ~ZCashBlockTracker() override;
  ZCashBlockTracker(const ZCashBlockTracker&) = delete;
  ZCashBlockTracker operator=(const ZCashBlockTracker&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnLatestHeightUpdated(const std::string& chain_id,
                                       uint32_t latest_height) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void Start(const std::string& chain_id, base::TimeDelta interval) override;
  std::optional<uint32_t> GetLatestHeight(const std::string& chain_id) const;

 private:
  void GetBlockHeight(const std::string& chain_id);
  void OnGetLatestBlockForHeight(
      const std::string& chain_id,
      base::expected<zcash::mojom::BlockIDPtr, std::string> latest_height);

  // <chain_id, block_height>
  std::map<std::string, uint32_t> latest_height_map_;
  base::ObserverList<Observer> observers_;

  raw_ptr<ZCashRpc> zcash_rpc_ = nullptr;

  base::WeakPtrFactory<ZCashBlockTracker> weak_ptr_factory_{this};
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_ZCASH_ZCASH_BLOCK_TRACKER_H_

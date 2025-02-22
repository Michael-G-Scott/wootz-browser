/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_BLOCK_TRACKER_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_BLOCK_TRACKER_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace wootz_wallet {

class BlockTracker {
 public:
  BlockTracker();
  virtual ~BlockTracker();

  virtual void Start(const std::string& chain_id, base::TimeDelta interval) = 0;
  virtual void Stop(const std::string& chain_id);
  virtual void Stop();
  bool IsRunning(const std::string& chain_id) const;

 protected:
  // <chain_id, timer>
  std::map<std::string, std::unique_ptr<base::RepeatingTimer>> timers_;
};

}  // namespace wootz_wallet

#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_BLOCK_TRACKER_H_

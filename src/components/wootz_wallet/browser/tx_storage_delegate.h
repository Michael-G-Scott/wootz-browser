/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_WALLET_BROWSER_TX_STORAGE_DELEGATE_H_
#define COMPONENTS_WOOTZ_WALLET_BROWSER_TX_STORAGE_DELEGATE_H_

#include "base/observer_list_types.h"
#include "base/values.h"

namespace wootz_wallet {

class TxStorageDelegate {
 public:
  virtual ~TxStorageDelegate() = default;

  virtual bool IsInitialized() const = 0;
  virtual const base::Value::Dict& GetTxs() const = 0;
  virtual base::Value::Dict& GetTxs() = 0;
  virtual void ScheduleWrite() = 0;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnStorageInitialized() {}
  };
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
};

}  // namespace wootz_wallet
   //
#endif  // COMPONENTS_WOOTZ_WALLET_BROWSER_TX_STORAGE_DELEGATE_H_

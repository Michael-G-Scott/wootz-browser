/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.wootz_wallet.listeners;

public interface TransactionConfirmationListener {
    default void onNextTransaction(){};
    default void onApproveTransaction(){};
    default void onRejectTransaction(){};
    default void onRejectAllTransactions(){};
    default void onDismiss(){};
    default void onCancel(){};
}

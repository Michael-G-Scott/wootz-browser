/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.wootz_wallet.fragments;

import android.content.Context;

import androidx.fragment.app.Fragment;

import org.chromium.wootz_wallet.mojom.SolanaInstruction;
import org.chromium.wootz_wallet.mojom.TransactionInfo;
import org.chromium.chrome.browser.wootz_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.wootz_wallet.presenters.SolanaInstructionPresenter;
import org.chromium.chrome.browser.wootz_wallet.util.TransactionUtils;

import java.util.ArrayList;
import java.util.List;

public class SolanaTxDetailsFragment {
    public static Fragment newInstance(TransactionInfo txInfo, Context context) {
        List<TwoLineItemRecyclerViewAdapter.TwoLineItem> details = new ArrayList<>();
        SolanaInstruction[] instructions =
                TransactionUtils.safeSolData(txInfo.txDataUnion).instructions;
        details.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                context.getString(TransactionUtils.getTxType(txInfo))));

        for (SolanaInstruction solanaInstruction : instructions) {
            SolanaInstructionPresenter solanaInstructionPresenter =
                    new SolanaInstructionPresenter(solanaInstruction);
            details.addAll(solanaInstructionPresenter.toTwoLineList(context));
            if (instructions.length > 1) {
                details.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
            }
        }
        return new TwoLineItemFragment(details);
    }
}

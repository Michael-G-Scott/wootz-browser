/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.wootz_wallet.fragments.onboarding;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import androidx.fragment.app.FragmentActivity;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.wootz_wallet.activities.WootzWalletActivity;

/**
 * Initial onboarding fragment to setup Wootz Wallet.
 */
public class OnboardingInitWalletFragment extends BaseOnboardingWalletFragment {
    private static final String TAG = "SetupWalletFragment";

    private boolean mRestartSetupAction;
    private boolean mRestartRestoreAction;
    private boolean mButtonClicked;

    public OnboardingInitWalletFragment(boolean restartSetupAction, boolean restartRestoreAction) {
        mRestartSetupAction = restartSetupAction;
        mRestartRestoreAction = restartRestoreAction;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mButtonClicked = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_setup_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setAnimatedBackground(view.findViewById(R.id.setup_wallet_root));

        CardView newWallet = view.findViewById(R.id.new_wallet_card_view);
        newWallet.setOnClickListener(
                v -> {
                    if (mButtonClicked) {
                        return;
                    }
                    mButtonClicked = true;

                    checkOnChromeActivity(true, false);
                    if (mOnNextPage != null) {
                        // Add a little delay for a smooth ripple effect animation.
                        PostTask.postDelayedTask(
                                TaskTraits.UI_DEFAULT, () -> mOnNextPage.gotoCreationPage(), 200);
                    }
                });

        CardView restoreWallet = view.findViewById(R.id.restore_wallet_card_view);
        restoreWallet.setOnClickListener(
                v -> {
                    if (mButtonClicked) {
                        return;
                    }
                    mButtonClicked = true;

                    checkOnChromeActivity(false, true);
                    if (mOnNextPage != null) {
                        // Add a little delay for a smooth ripple effect animation.
                        PostTask.postDelayedTask(
                                TaskTraits.UI_DEFAULT,
                                () -> mOnNextPage.gotoRestorePage(true),
                                200);
                    }
                });
        PostTask.postTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    if (mRestartSetupAction) {
                        newWallet.performClick();
                    } else if (mRestartRestoreAction) {
                        restoreWallet.performClick();
                    }
                    mRestartSetupAction = false;
                    mRestartRestoreAction = false;
                });
    }

    @Override
    public void onResume() {
        super.onResume();
        mButtonClicked = false;
    }

    @Override
    protected boolean canBeClosed() {
        return false;
    }

    @Override
    protected boolean canNavigateBack() {
        return false;
    }

    // We need to remove that check and restart once
    // is done.
    private void checkOnChromeActivity(boolean setupAction, boolean restoreAction) {
        try {
            ChromeActivity.getChromeActivity();
        } catch (ChromeActivity.ChromeActivityNotFoundException e) {
            Log.e(TAG, "checkOnChromeActivity ANKITANKITIVAN" + e);
            Intent intent = new Intent(getActivity(), ChromeTabbedActivity.class);
            intent.putExtra(WootzWalletActivity.RESTART_WALLET_ACTIVITY, true);
            intent.putExtra(WootzWalletActivity.RESTART_WALLET_ACTIVITY_SETUP, setupAction);
            intent.putExtra(WootzWalletActivity.RESTART_WALLET_ACTIVITY_RESTORE, restoreAction);
            intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            intent.setAction(Intent.ACTION_VIEW);
            startActivity(intent);
            requireActivity().finish();
        }
    }
}

/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.wootz_wallet.util;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.wootz_wallet.mojom.AccountId;
import org.chromium.wootz_wallet.mojom.AccountInfo;
import org.chromium.wootz_wallet.mojom.CoinType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.tab.TabUtils;
import org.chromium.mojo_base.mojom.TimeDelta;

import java.nio.ByteBuffer;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.stream.Collectors;

public class WalletUtils {
    private static final String ACCOUNT_INFO = "accountInfo";
    private static final String TAG = "WalletUtils";

    private static String getNewAccountPrefixForCoin(@CoinType.EnumType int coinType) {
        switch (coinType) {
            case CoinType.ETH:
                return "Ethereum";
            case CoinType.SOL:
                return "Solana";
            case CoinType.FIL:
                return "Filecoin";
            case CoinType.BTC:
                return "Bitcoin";
        }
        assert false;
        return "";
    }

    public static String timeDeltaToDateString(TimeDelta timeDelta) {
        DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd hh:mm a", Locale.getDefault());
        return dateFormat.format(new Date(timeDelta.microseconds / 1000));
    }

    public static String generateUniqueAccountName(
            @CoinType.EnumType int coinType, AccountInfo[] accountInfos) {
        Context context = ContextUtils.getApplicationContext();
        if (context == null) {
            Log.w(TAG, "Application context was null");
            return "";
        }
        Set<String> allNames =
                Arrays.stream(accountInfos)
                        .map(acc -> acc.name)
                        .collect(Collectors.toCollection(HashSet::new));

        for (int number = 1; number < 1000; ++number) {
            String accountName =
                    context.getString(
                            R.string.new_account_prefix,
                            getNewAccountPrefixForCoin(coinType),
                            String.valueOf(number));

            if (!allNames.contains(accountName)) {
                return accountName;
            }
        }
        return "";
    }

    public static boolean accountIdsEqual(AccountId left, AccountId right) {
        return left.uniqueKey.equals(right.uniqueKey);
    }
    public static boolean accountIdsEqual(AccountInfo left, AccountInfo right) {
        return accountIdsEqual(left.accountId, right.accountId);
    }

    public static void addAccountInfoToIntent(
            @NonNull final Intent intent, @NonNull final AccountInfo accountInfo) {
        ByteBuffer bb = accountInfo.serialize();
        byte[] bytes = new byte[bb.remaining()];
        bb.get(bytes);

        intent.putExtra(ACCOUNT_INFO, bytes);
    }

    @Nullable
    public static AccountInfo getAccountInfoFromIntent(@NonNull final Intent intent) {
        byte[] bytes = intent.getByteArrayExtra(ACCOUNT_INFO);
        if (bytes == null) {
            return null;
        }
        return AccountInfo.deserialize(ByteBuffer.wrap(bytes));
    }

    public static void openWebWallet(final boolean forceNewTab) {
        try {
            ChromeActivity activity = ChromeActivity.getChromeActivity();
            activity.openNewOrSelectExistingTab(ChromeActivity.WOOTZ_WALLET_URL, true);
            TabUtils.bringChromeTabbedActivityToTheTop(activity);
        } catch (ChromeActivity.ChromeActivityNotFoundException e) {
            Log.e(TAG, "Error while opening wallet tab.", e);
        }
    }

    public static void openWalletHelpCenter(Context context) {
        if (context == null) return;
        TabUtils.openUrlInCustomTab(context, WalletConstants.WALLET_HELP_CENTER);
    }
}

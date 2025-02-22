/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.wootz_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.wootz_wallet.util.Utils;
import org.chromium.chrome.browser.wootz_wallet.util.WalletConstants;
import org.chromium.chrome.browser.wootz_wallet.util.WalletNativeUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.ui.KeyboardVisibilityDelegate;

/**
 * The preference used to reset Wootz Wallet.
 */
public class WootzWalletResetPreference
        extends Preference implements Preference.OnPreferenceClickListener {
    private static final String TAG = "WootzWalletResetPref";

    private int mPrefAccentColor;
    private final String mConfirmationPhrase;

    /**
     * Constructor for WootzWalletResetPreference.
     */
    public WootzWalletResetPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        Resources resources = getContext().getResources();
        mPrefAccentColor = getContext().getColor(R.color.wallet_error_text_color);
        mConfirmationPhrase =
                resources.getString(R.string.wootz_wallet_reset_settings_confirmation_phrase);
        setOnPreferenceClickListener(this);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        TextView titleView = (TextView) holder.findViewById(android.R.id.title);
        titleView.setTextColor(mPrefAccentColor);
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        showWootzWalletResetDialog();
        return true;
    }

    private void showWootzWalletResetDialog() {
        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.wootz_wallet_reset_content, null);
        final TextView textView = (TextView) view.findViewById(R.id.wootz_wallet_reset_textview);
        final EditText input = (EditText) view.findViewById(R.id.wootz_wallet_reset_edittext);

        textView.setText(getContext().getString(
                R.string.wootz_wallet_reset_settings_confirmation, mConfirmationPhrase));

        DialogInterface.OnClickListener onClickListener =
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int button) {
                        if (button == AlertDialog.BUTTON_POSITIVE) {
                            String inputText = input.getText().toString().trim();
                            if (TextUtils.equals(inputText, mConfirmationPhrase)) {
                                Log.w(TAG, "Reset");
                                WalletNativeUtils.resetWallet(Utils.getProfile(false));
                                KeystoreHelper.resetBiometric();
                                Utils.setCryptoOnboarding(true);

                                for (String key : WalletConstants.WOOTZ_WALLET_PREFS) {
                                    ChromeSharedPreferences.getInstance().removeKey(key);
                                }
                            }

                            // Force clear activity stack
                            launchWootzTabbedActivity();
                        } else {
                            dialog.dismiss();
                        }
                    }
                };

        AlertDialog.Builder alert =
                new AlertDialog.Builder(getContext(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
        AlertDialog alertDialog =
                alert.setTitle(R.string.wootz_wallet_reset_settings_option)
                        .setView(view)
                        .setPositiveButton(R.string.wootz_wallet_confirm_text, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                KeyboardVisibilityDelegate.getInstance().showKeyboard(input);
            }
        });
        alertDialog.show();
        final Button okButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        okButton.setEnabled(false);

        input.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {}

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // Disable ok button if input is invalid
                String inputText = s.toString().trim();

                okButton.setEnabled(TextUtils.equals(inputText, mConfirmationPhrase));
            }
        });
    }

    private void launchWootzTabbedActivity() {
        Intent intent =
                new Intent(ChromeActivity.getChromeTabbedActivity(), ChromeTabbedActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        intent.setAction(Intent.ACTION_VIEW);
        ChromeActivity.getChromeTabbedActivity().startActivity(intent);
    }
}

/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.wootz_wallet.permission;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.card.MaterialCardView;

import org.jni_zero.CalledByNative;
import org.jni_zero.NativeMethods;

import org.chromium.base.Log;
import org.chromium.wootz_wallet.mojom.AccountInfo;
import org.chromium.wootz_wallet.mojom.WootzWalletService;
import org.chromium.wootz_wallet.mojom.CoinType;
import org.chromium.wootz_wallet.mojom.KeyringService;
import org.chromium.wootz_wallet.mojom.PermissionLifetimeOption;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
// import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.wootz_wallet.WootzWalletServiceFactory;
import org.chromium.chrome.browser.wootz_wallet.fragments.dapps.ConnectAccountFragment;
import org.chromium.chrome.browser.wootz_wallet.permission.WootzPermissionAccountsListAdapter.Mode;
import org.chromium.chrome.browser.wootz_wallet.util.Utils;
import org.chromium.chrome.browser.wootz_wallet.util.WalletConstants;
import org.chromium.components.browser_ui.modaldialog.ModalDialogView;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.LayoutInflaterUtils;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modaldialog.ModalDialogManager.ModalDialogType;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modaldialog.ModalDialogProperties.ButtonType;
import org.chromium.ui.modelutil.PropertyModel;

import java.lang.ref.WeakReference;
import java.util.List;

/** Dialog to grant website permissions to use DApps. */
public class WootzDappPermissionPromptDialog
        implements ModalDialogProperties.Controller, ConnectionErrorHandler {
    private static final String TAG = "WootzDappPermission";

    private final ModalDialogManager mModalDialogManager;
    private final int mCoinType;
    private final Context mContext;
    private long mNativeDialogController;
    private PropertyModel mPropertyModel;
    private String mFavIconURL;
    private MaterialCardView mCvFavContainer;
    private ImageView mFavIconImage;
    private RecyclerView mRecyclerView;
    private WootzPermissionAccountsListAdapter mAccountsListAdapter;
    private KeyringService mKeyringService;
    private boolean mMojoServicesClosed;
    private WootzWalletService mWootzWalletService;
    private View mPermissionDialogPositiveButton;
    private WalletModel mWalletModel;

    @CalledByNative
    private static WootzDappPermissionPromptDialog create(
            long nativeDialogController,
            @NonNull WindowAndroid windowAndroid,
            String favIconURL,
            @CoinType.EnumType int coinType) {
        return new WootzDappPermissionPromptDialog(
                nativeDialogController, windowAndroid, favIconURL, coinType);
    }

    public WootzDappPermissionPromptDialog(
            long nativeDialogController,
            WindowAndroid windowAndroid,
            String favIconURL,
            @CoinType.EnumType int coinType) {
        mNativeDialogController = nativeDialogController;
        mFavIconURL = favIconURL;
        mContext = windowAndroid.getActivity().get();

        mModalDialogManager = windowAndroid.getModalDialogManager();
        mCoinType = coinType;
        mMojoServicesClosed = false;
        try {
            ChromeActivity activity = ChromeActivity.getChromeActivity();
            mWalletModel = activity.getWalletModel();
        } catch (ChromeActivity.ChromeActivityNotFoundException e) {
            Log.e(TAG, "WootzDappPermissionPromptDialog", e);
        }
    }

    // @SuppressLint("DiscouragedApi") is required to suppress "getIdentifier" usage
    // The "container" view group is part of a dialog created by PropertyModel.Builder chromium API,
    // to which we don't have any direct access so there is no direct way to access the positive
    // button (as per my R&D) of the dialog created by PropertyModel.Builder. "getIdentifier" is the
    // only option to get the exact positive button's identifier value to change its state.
    @SuppressLint("DiscouragedApi")
    @CalledByNative
    void show() {
        View customView = LayoutInflaterUtils.inflate(
                mContext, R.layout.wootz_permission_prompt_dialog, null);

        mFavIconImage = customView.findViewById(R.id.favicon);
        mCvFavContainer = customView.findViewById(R.id.permission_prompt_fav_container);
        setFavIcon();
        mRecyclerView = customView.findViewById(R.id.accounts_list);

        initWootzWalletService();
        TextView domain = customView.findViewById(R.id.domain);
        mWootzWalletService.getActiveOrigin(
                originInfo -> { domain.setText(Utils.geteTldSpanned(originInfo)); });

        mPropertyModel =
                new PropertyModel.Builder(ModalDialogProperties.ALL_KEYS)
                        .with(ModalDialogProperties.CONTROLLER, this)
                        .with(ModalDialogProperties.CUSTOM_VIEW, customView)
                        .with(ModalDialogProperties.POSITIVE_BUTTON_TEXT,
                                mContext.getString(
                                        R.string.permissions_connect_wootz_wallet_connect_button_text))
                        .with(ModalDialogProperties.NEGATIVE_BUTTON_TEXT,
                                mContext.getString(
                                        R.string.permissions_connect_wootz_wallet_back_button_text))
                        .with(ModalDialogProperties.FILTER_TOUCH_FOR_SECURITY, true)
                        .build();
        mModalDialogManager.showDialog(mPropertyModel, ModalDialogType.APP);
        initKeyringService();
        try {
            ChromeActivity activity = ChromeActivity.getChromeActivity();
            activity.dismissWalletPanelOrDialog();

            ViewGroup container = getPermissionModalViewContainer(customView);
            mPermissionDialogPositiveButton =
                    container.findViewById(activity.getResources().getIdentifier(
                            WalletConstants.PERMISSION_DIALOG_POSITIVE_BUTTON_ID,
                            WalletConstants.RESOURCE_ID, activity.getPackageName()));
            if (mPermissionDialogPositiveButton != null) {
                mPermissionDialogPositiveButton.setEnabled(false);
            }
        } catch (ChromeActivity.ChromeActivityNotFoundException e) {
            Log.e(TAG, "show " + e);
        }
        initAccounts();
    }

    @PermissionLifetimeOption.EnumType
    int getPermissionLifetimeOption() {
        return PermissionLifetimeOption.FOREVER;
    }

    @NonNull
    private ViewGroup getPermissionModalViewContainer(@NonNull View customView) {
        ViewParent viewParent = customView.getParent();
        while (viewParent.getParent() != null) {
            viewParent = viewParent.getParent();
            if (viewParent instanceof ModalDialogView) {
                break;
            }
        }
        return (ViewGroup) viewParent;
    }

    private void initWootzWalletService() {
        if (mWootzWalletService != null) {
            return;
        }
        mWootzWalletService = WootzWalletServiceFactory.getInstance().getWootzWalletService(this);
    }

    @SuppressLint("NotifyDataSetChanged")
    private void initAccounts() {
        assert mKeyringService != null;
        assert mWalletModel != null;
        // Solana accounts support only single account selection,
        // while Ethereum account offer multiple selection mode.
        final Mode mode =
                mCoinType == CoinType.SOL
                        ? Mode.SINGLE_ACCOUNT_SELECTION
                        : Mode.MULTIPLE_ACCOUNT_SELECTION;
        mAccountsListAdapter =
                new WootzPermissionAccountsListAdapter(
                        new AccountInfo[0],
                        mode,
                        null,
                        (account, checked) -> {
                            if (mPermissionDialogPositiveButton != null) {
                                mPermissionDialogPositiveButton.setEnabled(
                                        getSelectedAccounts().length > 0);
                            }
                        });
        mRecyclerView.setAdapter(mAccountsListAdapter);
        LinearLayoutManager layoutManager = new LinearLayoutManager(mContext);
        mRecyclerView.setLayoutManager(layoutManager);
        mWalletModel.getDappsModel().fetchAccountsForConnectionReq(
                mCoinType, selectedAccountAllAccounts -> {
                    AccountInfo selectedAccount = selectedAccountAllAccounts.first;
                    List<AccountInfo> accounts = selectedAccountAllAccounts.second;
                    mAccountsListAdapter.setAccounts(accounts.toArray(new AccountInfo[0]));
                    if (accounts.size() > 0) {
                        mAccountsListAdapter.setSelectedAccount(selectedAccount);
                        if (mPermissionDialogPositiveButton != null) {
                            mPermissionDialogPositiveButton.setEnabled(true);
                        }
                    }
                    mAccountsListAdapter.notifyDataSetChanged();

                    // We are on the flow from ConnectAccountFragment.connectAccount
                    ConnectAccountFragment.ConnectAccountPendingData capd =
                            ConnectAccountFragment.getAndResetConnectAccountPendingData();
                    if (capd != null) {
                        final String[] selectedAccounts = {capd.accountAddress};
                        WootzDappPermissionPromptDialogJni.get().onPrimaryButtonClicked(
                                mNativeDialogController, selectedAccounts,
                                capd.permissionLifetimeOption);
                        mModalDialogManager.dismissDialog(
                                mPropertyModel, DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
                    }
                });
    }

    private void setFavIcon() {
        if (mFavIconURL.isEmpty()) {
            return;
        }
        // ImageLoader.fetchFavIcon(mFavIconURL, new WeakReference<>(mContext), fav -> {
        //     if (fav == null) return;
        //     mFavIconImage.setImageBitmap(fav);
        //     mCvFavContainer.setVisibility(View.VISIBLE);
        // });
    }

    public String[] getSelectedAccounts() {
        assert mAccountsListAdapter != null;
        AccountInfo[] accountInfo = mAccountsListAdapter.getCheckedAccounts();
        String[] accounts = new String[accountInfo.length];
        for (int i = 0; i < accountInfo.length; i++) {
            accounts[i] = accountInfo[i].address;
        }

        return accounts;
    }

    @Override
    public void onClick(PropertyModel model, @ButtonType int buttonType) {
        if (buttonType == ButtonType.POSITIVE) {
            WootzDappPermissionPromptDialogJni.get().onPrimaryButtonClicked(
                    mNativeDialogController, getSelectedAccounts(), getPermissionLifetimeOption());
            mModalDialogManager.dismissDialog(
                    mPropertyModel, DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
        } else if (buttonType == ButtonType.NEGATIVE) {
            WootzDappPermissionPromptDialogJni.get().onNegativeButtonClicked(
                    mNativeDialogController);
            mModalDialogManager.dismissDialog(
                    mPropertyModel, DialogDismissalCause.NEGATIVE_BUTTON_CLICKED);
        }
    }

    @Override
    public void onDismiss(PropertyModel model, int dismissalCause) {
        disconnectMojoServices();
        WootzDappPermissionPromptDialogJni.get().onDialogDismissed(mNativeDialogController);
        mNativeDialogController = 0;
    }

    @CalledByNative
    private void dismissDialog() {
        mModalDialogManager.dismissDialog(mPropertyModel, DialogDismissalCause.DISMISSED_BY_NATIVE);
    }

    public void disconnectMojoServices() {
        mMojoServicesClosed = true;
        if (mKeyringService != null) {
            mKeyringService.close();
            mKeyringService = null;
        }
        if (mWootzWalletService != null) {
            mWootzWalletService.close();
            mWootzWalletService = null;
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mMojoServicesClosed || mKeyringService == null) {
            return;
        }
        mKeyringService.close();
        mKeyringService = null;
        initKeyringService();
    }

    protected void initKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = WootzWalletServiceFactory.getInstance().getKeyringService(this);
    }

    @NativeMethods
    interface Natives {
        void onPrimaryButtonClicked(long nativeWootzDappPermissionPromptDialogController,
                String[] accounts, int permissionLifetimeOption);
        void onNegativeButtonClicked(long nativeWootzDappPermissionPromptDialogController);
        void onDialogDismissed(long nativeWootzDappPermissionPromptDialogController);
    }
}

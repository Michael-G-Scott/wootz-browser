// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.autofill.settings;

import static com.google.common.truth.Truth.assertThat;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

import androidx.core.content.res.ResourcesCompat;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.test.filters.MediumTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.Features;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.AutofillUiUtils.CardIconSize;
import org.chromium.chrome.browser.autofill.AutofillUiUtils.CardIconSpecs;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.SettingsActivity;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.R;
import org.chromium.components.autofill.payments.AccountType;
import org.chromium.components.autofill.payments.BankAccount;
import org.chromium.components.autofill.payments.PaymentInstrument;
import org.chromium.components.autofill.payments.PaymentRail;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.image_fetcher.test.TestImageFetcher;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.test.util.TestThreadUtils;
import org.chromium.url.GURL;

import java.util.concurrent.TimeoutException;

/** Instrumentation tests for FinancialAccountsManagementFragment. */
@RunWith(ChromeJUnit4ClassRunner.class)
@EnableFeatures({
    ChromeFeatureList.AUTOFILL_ENABLE_SYNCING_OF_PIX_BANK_ACCOUNTS,
    ChromeFeatureList.AUTOFILL_ENABLE_NEW_CARD_ART_AND_NETWORK_IMAGES
})
@Batch(Batch.PER_CLASS)
public class FinancialAccountsManagementFragmentTest {
    @Rule public TestRule mFeaturesProcessorRule = new Features.JUnitProcessor();
    @Rule public final AutofillTestRule rule = new AutofillTestRule();

    @Rule
    public final SettingsActivityTestRule<FinancialAccountsManagementFragment>
            mSettingsActivityTestRule =
                    new SettingsActivityTestRule<>(FinancialAccountsManagementFragment.class);

    private static final GURL PIX_BANK_ACCOUNT_DISPLAY_ICON_URL = new GURL("http://example.com");
    private static final BankAccount PIX_BANK_ACCOUNT =
            new BankAccount.Builder()
                    .setPaymentInstrument(
                            new PaymentInstrument.Builder()
                                    .setInstrumentId(100L)
                                    .setNickname("nickname")
                                    .setDisplayIconUrl(PIX_BANK_ACCOUNT_DISPLAY_ICON_URL)
                                    .setSupportedPaymentRails(new int[] {PaymentRail.PIX})
                                    .build())
                    .setBankName("bank_name")
                    .setAccountNumberSuffix("account_number_suffix")
                    .setAccountType(AccountType.CHECKING)
                    .build();
    private static final Bitmap PIX_BANK_ACCOUNT_DISPLAY_ICON_BITMAP =
            Bitmap.createBitmap(100, 200, Bitmap.Config.ARGB_8888);

    private AutofillTestHelper mAutofillTestHelper;

    @Before
    public void setUp() {
        mAutofillTestHelper = new AutofillTestHelper();
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    PersonalDataManager personalDataManager =
                            AutofillTestHelper.getPersonalDataManagerForLastUsedProfile();
                    personalDataManager.setImageFetcherForTesting(
                            new TestImageFetcher(PIX_BANK_ACCOUNT_DISPLAY_ICON_BITMAP));
                    // Cache the test image in AutofillImageFetcher. Only cached images are returned
                    // immediately by the AutofillImageFetcher. If the image is not cached, it'll
                    // trigger an async fetch from the above TestImageFetcher and cache it for the
                    // next time.
                    personalDataManager
                            .getImageFetcherForTesting()
                            .addImageToCacheForTesting(
                                    PIX_BANK_ACCOUNT_DISPLAY_ICON_URL,
                                    PIX_BANK_ACCOUNT_DISPLAY_ICON_BITMAP,
                                    CardIconSpecs.create(
                                            ContextUtils.getApplicationContext(),
                                            CardIconSize.LARGE));
                    // Set the Pix pref to true.
                    getPrefService().setBoolean(Pref.FACILITATED_PAYMENTS_PIX, true);
                });
    }

    @After
    public void tearDown() throws TimeoutException {
        mAutofillTestHelper.clearAllDataForTesting();
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    AutofillTestHelper.getPersonalDataManagerForLastUsedProfile()
                            .getImageFetcherForTesting()
                            .clearCachedImagesForTesting();
                });
    }

    // Test that when Pix accounts are available the Pix preference toggle is shown.
    @Test
    @MediumTest
    public void testPixAccountAvailable_pixSwitchShown() throws Exception {
        AutofillTestHelper.addMaskedBankAccount(PIX_BANK_ACCOUNT);

        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        // Verify that the switch preference for Pix is displayed.
        ChromeSwitchPreference pixSwitch = getPixSwitchPreference(activity);
        assertThat(pixSwitch).isNotNull();
    }

    // Test that when Pix accounts are not available the Pix preference toggle is not shown.
    @Test
    @MediumTest
    public void testPixAccountNotAvailable_pixSwitchNotShown() throws Exception {
        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        // Verify that the switch preference for Pix is not displayed.
        ChromeSwitchPreference pixSwitch = getPixSwitchPreference(activity);
        assertThat(pixSwitch).isNull();
    }

    // Test that when Pix profile preference is set to true, the Pix toggle is checked.
    @Test
    @MediumTest
    public void testPixPrefEnabled_pixSwitchEnabled() throws Exception {
        AutofillTestHelper.addMaskedBankAccount(PIX_BANK_ACCOUNT);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    getPrefService().setBoolean(Pref.FACILITATED_PAYMENTS_PIX, true);
                });

        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        // Verify that the switch preference for Pix is displayed and is checked.
        ChromeSwitchPreference pixSwitch = getPixSwitchPreference(activity);
        assertThat(pixSwitch.isChecked()).isTrue();
    }

    // Test that when the Pix profile preference is set to false, the Pix toggle is not checked.
    @Test
    @MediumTest
    public void testPixPrefDisabled_pixSwitchDisabled() throws Exception {
        AutofillTestHelper.addMaskedBankAccount(PIX_BANK_ACCOUNT);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    getPrefService().setBoolean(Pref.FACILITATED_PAYMENTS_PIX, false);
                });

        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        // Verify that the switch preference for Pix is displayed the is not checked.
        ChromeSwitchPreference pixSwitch = getPixSwitchPreference(activity);
        assertThat(pixSwitch.isChecked()).isFalse();
    }

    @Test
    @MediumTest
    public void testPixAccountShown() {
        AutofillTestHelper.addMaskedBankAccount(PIX_BANK_ACCOUNT);

        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        String expectedPrefSummary =
                String.format(
                        "Pix  •  %s ••••%s",
                        activity.getString(R.string.bank_account_type_checking),
                        PIX_BANK_ACCOUNT.getAccountNumberSuffix());
        Preference bankAccountPref = getBankAccountPreference(activity, PIX_BANK_ACCOUNT);
        assertThat(bankAccountPref.getTitle()).isEqualTo(PIX_BANK_ACCOUNT.getBankName());
        assertThat(bankAccountPref.getSummary()).isEqualTo(expectedPrefSummary);
        assertThat(bankAccountPref.getWidgetLayoutResource())
                .isEqualTo(R.layout.autofill_server_data_label);
        assertThat(((BitmapDrawable) bankAccountPref.getIcon()).getBitmap())
                .isEqualTo(PIX_BANK_ACCOUNT_DISPLAY_ICON_BITMAP);
    }

    @Test
    @MediumTest
    public void testPixAccountDisplayIconUrlAbsent_defaulAccountBalanceIconShown() {
        AutofillTestHelper.addMaskedBankAccount(
                new BankAccount.Builder()
                        .setPaymentInstrument(
                                new PaymentInstrument.Builder()
                                        .setInstrumentId(100L)
                                        .setNickname("nickname")
                                        .setSupportedPaymentRails(new int[] {PaymentRail.PIX})
                                        .build())
                        .setBankName("bank_name")
                        .setAccountNumberSuffix("account_number_suffix")
                        .setAccountType(AccountType.CHECKING)
                        .build());
        String bankAccountPrefKey =
                String.format(
                        FinancialAccountsManagementFragment.PREFERENCE_KEY_PIX_BANK_ACCOUNT,
                        PIX_BANK_ACCOUNT.getInstrumentId());

        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        Preference bankAccountPref =
                getPreferenceScreen(activity).findPreference(bankAccountPrefKey);
        assertThat(
                        convertDrawableToBitmap(bankAccountPref.getIcon())
                                .sameAs(
                                        convertDrawableToBitmap(
                                                ResourcesCompat.getDrawable(
                                                        activity.getResources(),
                                                        R.drawable.ic_account_balance,
                                                        activity.getTheme()))))
                .isTrue();
    }

    @Test
    @MediumTest
    public void testActivityTriggered_noArgs_emptyTitle() {
        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        assertThat(activity.getTitle().toString()).isEmpty();
    }

    @Test
    @MediumTest
    public void testActivityTriggered_titlePresentInArgs_titleSet() {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putString(FinancialAccountsManagementFragment.TITLE_KEY, "Title");

        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity(fragmentArgs);

        assertThat(activity.getTitle().toString()).isEqualTo("Title");
    }

    @Test
    @MediumTest
    public void testActivityTriggered_titleNotPresentInArgs_emptyTitle() {
        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity(new Bundle());

        assertThat(activity.getTitle().toString()).isEmpty();
    }

    // Test that the icon for the Pix bank account preference is not set if the icon is not already
    // cached in memory.
    @Test
    @MediumTest
    public void testPixAccountDisplayIconNotCached_prefIconSetToDefault() {
        AutofillTestHelper.addMaskedBankAccount(PIX_BANK_ACCOUNT);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    AutofillTestHelper.getPersonalDataManagerForLastUsedProfile()
                            .getImageFetcherForTesting()
                            .clearCachedImagesForTesting();
                });
        String bankAccountPrefKey =
                String.format(
                        FinancialAccountsManagementFragment.PREFERENCE_KEY_PIX_BANK_ACCOUNT,
                        PIX_BANK_ACCOUNT.getInstrumentId());

        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity();

        Preference bankAccountPref =
                getPreferenceScreen(activity).findPreference(bankAccountPrefKey);
        assertThat(
                        convertDrawableToBitmap(bankAccountPref.getIcon())
                                .sameAs(
                                        convertDrawableToBitmap(
                                                ResourcesCompat.getDrawable(
                                                        activity.getResources(),
                                                        R.drawable.ic_account_balance,
                                                        activity.getTheme()))))
                .isTrue();
    }

    // Test that Pix bank accounts are removed when the Pix toggle is turned off.
    @Test
    @MediumTest
    public void testPixSwitchDisabled_bankAccountPrefsRemoved() {
        AutofillTestHelper.addMaskedBankAccount(PIX_BANK_ACCOUNT);
        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity(new Bundle());
        ChromeSwitchPreference pixSwitch = getPixSwitchPreference(activity);
        Preference bankAccountPref = getBankAccountPreference(activity, PIX_BANK_ACCOUNT);
        assertThat(bankAccountPref).isNotNull();

        // Set the Pix toggle to off.
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    pixSwitch.performClick();
                });

        // Verify that the bank account preference is now null.
        bankAccountPref = getBankAccountPreference(activity, PIX_BANK_ACCOUNT);
        assertThat(bankAccountPref).isNull();
    }

    // Test that Pix bank accounts are added when the Pix toggle is turned on.
    @Test
    @MediumTest
    public void testPixSwitchEnabled_bankAccountPrefsAdded() {
        AutofillTestHelper.addMaskedBankAccount(PIX_BANK_ACCOUNT);
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    getPrefService().setBoolean(Pref.FACILITATED_PAYMENTS_PIX, false);
                });
        SettingsActivity activity = mSettingsActivityTestRule.startSettingsActivity(new Bundle());
        ChromeSwitchPreference pixSwitch = getPixSwitchPreference(activity);

        Preference bankAccountPref = getBankAccountPreference(activity, PIX_BANK_ACCOUNT);
        assertThat(bankAccountPref).isNull();

        // Set the Pix toggle to on.
        TestThreadUtils.runOnUiThreadBlocking(
                () -> {
                    pixSwitch.performClick();
                });

        // Verify that the bank account preference is now not null.
        bankAccountPref = getBankAccountPreference(activity, PIX_BANK_ACCOUNT);
        assertThat(bankAccountPref).isNotNull();
    }

    private static PreferenceScreen getPreferenceScreen(SettingsActivity activity) {
        return ((FinancialAccountsManagementFragment) activity.getMainFragment())
                .getPreferenceScreen();
    }

    private static PrefService getPrefService() {
        return UserPrefs.get(ProfileManager.getLastUsedRegularProfile());
    }

    private static ChromeSwitchPreference getPixSwitchPreference(SettingsActivity activity) {
        return (ChromeSwitchPreference)
                getPreferenceScreen(activity)
                        .findPreference(FinancialAccountsManagementFragment.PREFERENCE_KEY_PIX);
    }

    private static Preference getBankAccountPreference(
            SettingsActivity activity, BankAccount bankAccount) {
        String bankAccountPrefKey =
                String.format(
                        FinancialAccountsManagementFragment.PREFERENCE_KEY_PIX_BANK_ACCOUNT,
                        bankAccount.getInstrumentId());
        return getPreferenceScreen(activity).findPreference(bankAccountPrefKey);
    }

    private static Bitmap convertDrawableToBitmap(Drawable drawable) {
        Bitmap bitmap =
                Bitmap.createBitmap(
                        drawable.getIntrinsicWidth(),
                        drawable.getIntrinsicHeight(),
                        Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        return bitmap;
    }
}

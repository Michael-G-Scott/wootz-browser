// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab_resumption;

import androidx.annotation.Nullable;

import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.magic_stack.HomeModulesConfigManager;
import org.chromium.chrome.browser.magic_stack.HomeModulesMetricsUtils;
import org.chromium.chrome.browser.magic_stack.ModuleDelegate;
import org.chromium.chrome.browser.magic_stack.ModuleDelegate.ModuleType;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.signin.services.IdentityServicesProvider;
import org.chromium.chrome.browser.sync.SyncServiceFactory;
import org.chromium.chrome.browser.tab_resumption.TabResumptionModuleMetricsUtils.ModuleNotShownReason;
import org.chromium.components.signin.identitymanager.ConsentLevel;

/** Utility class for the decision funnel on showing or hiding the tab resumption module. */
public class TabResumptionModuleEnablement {

    static class LocalTab {
        static boolean isFeatureEnabled() {
            return HomeModulesMetricsUtils.HOME_MODULES_COMBINE_TABS.getValue();
        }

        static boolean isAllowedByConfig() {
            return HomeModulesConfigManager.getInstance()
                    .getPrefModuleTypeEnabled(ModuleType.TAB_RESUMPTION);
        }

        static boolean hasData(ModuleDelegate moduleDelegate) {
            return moduleDelegate.getTrackingTab() != null;
        }

        static boolean shouldMakeProvider(ModuleDelegate moduleDelegate) {
            return isFeatureEnabled() && isAllowedByConfig() && hasData(moduleDelegate);
        }
    }

    static class SyncDerived {
        static boolean isFeatureEnabled() {
            return ChromeFeatureList.sTabResumptionModuleAndroid.isEnabled();
        }

        static boolean isAllowedByConfig() {
            return HomeModulesConfigManager.getInstance()
                    .getPrefModuleTypeEnabled(ModuleType.TAB_RESUMPTION);
        }

        static boolean isSignedIn(Profile profile) {
            return IdentityServicesProvider.get()
                    .getIdentityManager(profile)
                    .hasPrimaryAccount(ConsentLevel.SYNC);
        }

        static boolean isSyncEnabled(Profile profile) {
            return SyncServiceFactory.getForProfile(profile).hasKeepEverythingSynced();
        }

        static boolean shouldMakeProvider(Profile profile) {
            return isFeatureEnabled()
                    && isAllowedByConfig()
                    && isSignedIn(profile)
                    && isSyncEnabled(profile);
        }
    }

    /**
     * Returns whether Magic Stack can potentially show the tab resumption module, based on module
     * feature enablement, ignoring other user settings and data availability.
     */
    static boolean isFeatureEnabled() {
        return LocalTab.isFeatureEnabled() || SyncDerived.isFeatureEnabled();
    }

    /**
     * Returns whether Magic Stack should attempt to show the tab resumption module, based on module
     * feature enablement and other user settings, ignoring data availability. The return value
     * encodes the reason as ModuleNotShownReason (for logging) if the decision is to not show.
     *
     * @return null if should show, otherwise Integer wrapping ModuleNotShownReason.
     */
    static @Nullable Integer computeModuleNotShownReason(
            ModuleDelegate moduleDelegate, Profile profile) {
        if (LocalTab.isFeatureEnabled() && LocalTab.hasData(moduleDelegate)) return null;

        if (!SyncDerived.isFeatureEnabled()) return ModuleNotShownReason.FEATURE_DISABLED;

        if (!SyncDerived.isAllowedByConfig()) return ModuleNotShownReason.FEATURE_DISABLED;

        if (!SyncDerived.isSignedIn(profile)) return ModuleNotShownReason.NOT_SIGNED_IN;

        if (!SyncDerived.isSyncEnabled(profile)) return ModuleNotShownReason.NOT_SYNC;

        return null;
    }
}

// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/login/screens/locale_switch_screen.h"

#include "ash/constants/ash_features.h"
#include "base/containers/contains.h"
#include "base/time/time.h"
#include "chrome/browser/ash/base/locale_util.h"
#include "chrome/browser/ash/login/login_pref_names.h"
#include "chrome/browser/ash/login/screens/locale_switch_notification.h"
#include "chrome/browser/ash/login/users/chrome_user_manager_util.h"
#include "chrome/browser/ash/login/wizard_context.h"
#include "chrome/browser/ash/profiles/profile_helper.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/ui/webui/ash/login/locale_switch_screen_handler.h"
#include "chromeos/ash/components/osauth/public/auth_session_storage.h"
#include "components/language/core/browser/pref_names.h"
#include "components/language/core/common/locale_util.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/signin_metrics.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/user_manager/user_manager.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace ash {
namespace {

constexpr base::TimeDelta kWaitTimeout = base::Seconds(5);

// Returns whether all information needed (locale and account capabilities)
// has been fetched.
bool IsAllInfoFetched(const AccountInfo& info) {
  return !info.locale.empty() && info.capabilities.AreAllCapabilitiesKnown();
}

}  // namespace

// static
std::string LocaleSwitchScreen::GetResultString(Result result) {
  // LINT.IfChange(UsageMetrics)
  switch (result) {
    case Result::kLocaleFetchFailed:
      return "LocaleFetchFailed";
    case Result::kLocaleFetchTimeout:
      return "LocaleFetchTimeout";
    case Result::kNoSwitchNeeded:
      return "NoSwitchNeeded";
    case Result::kSwitchFailed:
      return "SwitchFailed";
    case Result::kSwitchSucceded:
      return "SwitchSucceded";
    case Result::kSwitchDelegated:
      return "SwitchDelegated";
    case Result::kNotApplicable:
      return BaseScreen::kNotApplicable;
  }
  // LINT.ThenChange(//tools/metrics/histograms/metadata/oobe/histograms.xml)
}

LocaleSwitchScreen::LocaleSwitchScreen(base::WeakPtr<LocaleSwitchView> view,
                                       const ScreenExitCallback& exit_callback)
    : BaseScreen(LocaleSwitchView::kScreenId, OobeScreenPriority::DEFAULT),
      view_(std::move(view)),
      exit_callback_(exit_callback) {}

LocaleSwitchScreen::~LocaleSwitchScreen() = default;

bool LocaleSwitchScreen::MaybeSkip(WizardContext& wizard_context) {
  if (wizard_context.skip_post_login_screens_for_tests) {
    exit_callback_.Run(Result::kNotApplicable);
    return true;
  }

  // Skip GAIA language sync if user specifically set language through the UI
  // on the welcome screen.
  PrefService* local_state = g_browser_process->local_state();
  if (local_state->GetBoolean(prefs::kOobeLocaleChangedOnWelcomeScreen)) {
    VLOG(1) << "Skipping GAIA language sync because user chose specific"
            << " locale on the Welcome Screen.";
    local_state->ClearPref(prefs::kOobeLocaleChangedOnWelcomeScreen);
    exit_callback_.Run(Result::kNotApplicable);
    return true;
  }

  user_manager::User* user = user_manager::UserManager::Get()->GetActiveUser();
  if (user->HasGaiaAccount()) {
    return false;
  }

  // Switch language if logging into a managed guest session.
  if (user_manager::UserManager::Get()->IsLoggedInAsManagedGuestSession()) {
    return false;
  }

  exit_callback_.Run(Result::kNotApplicable);
  return true;
}

void LocaleSwitchScreen::ShowImpl() {
  if (ash::features::AreLocalPasswordsEnabledForConsumers()) {
    if (context()->extra_factors_token) {
      session_refresher_ = AuthSessionStorage::Get()->KeepAlive(
          context()->extra_factors_token.value());
    }
  }

  user_manager::User* user = user_manager::UserManager::Get()->GetActiveUser();
  DCHECK(user->is_profile_created());
  Profile* profile = ProfileHelper::Get()->GetProfileByUser(user);
  if (user->GetType() == user_manager::UserType::kPublicAccount) {
    std::string locale =
        profile->GetPrefs()->GetString(language::prefs::kApplicationLocale);
    DCHECK(!locale.empty());
    SwitchLocale(std::move(locale));
    return;
  }

  DCHECK(user->HasGaiaAccount());

  identity_manager_ = IdentityManagerFactory::GetForProfile(profile);
  if (!identity_manager_) {
    NOTREACHED_IN_MIGRATION();
    exit_callback_.Run(Result::kNotApplicable);
    return;
  }

  CoreAccountId primary_account_id =
      identity_manager_->GetPrimaryAccountId(signin::ConsentLevel::kSignin);
  refresh_token_loaded_ =
      identity_manager_->HasAccountWithRefreshToken(primary_account_id);

  if (identity_manager_->GetErrorStateOfRefreshTokenForAccount(
          primary_account_id) != GoogleServiceAuthError::AuthErrorNone()) {
    exit_callback_.Run(Result::kLocaleFetchFailed);
    return;
  }

  identity_manager_observer_.Observe(identity_manager_.get());

  gaia_id_ = user->GetAccountId().GetGaiaId();
  const AccountInfo account_info =
      identity_manager_->FindExtendedAccountInfoByGaiaId(gaia_id_);
  if (!refresh_token_loaded_ || !IsAllInfoFetched(account_info)) {
    // Will continue from observer.
    timeout_waiter_.Start(FROM_HERE, kWaitTimeout,
                          base::BindOnce(&LocaleSwitchScreen::OnTimeout,
                                         weak_factory_.GetWeakPtr()));
    return;
  }

  std::string locale = account_info.locale;
  SwitchLocale(std::move(locale));
}

void LocaleSwitchScreen::HideImpl() {
  session_refresher_.reset();
  ResetState();
}

void LocaleSwitchScreen::OnErrorStateOfRefreshTokenUpdatedForAccount(
    const CoreAccountInfo& account_info,
    const GoogleServiceAuthError& error,
    signin_metrics::SourceForRefreshTokenOperation token_operation_source) {
  if (error == GoogleServiceAuthError::AuthErrorNone()) {
    return;
  }
  if (account_info.gaia != gaia_id_) {
    return;
  }
  ResetState();
  exit_callback_.Run(Result::kLocaleFetchFailed);
}

void LocaleSwitchScreen::OnExtendedAccountInfoUpdated(
    const AccountInfo& account_info) {
  if (account_info.gaia != gaia_id_ || !refresh_token_loaded_ ||
      !IsAllInfoFetched(account_info)) {
    return;
  }
  SwitchLocale(account_info.locale);
}

void LocaleSwitchScreen::OnRefreshTokensLoaded() {
  // Account information can only be guaranteed correct after refresh tokens
  // are loaded.
  refresh_token_loaded_ = true;
  OnExtendedAccountInfoUpdated(
      identity_manager_->FindExtendedAccountInfoByGaiaId(gaia_id_));
}

void LocaleSwitchScreen::SwitchLocale(std::string locale) {
  ResetState();

  language::ConvertToActualUILocale(&locale);

  if (locale.empty() || locale == g_browser_process->GetApplicationLocale()) {
    exit_callback_.Run(Result::kNoSwitchNeeded);
    return;
  }

  // Types of users that have a GAIA account and could be used during the
  // "Add Person" flow.
  static constexpr user_manager::UserType kAddPersonUserTypes[] = {
      user_manager::UserType::kRegular, user_manager::UserType::kChild};
  const user_manager::User* user =
      user_manager::UserManager::Get()->GetActiveUser();
  // Don't show notification for the ephemeral logins, proceed with the default
  // flow.
  if (!chrome_user_manager_util::IsManagedGuestSessionOrEphemeralLogin() &&
      context()->is_add_person_flow &&
      base::Contains(kAddPersonUserTypes, user->GetType())) {
    VLOG(1) << "Add Person flow detected, delegating locale switch decision"
            << " to the user.";
    // Delegate language switch to the notification. User will be able to
    // decide whether switch/not switch on their own.
    Profile* profile = ProfileHelper::Get()->GetProfileByUser(user);
    locale_util::SwitchLanguageCallback callback(base::BindOnce(
        &LocaleSwitchScreen::OnLanguageChangedNotificationCallback,
        weak_factory_.GetWeakPtr()));
    LocaleSwitchNotification::Show(profile, std::move(locale),
                                   std::move(callback));
    exit_callback_.Run(Result::kSwitchDelegated);
    return;
  }

  locale_util::SwitchLanguageCallback callback(
      base::BindOnce(&LocaleSwitchScreen::OnLanguageChangedCallback,
                     weak_factory_.GetWeakPtr()));
  locale_util::SwitchLanguage(
      locale,
      /*enable_locale_keyboard_layouts=*/false,  // The layouts will be synced
                                                 // instead. Also new user could
                                                 // enable required layouts from
                                                 // the settings.
      /*login_layouts_only=*/false, std::move(callback),
      ProfileManager::GetActiveUserProfile());
}

void LocaleSwitchScreen::OnLanguageChangedCallback(
    const locale_util::LanguageSwitchResult& result) {
  if (!result.success) {
    exit_callback_.Run(Result::kSwitchFailed);
    return;
  }

  view_->UpdateStrings();
  exit_callback_.Run(Result::kSwitchSucceded);
}

void LocaleSwitchScreen::OnLanguageChangedNotificationCallback(
    const locale_util::LanguageSwitchResult& result) {
  if (!result.success) {
    return;
  }

  view_->UpdateStrings();
}

void LocaleSwitchScreen::ResetState() {
  identity_manager_observer_.Reset();
  timeout_waiter_.AbandonAndStop();
}

void LocaleSwitchScreen::OnTimeout() {
  const AccountInfo account_info =
      identity_manager_->FindExtendedAccountInfoByGaiaId(gaia_id_);
  if (refresh_token_loaded_ && !account_info.locale.empty()) {
    // We should switch locale if locale is fetched but it timed out while
    // waiting for other account information (e.g. capabilities).
    SwitchLocale(account_info.locale);
  } else {
    ResetState();
    // If it happens during the tests - something is wrong with the test
    // configuration. Thus making it debug log.
    DLOG(ERROR) << "Timeout of the locale fetch";
    exit_callback_.Run(Result::kLocaleFetchTimeout);
  }
}

}  // namespace ash

// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/app_service/publisher_helper.h"

#include "chrome/browser/web_applications/web_app_constants.h"
#include "chrome/browser/web_applications/web_app_provider.h"
#include "chrome/browser/web_applications/web_app_registrar.h"
#include "components/services/app_service/public/cpp/app_types.h"
#include "components/services/app_service/public/cpp/shortcut/shortcut.h"
#include "components/webapps/browser/installable/installable_metrics.h"

#if BUILDFLAG(IS_CHROMEOS)
#include "chromeos/constants/chromeos_features.h"
#endif

namespace web_app {

webapps::WebappUninstallSource ConvertUninstallSourceToWebAppUninstallSource(
    apps::UninstallSource uninstall_source) {
  switch (uninstall_source) {
    case apps::UninstallSource::kAppList:
      return webapps::WebappUninstallSource::kAppList;
    case apps::UninstallSource::kAppManagement:
      return webapps::WebappUninstallSource::kAppManagement;
    case apps::UninstallSource::kShelf:
      return webapps::WebappUninstallSource::kShelf;
    case apps::UninstallSource::kMigration:
      return webapps::WebappUninstallSource::kMigration;
    case apps::UninstallSource::kUnknown:
      return webapps::WebappUninstallSource::kUnknown;
  }
}

bool IsAppServiceShortcut(const webapps::AppId& web_app_id,
                          const WebAppProvider& provider) {
// On non-ChromeOS platforms, shortcuts will still be published as web apps.
#if BUILDFLAG(IS_CHROMEOS)
  if (chromeos::features::IsCrosWebAppShortcutUiUpdateEnabled()) {
    return provider.registrar_unsafe().IsInstalled(web_app_id) &&
           provider.registrar_unsafe().IsShortcutApp(web_app_id);
  }
#endif
  return false;
}

apps::ShortcutSource ConvertWebAppManagementTypeToShortcutSource(
    WebAppManagement::Type management_type) {
  switch (management_type) {
    case WebAppManagement::Type::kSync:
    case WebAppManagement::Type::kWebAppStore:
    case WebAppManagement::Type::kOneDriveIntegration:
    case WebAppManagement::Type::kIwaUserInstalled:
      return apps::ShortcutSource::kUser;
    case WebAppManagement::Type::kPolicy:
    case WebAppManagement::Type::kIwaPolicy:
      return apps::ShortcutSource::kPolicy;
    case WebAppManagement::Type::kOem:
    case WebAppManagement::Type::kApsDefault:
    case WebAppManagement::Type::kDefault:
      return apps::ShortcutSource::kDefault;
    case WebAppManagement::Type::kKiosk:
    case WebAppManagement::Type::kSystem:
    case WebAppManagement::Type::kIwaShimlessRma:
    case WebAppManagement::Type::kSubApp:
      return apps::ShortcutSource::kUnknown;
  }
}

}  // namespace web_app

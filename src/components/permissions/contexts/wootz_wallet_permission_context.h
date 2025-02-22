/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_PERMISSIONS_CONTEXTS_WOOTZ_WALLET_PERMISSION_CONTEXT_H_
#define COMPONENTS_PERMISSIONS_CONTEXTS_WOOTZ_WALLET_PERMISSION_CONTEXT_H_

#include <map>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "components/wootz_wallet/common/wootz_wallet.mojom.h"
#include "components/permissions/permission_context_base.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/request_type.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace permissions {

class WootzWalletPermissionContext : public PermissionContextBase {
 public:
  // using PermissionContextBase::RequestPermission;
  explicit WootzWalletPermissionContext(
      content::BrowserContext* browser_context,
      ContentSettingsType content_settings_type);
  ~WootzWalletPermissionContext() override;

  WootzWalletPermissionContext(const WootzWalletPermissionContext&) = delete;
  WootzWalletPermissionContext& operator=(const WootzWalletPermissionContext&) =
      delete;

  /**
   * This is called by PermissionManager::RequestPermissions, for each
   * permission request ID, we will parse the requesting_frame URL to get the
   * ethereum address list to be used for each sub-request. Each sub-request
   * will then consume one address from the saved list and call
   * PermissionContextBase::RequestPermission with it.
   */
  void RequestPermission(PermissionRequestData request_data,
                         BrowserPermissionCallback callback) override;

  static void RequestPermissions(
      blink::PermissionType permission,
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses,
      base::OnceCallback<
          void(const std::vector<blink::mojom::PermissionStatus>&)> callback);
  static bool HasRequestsInProgress(content::RenderFrameHost* rfh,
                                    permissions::RequestType request_type);
  static void AcceptOrCancel(
      const std::vector<std::string>& accounts,
      wootz_wallet::mojom::PermissionLifetimeOption option,
      content::WebContents* web_contents);
  static void Cancel(content::WebContents* web_contents);

  static std::optional<std::vector<std::string>> GetAllowedAccounts(
      blink::PermissionType permission,
      content::RenderFrameHost* rfh,
      const std::vector<std::string>& addresses);

  // We will only check global setting and setting per origin since we won't
  // write block rule per address on an origin.
  static bool IsPermissionDenied(blink::PermissionType permission,
                                 content::BrowserContext* context,
                                 const url::Origin& origin);

  static bool AddPermission(blink::PermissionType permission,
                            content::BrowserContext* context,
                            const url::Origin& origin,
                            const std::string& account);
  static bool HasPermission(blink::PermissionType permission,
                            content::BrowserContext* context,
                            const url::Origin& origin,
                            const std::string& account,
                            bool* has_permission);
  static bool ResetPermission(blink::PermissionType permission,
                              content::BrowserContext* context,
                              const url::Origin& origin,
                              const std::string& account);
  static void ResetAllPermissions(content::BrowserContext* context);

  static std::vector<std::string> GetWebSitesWithPermission(
      blink::PermissionType permission,
      content::BrowserContext* context);
  static bool ResetWebSitePermission(blink::PermissionType permission,
                                     content::BrowserContext* context,
                                     const std::string& formed_website);

 protected:
  bool IsRestrictedToSecureOrigins() const override;

 private:
  std::map<std::string, std::queue<std::string>> request_address_queues_;
};

}  // namespace permissions

#endif  // COMPONENTS_PERMISSIONS_CONTEXTS_WOOTZ_WALLET_PERMISSION_CONTEXT_H_

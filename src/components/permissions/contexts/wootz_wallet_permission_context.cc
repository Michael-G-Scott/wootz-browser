/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/permissions/contexts/wootz_wallet_permission_context.h"

#include <optional>
#include <utility>

#include "components/wootz_wallet/browser/permission_utils.h"
#include "components/permissions/permission_lifetime_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_manager.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_data.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permissions_client.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/permission_request_description.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"
#include "url/origin.h"

namespace permissions {

namespace {

bool IsAccepted(PermissionRequest* request,
                const std::vector<std::string>& accounts) {
  for (const auto& account : accounts) {
    if (base::EndsWith(request->requesting_origin().host_piece(), account,
                       base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }

  return false;
}

}  // namespace

WootzWalletPermissionContext::WootzWalletPermissionContext(
    content::BrowserContext* browser_context,
    ContentSettingsType content_settings_type)
    : PermissionContextBase(browser_context,
                            content_settings_type,
                            blink::mojom::PermissionsPolicyFeature::kNotFound) {
}

WootzWalletPermissionContext::~WootzWalletPermissionContext() = default;

bool WootzWalletPermissionContext::IsRestrictedToSecureOrigins() const {
  // For parity with Crypto Wallets and MM we should allow a permission prompt
  // to be shown for HTTP sites. Developers often use localhost for development
  // for example.
  return false;
}

void WootzWalletPermissionContext::RequestPermission(
    PermissionRequestData request_data,
    BrowserPermissionCallback callback) {
  const std::string id_str = request_data.id.ToString();
  url::Origin requesting_origin =
      url::Origin::Create(request_data.requesting_origin);
  url::Origin origin;
  permissions::RequestType type =
      ContentSettingsTypeToRequestType(content_settings_type());

  // Parse address list from the requesting frame and save it to the map when
  // it is the first time seeing this request ID.
  auto addr_queue_it = request_address_queues_.find(id_str);
  std::queue<std::string> address_queue;
  bool is_new_id = addr_queue_it == request_address_queues_.end();
  if (!wootz_wallet::ParseRequestingOrigin(
          type, requesting_origin, &origin,
          is_new_id ? &address_queue : nullptr)) {
    content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
        request_data.id.global_render_frame_host_id());
    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(rfh);
    GURL embedding_origin =
        url::Origin::Create(web_contents->GetLastCommittedURL()).GetURL();
    NotifyPermissionSet(request_data.id, requesting_origin.GetURL(),
                        embedding_origin, std::move(callback),
                        /*persist=*/false, CONTENT_SETTING_BLOCK,
                        /*is_one_time=*/false,
                        /*is_final_decision=*/true);
    return;
  }
  if (is_new_id) {
    addr_queue_it =
        request_address_queues_.insert(std::make_pair(id_str, address_queue))
            .first;
  }

  // Overwrite the requesting_frame URL for each sub-request with one address
  // at a time from the map.
  auto& addr_queue = addr_queue_it->second;
  DCHECK(!addr_queue.empty());
  url::Origin sub_request_origin;
  wootz_wallet::GetSubRequestOrigin(type, origin, addr_queue.front(),
                                    &sub_request_origin);
  addr_queue.pop();
  if (addr_queue.empty()) {
    request_address_queues_.erase(addr_queue_it);
  }
  auto data =
      PermissionRequestData(this, request_data.id, request_data.user_gesture,
                            sub_request_origin.GetURL());
  // This will prevent PermissionRequestManager from reprioritize the request
  // queue.
  data.embedded_permission_element_initiated = true;
  PermissionContextBase::RequestPermission(std::move(data),
                                           std::move(callback));
}

// static
void WootzWalletPermissionContext::AcceptOrCancel(
    const std::vector<std::string>& accounts,
    wootz_wallet::mojom::PermissionLifetimeOption option,
    content::WebContents* web_contents) {
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (!manager) {
    return;
  }

  std::vector<PermissionRequest*> allowed_requests;
  std::vector<PermissionRequest*> cancelled_requests;
  for (PermissionRequest* request : manager->Requests()) {
    if (IsAccepted(request, accounts)) {
      const auto options = CreatePermissionLifetimeOptions();
      SetRequestLifetime(options, static_cast<size_t>(option), request);
      allowed_requests.push_back(request);
    } else {
      cancelled_requests.push_back(request);
    }
  }

  manager->AcceptDenyCancel(allowed_requests, std::vector<PermissionRequest*>(),
                            cancelled_requests);
}

// static
void WootzWalletPermissionContext::Cancel(content::WebContents* web_contents) {
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (!manager) {
    return;
  }

  // Dismiss all requests.
  manager->Dismiss();
}

// static
bool WootzWalletPermissionContext::HasRequestsInProgress(
    content::RenderFrameHost* rfh,
    permissions::RequestType request_type) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (!manager) {
    return false;
  }

  // Only check the first entry because it will not be grouped with other types
  return !manager->Requests().empty() &&
         manager->Requests()[0]->request_type() == request_type;
}

// static
void WootzWalletPermissionContext::RequestPermissions(
    blink::PermissionType permission,
    content::RenderFrameHost* rfh,
    const std::vector<std::string>& addresses,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  if (!rfh) {
    std::move(callback).Run(std::vector<blink::mojom::PermissionStatus>());
    return;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    std::move(callback).Run(std::vector<blink::mojom::PermissionStatus>());
    return;
  }

  // Create a vector of GRANTED status for each address
  std::vector<blink::mojom::PermissionStatus> results(
      addresses.size(), blink::mojom::PermissionStatus::GRANTED);
  
  // Store the permissions in settings
  const ContentSettingsType settings_type = 
      PermissionUtil::PermissionTypeToContentSettingTypeSafe(permission);
  
  PermissionsClient::Get()
      ->GetSettingsMap(web_contents->GetBrowserContext())
      ->SetContentSettingDefaultScope(
          rfh->GetLastCommittedOrigin().GetURL(),
          rfh->GetLastCommittedOrigin().GetURL(),
          settings_type,
          CONTENT_SETTING_ALLOW);
  
  // Return granted status for all addresses
  std::move(callback).Run(results);
}

// static
std::optional<std::vector<std::string>>
WootzWalletPermissionContext::GetAllowedAccounts(
    blink::PermissionType permission,
    content::RenderFrameHost* rfh,
    const std::vector<std::string>& addresses) {
  if (!rfh) {
    return std::nullopt;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    return std::vector<std::string>();
  }

  // Check if permission is already granted
  const ContentSettingsType settings_type = 
      PermissionUtil::PermissionTypeToContentSettingTypeSafe(permission);
      
  auto setting = PermissionsClient::Get()
                    ->GetSettingsMap(web_contents->GetBrowserContext())
                    ->GetContentSetting(
                        rfh->GetLastCommittedOrigin().GetURL(),
                        rfh->GetLastCommittedOrigin().GetURL(),
                        settings_type);

  // Return all addresses if permission was previously granted
  if (setting == CONTENT_SETTING_ALLOW) {
    return addresses;
  }

  return std::vector<std::string>();
}

// static
bool WootzWalletPermissionContext::IsPermissionDenied(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin) {
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  if (!delegate) {
    return false;
  }

  return delegate->GetPermissionStatus(permission, origin.GetURL(),
                                       origin.GetURL()) ==
         blink::mojom::PermissionStatus::DENIED;
}

// static
bool WootzWalletPermissionContext::AddPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account) {
  bool has_permission;
  if (!HasPermission(permission, context, origin, account, &has_permission)) {
    return false;
  }

  if (has_permission) {
    return true;
  }

  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingTypeSafe(permission);

  url::Origin origin_wallet_address;
  if (!wootz_wallet::GetSubRequestOrigin(
          ContentSettingsTypeToRequestType(content_settings_type), origin,
          account, &origin_wallet_address)) {
    return false;
  }

  PermissionsClient::Get()
      ->GetSettingsMap(context)
      ->SetContentSettingDefaultScope(
          origin_wallet_address.GetURL(), origin_wallet_address.GetURL(),
          content_settings_type, ContentSetting::CONTENT_SETTING_ALLOW);

  return true;
}

// static
bool WootzWalletPermissionContext::HasPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account,
    bool* has_permission) {
  CHECK(has_permission);
  *has_permission = false;
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  if (!delegate) {
    return false;
  }

  if (IsPermissionDenied(permission, context, origin)) {
    return true;
  }

  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingTypeSafe(permission);

  url::Origin origin_wallet_address;
  if (!wootz_wallet::GetSubRequestOrigin(
          ContentSettingsTypeToRequestType(content_settings_type), origin,
          account, &origin_wallet_address)) {
    return false;
  }

  auto status =
      delegate->GetPermissionStatus(permission, origin_wallet_address.GetURL(),
                                    origin_wallet_address.GetURL());

  *has_permission = status == blink::mojom::PermissionStatus::GRANTED;
  return true;
}

// static
bool WootzWalletPermissionContext::ResetPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account) {
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  if (!delegate) {
    return false;
  }

  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingTypeSafe(permission);

  url::Origin origin_wallet_address;
  if (!wootz_wallet::GetSubRequestOrigin(
          ContentSettingsTypeToRequestType(content_settings_type), origin,
          account, &origin_wallet_address)) {
    return false;
  }

  delegate->ResetPermission(permission, origin_wallet_address.GetURL(),
                            origin_wallet_address.GetURL());
  return true;
}

// static
std::vector<std::string>
WootzWalletPermissionContext::GetWebSitesWithPermission(
    blink::PermissionType permission,
    content::BrowserContext* context) {
  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingTypeSafe(permission);

  HostContentSettingsMap* map =
      PermissionsClient::Get()->GetSettingsMap(context);
  ContentSettingsForOneType settings =
      map->GetSettingsForOneType(content_settings_type);

  std::vector<std::string> result;
  for (const auto& setting : settings) {
    if (setting.GetContentSetting() != CONTENT_SETTING_ALLOW) {
      continue;
    }
    result.push_back(setting.primary_pattern.ToString());
  }

  return result;
}

// static
bool WootzWalletPermissionContext::ResetWebSitePermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const std::string& formed_website) {
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  GURL url(formed_website);
  if (!delegate || !url.is_valid()) {
    return false;
  }

  delegate->ResetPermission(permission, url, url);
  return true;
}

void WootzWalletPermissionContext::ResetAllPermissions(
    content::BrowserContext* context) {
  HostContentSettingsMap* map =
      PermissionsClient::Get()->GetSettingsMap(context);

  // map->ClearSettingsForOneType(ContentSettingsType::WOOTZ_ETHEREUM);
  // map->ClearSettingsForOneType(ContentSettingsType::WOOTZ_SOLANA);
  // Get all settings for each type
  auto ethereum_settings = map->GetSettingsForOneType(ContentSettingsType::WOOTZ_ETHEREUM);
  auto solana_settings = map->GetSettingsForOneType(ContentSettingsType::WOOTZ_SOLANA);

  // Clear settings except for chrome-extension URLs
  for (const auto& setting : ethereum_settings) {
    const GURL url(setting.primary_pattern.ToString());
    if (!url.SchemeIs("chrome-extension")) {
      map->SetContentSettingDefaultScope(
          url,
          GURL(),
          ContentSettingsType::WOOTZ_ETHEREUM,
          CONTENT_SETTING_DEFAULT);
    }
  }
  for (const auto& setting : solana_settings) {
    const GURL url(setting.primary_pattern.ToString());
    if (!url.SchemeIs("chrome-extension")) {
      map->SetContentSettingDefaultScope(
          url,
          GURL(),
          ContentSettingsType::WOOTZ_SOLANA,
          CONTENT_SETTING_DEFAULT);
    }
  }
}

}  // namespace permissions

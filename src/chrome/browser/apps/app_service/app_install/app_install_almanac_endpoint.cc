// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_service/app_install/app_install_almanac_endpoint.h"

#include "base/functional/callback.h"
#include "chrome/browser/apps/almanac_api_client/almanac_api_util.h"
#include "chrome/browser/apps/almanac_api_client/device_info_manager.h"
#include "chrome/browser/apps/app_service/app_install/app_install.pb.h"
#include "chrome/browser/apps/app_service/app_install/app_install_types.h"
#include "chrome/browser/profiles/profile.h"
#include "components/services/app_service/public/cpp/package_id.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace apps::app_install_almanac_endpoint {

namespace {

constexpr char kAlmanacAppInstallEndpoint[] = "v1/app-install";

// TODO(b/307632613): Update annotations.xml and grouping.xml entries once
// update script issues are resolved.
constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("almanac_app_install", R"(
      semantics {
        sender: "App Install Service"
        description:
          "Sends a request to a Google server to fetch app data for "
          "installation."
        trigger:
          "A request is sent when an app installation is triggered by the user "
          "for apps hosted on Almanac."
        internal: {
          contacts {
            email: "cros-apps-foundation-system@google.com"
          }
        }
        user_data: {
          type: NONE
        }
        data: "Device technical specifications (e.g. model)."
        destination: GOOGLE_OWNED_SERVICE
        last_reviewed: "2023-10-24"
      }
      policy {
        cookies_allowed: NO
        setting: "This feature cannot be disabled by settings."
        policy_exception_justification:
          "This feature is required to deliver core user experiences and "
          "cannot be disabled by policy."
      }
    )");

constexpr int kMaxResponseSizeInBytes = 1024 * 1024;

std::string BuildRequestBody(const DeviceInfo& info,
                             std::string serialized_package_id) {
  proto::AppInstallRequest request_proto;
  *request_proto.mutable_device_context() = info.ToDeviceContext();
  *request_proto.mutable_user_context() = info.ToUserContext();
  *request_proto.mutable_package_id() = std::move(serialized_package_id);

  return request_proto.SerializeAsString();
}

std::optional<AppInstallData> ParseAppInstallResponseProto(
    const proto::AppInstallResponse& app_install_response) {
  if (!app_install_response.has_app_instance()) {
    return std::nullopt;
  }
  const proto::AppInstallResponse_AppInstance& instance =
      app_install_response.app_instance();

  std::optional<PackageId> package_id =
      PackageId::FromString(instance.package_id());
  if (!package_id.has_value()) {
    return std::nullopt;
  }

  AppInstallData result(std::move(package_id).value());

  if (!instance.has_name()) {
    return std::nullopt;
  }
  result.name = instance.name();

  result.description = instance.description();

  if (instance.has_icon()) {
    AppInstallIcon icon{
        .url = GURL(instance.icon().url()),
        .width_in_pixels = instance.icon().width_in_pixels(),
        .mime_type = instance.icon().mime_type(),
        .is_masking_allowed = instance.icon().is_masking_allowed()};
    if (icon.url.is_valid() && icon.width_in_pixels > 0) {
      result.icon = std::move(icon);
    }
  }

  for (const proto::AppInstallResponse_Screenshot& instance_screenshot :
       instance.screenshots()) {
    AppInstallScreenshot screenshot{
        .url = GURL(instance_screenshot.url()),
        .mime_type = instance_screenshot.mime_type(),
        .width_in_pixels = instance_screenshot.width_in_pixels(),
        .height_in_pixels = instance_screenshot.height_in_pixels(),
    };
    if (screenshot.url.is_valid()) {
      result.screenshots.push_back(std::move(screenshot));
    }
  }

  if (instance.has_install_url()) {
    result.install_url = GURL(instance.install_url());
  }

  if (instance.has_android_extras()) {
    if (result.package_id.package_type() != PackageType::kArc) {
      return std::nullopt;
    }
  } else if (instance.has_web_extras()) {
    if (result.package_id.package_type() != PackageType::kWeb) {
      return std::nullopt;
    }
    WebAppInstallData& web_app_data =
        result.app_type_data.emplace<WebAppInstallData>();
    web_app_data.document_url = GURL(instance.web_extras().document_url());
    if (!web_app_data.document_url.is_valid()) {
      return std::nullopt;
    }
    web_app_data.original_manifest_url =
        GURL(instance.web_extras().original_manifest_url());
    if (!web_app_data.original_manifest_url.is_valid()) {
      return std::nullopt;
    }
    web_app_data.proxied_manifest_url = GURL(instance.web_extras().scs_url());
    if (!web_app_data.proxied_manifest_url.is_valid()) {
      return std::nullopt;
    }
  } else if (instance.has_gfn_extras()) {
    if (result.package_id.package_type() != PackageType::kGeForceNow) {
      return std::nullopt;
    }
  } else if (instance.has_steam_extras()) {
    if (result.package_id.package_type() != PackageType::kBorealis) {
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }

  return result;
}

base::expected<AppInstallData, QueryError> ConvertAppInstallResponseProto(
    base::expected<proto::AppInstallResponse, QueryError> query_response) {
  if (!query_response.has_value()) {
    return base::unexpected(std::move(query_response).error());
  }

  std::optional<AppInstallData> data =
      ParseAppInstallResponseProto(query_response.value());
  if (!data.has_value()) {
    return base::unexpected(
        QueryError{QueryError::kBadResponse, "Failed to convert proto"});
  }

  return base::ok(std::move(data).value());
}

base::expected<GURL, QueryError> ExtractAppInstallUrlFromResponseProto(
    base::expected<proto::AppInstallResponse, QueryError> query_response) {
  if (!query_response.has_value()) {
    return base::unexpected(std::move(query_response).error());
  }

  GURL result = GURL(query_response.value().app_instance().install_url());
  if (!result.is_valid()) {
    return base::unexpected(
        QueryError{QueryError::kBadResponse, "Failed to convert install URL"});
  }

  return base::ok(std::move(result));
}

}  // namespace

GURL GetEndpointUrlForTesting() {
  return GetAlmanacEndpointUrl(kAlmanacAppInstallEndpoint);
}

void GetAppInstallInfo(
    PackageId package_id,
    DeviceInfo device_info,
    network::mojom::URLLoaderFactory& url_loader_factory,
    GetAppInstallInfoCallback callback) {
  QueryAlmanacApi<proto::AppInstallResponse>(
      url_loader_factory, kTrafficAnnotation,
      BuildRequestBody(device_info, package_id.ToString()),
      kAlmanacAppInstallEndpoint, kMaxResponseSizeInBytes,
      /*error_histogram_name=*/std::nullopt,
      base::BindOnce(&ConvertAppInstallResponseProto)
          .Then(std::move(callback)));
}

using GetAppInstallUrlCallback =
    base::OnceCallback<void(base::expected<GURL, QueryError>)>;
void GetAppInstallUrl(std::string serialized_package_id,
                      DeviceInfo device_info,
                      network::mojom::URLLoaderFactory& url_loader_factory,
                      GetAppInstallUrlCallback callback) {
  QueryAlmanacApi<proto::AppInstallResponse>(
      url_loader_factory, kTrafficAnnotation,
      BuildRequestBody(device_info, std::move(serialized_package_id)),
      kAlmanacAppInstallEndpoint, kMaxResponseSizeInBytes,
      /*error_histogram_name=*/std::nullopt,
      base::BindOnce(&ExtractAppInstallUrlFromResponseProto)
          .Then(std::move(callback)));
}

}  // namespace apps::app_install_almanac_endpoint

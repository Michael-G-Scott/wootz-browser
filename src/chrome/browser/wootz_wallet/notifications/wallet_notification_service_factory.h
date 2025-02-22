/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef CHROME_BROWSER_WOOTZ_WALLET_NOTIFICATIONS_WALLET_NOTIFICATION_SERVICE_FACTORY_H_
#define CHROME_BROWSER_WOOTZ_WALLET_NOTIFICATIONS_WALLET_NOTIFICATION_SERVICE_FACTORY_H_

#include "chrome/browser/wootz_wallet/notifications/wallet_notification_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace wootz_wallet {

// Singleton that owns all WalletNotificationService and associates them with
// BrowserContext.
class WalletNotificationServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  WalletNotificationServiceFactory(const WalletNotificationServiceFactory&) =
      delete;
  WalletNotificationServiceFactory& operator=(
      const WalletNotificationServiceFactory&) = delete;

  static WalletNotificationServiceFactory* GetInstance();
  static WalletNotificationService* GetServiceForContext(
      content::BrowserContext* context);

 private:
  friend base::NoDestructor<WalletNotificationServiceFactory>;

  WalletNotificationServiceFactory();
  ~WalletNotificationServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace wootz_wallet

#endif  // CHROME_BROWSER_WOOTZ_WALLET_NOTIFICATIONS_WALLET_NOTIFICATION_SERVICE_FACTORY_H_

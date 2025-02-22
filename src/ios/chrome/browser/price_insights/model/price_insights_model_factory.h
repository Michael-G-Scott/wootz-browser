// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PRICE_INSIGHTS_MODEL_PRICE_INSIGHTS_MODEL_FACTORY_H_
#define IOS_CHROME_BROWSER_PRICE_INSIGHTS_MODEL_PRICE_INSIGHTS_MODEL_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ChromeBrowserState;
class PriceInsightsModel;

// Singleton that owns all PriceInsightsModels and associates them with
// BrowserStates.
class PriceInsightsModelFactory : public BrowserStateKeyedServiceFactory {
 public:
  static PriceInsightsModel* GetForBrowserState(
      ChromeBrowserState* browser_state);

  static PriceInsightsModelFactory* GetInstance();

  PriceInsightsModelFactory(const PriceInsightsModelFactory&) = delete;
  PriceInsightsModelFactory& operator=(const PriceInsightsModelFactory&) =
      delete;

 private:
  friend class base::NoDestructor<PriceInsightsModelFactory>;

  PriceInsightsModelFactory();
  ~PriceInsightsModelFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
};

#endif  // IOS_CHROME_BROWSER_PRICE_INSIGHTS_MODEL_PRICE_INSIGHTS_MODEL_FACTORY_H_

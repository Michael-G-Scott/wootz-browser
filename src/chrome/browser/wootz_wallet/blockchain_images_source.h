/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef CHROME_BROWSER_WOOTZ_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_
#define CHROME_BROWSER_WOOTZ_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_

#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/url_data_source.h"

namespace base {
class FilePath;
}  // namespace base

namespace wootz_wallet {

// This serves background image data.
class BlockchainImagesSource : public content::URLDataSource {
 public:
  explicit BlockchainImagesSource(const base::FilePath& base_path);

  ~BlockchainImagesSource() override;

  BlockchainImagesSource(const BlockchainImagesSource&) = delete;
  BlockchainImagesSource& operator=(const BlockchainImagesSource&) = delete;
  std::string getImagePath();

 private:
  FRIEND_TEST_ALL_PREFIXES(BlockchainImagesSourceTest, GetMimeType);
  friend class BlockchainImagesSourceTest;

  // content::URLDataSource overrides:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;

  void OnGotImageFile(GotDataCallback callback,
                      std::optional<std::string> input);

  base::FilePath base_path_;
  base::WeakPtrFactory<BlockchainImagesSource> weak_factory_;
};

}  // namespace wootz_wallet

#endif  // CHROME_BROWSER_WOOTZ_WALLET_BLOCKCHAIN_IMAGES_SOURCE_H_

/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_WOOTZ_COMPONENT_UPDATER_BROWSER_LOCAL_DATA_FILES_SERVICE_H_
#define COMPONENTS_WOOTZ_COMPONENT_UPDATER_BROWSER_LOCAL_DATA_FILES_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "components/wootz_component_updater/browser/wootz_component.h"

namespace wootz_component_updater {

class LocalDataFilesObserver;

const char kLocalDataFilesComponentName[] = "Wootz Local Data Updater";
const char kLocalDataFilesComponentId[] = "afalakplffnnnlkncjhbmahjfjhmlkal";
const char kLocalDataFilesComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAs4TIQXRCftLpGmQZxmm6"
    "AU8pqGKLoDyi537HGQyRKcK7j/CSXCf3vwJr7xkV72p7bayutuzyNZ3740QxBPie"
    "sfBOp8bBb8d2VgTHP3b+SuNmK/rsSRsMRhT05x8AAr/7ab6U3rW0Gsalm2653xnn"
    "QS8vt0s62xQTmC+UMXowaSLUZ0Be/TOu6lHZhOeo0NBMKc6PkOu0R1EEfP7dJR6S"
    "M/v4dBUBZ1HXcuziVbCXVyU51opZCMjlxyUlQR9pTGk+Zh5sDn1Vw1MwLnWiEfQ4"
    "EGL1V7GeI4vgLoOLgq7tmhEratHGCfC1IHm9luMACRr/ybMI6DQJOvgBvecb292F"
    "xQIDAQAB";

// The component in charge of delegating access to different DAT files
// such as tracking protection.
class LocalDataFilesService : public WootzComponent {
 public:
  explicit LocalDataFilesService(WootzComponent::Delegate* delegate);
  LocalDataFilesService(const LocalDataFilesService&) = delete;
  LocalDataFilesService& operator=(const LocalDataFilesService&) = delete;
  ~LocalDataFilesService() override;
  bool Start();
  bool IsInitialized() const { return initialized_; }
  void AddObserver(LocalDataFilesObserver* observer);
  void RemoveObserver(LocalDataFilesObserver* observer);

  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);

 protected:
  void OnComponentReady(const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;

 private:
  static std::string g_local_data_files_component_id_;
  static std::string g_local_data_files_component_base64_public_key_;

  bool initialized_;
  base::ObserverList<LocalDataFilesObserver>::Unchecked observers_;
};

// Creates the LocalDataFilesService
std::unique_ptr<LocalDataFilesService>
LocalDataFilesServiceFactory(WootzComponent::Delegate* delegate);

}  // namespace wootz_component_updater

#endif  // COMPONENTS_WOOTZ_COMPONENT_UPDATER_BROWSER_LOCAL_DATA_FILES_SERVICE_H_

// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/updater/win/ui/l10n_util.h"

#include <string>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/win/atl.h"
#include "base/win/embedded_i18n/language_selector.h"
#include "base/win/i18n.h"
#include "chrome/updater/util/util.h"
#include "chrome/updater/win/installer/exit_code.h"
#include "chrome/updater/win/ui/resources/updater_installer_strings.h"

namespace updater {
namespace {

constexpr base::win::i18n::LanguageSelector::LangToOffset
    kLanguageOffsetPairs[] = {
#define HANDLE_LANGUAGE(l_, o_) {L## #l_, o_},
        DO_LANGUAGES
#undef HANDLE_LANGUAGE
};

size_t GetLanguageOffset(const std::wstring& lang) {
  return base::win::i18n::LanguageSelector(lang, kLanguageOffsetPairs).offset();
}

}  // namespace

std::wstring GetPreferredLanguage() {
  std::vector<std::wstring> languages;
  if (!base::win::i18n::GetUserPreferredUILanguageList(&languages) ||
      languages.size() == 0) {
    return L"en-us";
  }

  return languages[0];
}

std::wstring GetLocalizedString(UINT base_message_id,
                                const std::wstring& lang) {
  // Map `base_message_id` to the base id for the current install mode.
  UINT message_id =
      static_cast<UINT>(base_message_id + GetLanguageOffset(lang));
  const ATLSTRINGRESOURCEIMAGE* image =
      AtlGetStringResourceImage(_AtlBaseModule.GetModuleInstance(), message_id);
  if (image) {
    return std::wstring(image->achString, image->nLength);
  }
  NOTREACHED_IN_MIGRATION() << "Unable to find resource id " << message_id;
  return std::wstring();
}

std::wstring GetLocalizedStringF(UINT base_message_id,
                                 const std::wstring& replacement,
                                 const std::wstring& lang) {
  return GetLocalizedStringF(base_message_id,
                             std::vector<std::wstring>{replacement}, lang);
}

std::wstring GetLocalizedStringF(UINT base_message_id,
                                 std::vector<std::wstring> replacements,
                                 const std::wstring& lang) {
  // Replacements start at index 1 because the implementation of
  // ReplaceStringPlaceholders does i+1, so the first placeholder would be `$1`.
  // A `$0` is considered an invalid placeholder.
  replacements.insert(replacements.begin(), {});
  return base::ReplaceStringPlaceholders(
      GetLocalizedString(base_message_id, lang), replacements, nullptr);
}

std::wstring GetLocalizedErrorString(DWORD exit_code) {
  switch (exit_code) {
    case updater::UNSUPPORTED_WINDOWS_VERSION:
      return GetLocalizedString(IDS_INSTALL_OS_NOT_SUPPORTED_BASE);
    default:
      std::wstring error = L"Updater error ";
      error.append(std::to_wstring(exit_code));
      return error;
  }
}

}  // namespace updater

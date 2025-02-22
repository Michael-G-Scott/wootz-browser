/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/wootz_component_updater/browser/dat_file_util.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"

namespace {

void GetDATFileData(const base::FilePath& file_path,
                    wootz_component_updater::DATFileDataBuffer* buffer) {
  int64_t size = 0;
  if (!base::PathExists(file_path) ||
      !base::GetFileSize(file_path, &size) ||
      0 == size) {
    LOG(ERROR) << "GetDATFileData: "
               << "the dat file is not found or corrupted "
               << file_path;
    return;
  }

  buffer->resize(size);
  if (size != base::ReadFile(file_path,
                             reinterpret_cast<char*>(&buffer->front()),
                             size)) {
    LOG(ERROR) << "GetDATFileData: cannot "
               << "read dat file " << file_path;
  }
}

}  // namespace

namespace wootz_component_updater {

DATFileDataBuffer ReadDATFileData(const base::FilePath& dat_file_path) {
  // TRACE_EVENT_BEGIN1("wootz.adblock", "ReadDATFileData", "path",
  //                    dat_file_path.MaybeAsASCII());
  DATFileDataBuffer buffer;
  GetDATFileData(dat_file_path, &buffer);
  // TRACE_EVENT_END1("wootz.adblock", "ReadDATFileData", "size", buffer.size());
  return buffer;
}

std::string GetDATFileAsString(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    LOG(ERROR) << "GetDATFileAsString: cannot "
               << "read dat file " << file_path;
  }
  return contents;
}

}  // namespace wootz_component_updater

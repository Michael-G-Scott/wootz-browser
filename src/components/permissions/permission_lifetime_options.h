/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_OPTIONS_H_
#define COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_OPTIONS_H_

#include <optional>
#include <vector>

#include "base/time/time.h"

namespace permissions {

struct PermissionLifetimeOption {
  PermissionLifetimeOption(std::u16string label,
                           std::optional<base::TimeDelta> lifetime);
  PermissionLifetimeOption(const PermissionLifetimeOption&);
  PermissionLifetimeOption& operator=(const PermissionLifetimeOption&);
  PermissionLifetimeOption(PermissionLifetimeOption&&) noexcept;
  PermissionLifetimeOption& operator=(PermissionLifetimeOption&&) noexcept;
  ~PermissionLifetimeOption();

  // Text visible to the user.
  std::u16string label;
  // If not set, lifetime will not be controlled (i.e. permanent). If set to
  // base::TimeDelta(), permission should be alive until eTLD+1 is closed.
  std::optional<base::TimeDelta> lifetime;
};

}  // namespace permissions

#endif  // COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_OPTIONS_H_

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_MAHI_MAHI_ERROR_STATUS_VIEW_H_
#define ASH_SYSTEM_MAHI_MAHI_ERROR_STATUS_VIEW_H_

#include "ash/system/mahi/mahi_ui_controller.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/metadata/view_factory.h"

namespace chromeos {
enum class MahiResponseStatus;
}  // namespace chromeos

namespace views {
class View;
}  // namespace views

namespace ash {

enum class VisibilityState;

// Presents the current Mahi error if any. It should show when the UI controller
// is in the error state. NOTE:
// 1. This class is created only when the Mahi feature is enabled.
// 2. `chromeos::MahiResponseStatus::kLowQuota` is presented in a toast view
//    instead of this class.
class MahiErrorStatusView : public views::FlexLayoutView,
                            public MahiUiController::Delegate {
  METADATA_HEADER(MahiErrorStatusView, views::View)

 public:
  explicit MahiErrorStatusView(MahiUiController* ui_controller);
  MahiErrorStatusView(const MahiErrorStatusView&) = delete;
  MahiErrorStatusView& operator=(const MahiErrorStatusView&) = delete;
  ~MahiErrorStatusView() override;

 private:
  // MahiUiController::Delegate:
  views::View* GetView() override;
  bool GetViewVisibility(VisibilityState state) const override;
};

BEGIN_VIEW_BUILDER(/*no export*/, MahiErrorStatusView, views::FlexLayoutView)
END_VIEW_BUILDER

}  // namespace ash

DEFINE_VIEW_BUILDER(/*no export*/, ash::MahiErrorStatusView)

#endif  // ASH_SYSTEM_MAHI_MAHI_ERROR_STATUS_VIEW_H_

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_DESKS_DESK_ACTION_BUTTON_H_
#define ASH_WM_DESKS_DESK_ACTION_BUTTON_H_

#include "ash/ash_export.h"
#include "ash/style/close_button.h"
#include "ash/wm/overview/overview_focusable_view.h"
#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace ash {

class DeskActionView;

// This class defines desk action buttons under the desk action view. It can
// represent either the combine desk button or the close desk button. It
// implements `OverviewFocusableView` interface as well as `ui::View` interface
// so that it supports focus inside and outside of overview.
class ASH_EXPORT DeskActionButton : public CloseButton,
                                    public OverviewFocusableView {
  METADATA_HEADER(DeskActionButton, CloseButton)

 public:
  enum class Type {
    kCombineDesk,
    kCloseDesk,
  };

  DeskActionButton(const std::u16string& tooltip,
                   Type type,
                   base::RepeatingClosure pressed_callback,
                   DeskActionView* desk_action_view);
  DeskActionButton(const DeskActionButton&) = delete;
  DeskActionButton& operator=(const DeskActionButton&) = delete;
  ~DeskActionButton() override;

  // OverviewFocusableView:
  views::View* GetView() override;
  void MaybeActivateFocusedView() override;
  void MaybeCloseFocusedView(bool primary_action) override;
  void MaybeSwapFocusedView(bool right) override;
  void OnFocusableViewFocused() override;
  void OnFocusableViewBlurred() override;

  // Returns true if the button can show given the virtual desk status.
  bool CanShow() const;

  // Updates the tooltip for the button.
  void UpdateTooltip(const std::u16string& tooltip);

 private:
  Type type_;
  base::RepeatingClosure pressed_callback_;
  raw_ptr<DeskActionView> desk_action_view_;
};

}  // namespace ash

#endif  // ASH_WM_DESKS_DESK_ACTION_BUTTON_H_

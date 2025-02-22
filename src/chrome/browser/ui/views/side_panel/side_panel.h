// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_
#define CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "components/prefs/pref_change_registrar.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/accessible_pane_view.h"
#include "ui/views/animation/animation_delegate_views.h"
#include "ui/views/controls/resize_area_delegate.h"

class BrowserView;

class SidePanel : public views::AccessiblePaneView,
                  public views::ResizeAreaDelegate,
                  public views::AnimationDelegateViews {
  METADATA_HEADER(SidePanel, views::AccessiblePaneView)

 public:
  // Determines the side from which the side panel will appear.
  // LTR / RTL conversions are handled in
  // BrowserViewLayout::LayoutSidePanelView. As such, left will always be on the
  // left side of the browser regardless of LTR / RTL mode.
  enum HorizontalAlignment { kAlignLeft = 0, kAlignRight };
  explicit SidePanel(BrowserView* browser_view,
                     HorizontalAlignment horizontal_alignment =
                         HorizontalAlignment::kAlignRight);
  SidePanel(const SidePanel&) = delete;
  SidePanel& operator=(const SidePanel&) = delete;
  ~SidePanel() override;

  void SetPanelWidth(int width);
  double GetAnimationValue() const;
  gfx::RoundedCornersF background_radii() const { return background_radii_; }
  void SetBackgroundRadii(const gfx::RoundedCornersF& radii);
  void SetHorizontalAlignment(HorizontalAlignment alignment);
  HorizontalAlignment GetHorizontalAlignment();
  bool IsRightAligned();
  gfx::Size GetMinimumSize() const override;
  bool IsClosing();
  void DisableAnimationsForTesting() { animations_disabled_ = true; }

  // Add a header view that gets painted over the side panel border. The top
  // border area grows to accommodate the additional height of the header,
  // pushing the other side panel content down.
  void AddHeaderView(std::unique_ptr<views::View> view);

  // Gets the upper bound of the content area size if the side panel is shown
  // right now. If the side panel is not showing, returns the minimum width
  // and browser view height minus the padding insets. The actual content
  // size will be smaller than the returned result when the side panel header
  // is shown, for example.
  gfx::Size GetContentSizeUpperBound() const;

  // views::ResizeAreaDelegate:
  void OnResize(int resize_amount, bool done_resizing) override;

  // Log UMA data for the side panel resize feature. Will only log if the side
  // panel has been resized since metrics were last logged.
  void RecordMetricsIfResized();

 private:
  void UpdateVisibility();
  bool ShouldShowAnimation() const;

  // views::View:
  void ChildVisibilityChanged(View* child) override;

  // views::ViewObserver:
  void OnChildViewAdded(View* observed_view, View* child) override;
  void OnChildViewRemoved(View* observed_view, View* child) override;
  void OnViewPropertyChanged(View* observed_view,
                             const void* key,
                             int64_t old_value) override;

  // views::AnimationDelegateViews:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  // Timestamp of the last step in the side panel open/close animation. This is
  // used for metrics purposes.
  base::TimeTicks last_animation_step_timestamp_;
  std::optional<base::TimeDelta> largest_animation_step_time_;

  raw_ptr<View> border_view_ = nullptr;
  const raw_ptr<BrowserView> browser_view_;
  raw_ptr<View> resize_area_ = nullptr;
  raw_ptr<views::View> header_view_ = nullptr;

  // -1 if a side panel resize is not in progress, otherwise the width of the
  // side panel when the current resize was initiated.
  int starting_width_on_resize_ = -1;

  // Should be true if the side panel was resized since metrics were last
  // logged.
  bool did_resize_ = false;

  bool animations_disabled_ = false;

  // Animation controlling showing and hiding of the side panel.
  gfx::SlideAnimation animation_{this};
  // Monitors content views so we will be notified if their property
  // state changes.
  base::ScopedMultiSourceObservation<View, ViewObserver>
      content_view_observations_{this};

  gfx::RoundedCornersF background_radii_;

  // Keeps track of the side the side panel will appear on (left or right).
  HorizontalAlignment horizontal_alignment_;

  // Observes and listens to side panel alignment changes.
  PrefChangeRegistrar pref_change_registrar_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {EventGenerator} from '../../event_generator.js';

import {CheckContextResult, Macro, MacroError, RunMacroResult} from './macro.js';
import {MacroName} from './macro_names.js';

import ScreenPoint = chrome.accessibilityPrivate.ScreenPoint;
import SyntheticMouseEventButton = chrome.accessibilityPrivate.SyntheticMouseEventButton;

/**
 * Class that implements a macro to send a synthetic mouse click.
 */
export class MouseClickMacro extends Macro {
  private leftClick_: boolean;
  private location_: ScreenPoint|undefined;
  private runCount_ = 0;

  /**
   * Pass the location in density-independent pixels. Defaults to left click.
   */
  constructor(location: ScreenPoint|undefined, leftClick = true) {
    super(leftClick ? MacroName.MOUSE_CLICK_LEFT : MacroName.MOUSE_CLICK_RIGHT);
    this.leftClick_ = leftClick;
    this.location_ = location;
  }

  /**
   * The mouse click macro should be run twice, once when the click begins and
   * again when it ends.
   */
  override triggersAtActionStartAndEnd(): boolean {
    return true;
  }

  /** Invalid context if location isn't set. */
  override checkContext(): CheckContextResult {
    if (!this.location_) {
      return this.createFailureCheckContextResult_(MacroError.BAD_CONTEXT);
    } else if (this.runCount_ > 2) {
      return this.createFailureCheckContextResult_(
          MacroError.INVALID_USER_INTENT);
    } else {
      return this.createSuccessCheckContextResult_();
    }
  }

  updateLocation(location?: ScreenPoint): void {
    this.location_ = location;
  }

  override run(): RunMacroResult {
    if (!this.location_ || this.runCount_ > 2) {
      return this.createRunMacroResult_(/*isSuccess=*/ false);
    }
    const mouseButton = this.leftClick_ ? SyntheticMouseEventButton.LEFT :
                                          SyntheticMouseEventButton.RIGHT;
    if (this.runCount_ === 0) {
      EventGenerator.sendMousePress(
          this.location_.x, this.location_.y, mouseButton);
    } else if (this.runCount_ === 1) {
      EventGenerator.sendMouseRelease(this.location_.x, this.location_.y);
    }
    this.runCount_++;
    return this.createRunMacroResult_(/*isSuccess=*/ true);
  }
}

// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ui.appmenu;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SoundEffectConstants;
import android.view.View;
import android.view.View.AccessibilityDelegate;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import android.util.Log;

import org.chromium.base.metrics.RecordUserAction;

/**
 * A helper class for a menu button to decide when to show the app menu.
 *
 * Simply construct this class and pass the class instance to a menu button as TouchListener.
 * Then this class will handle everything regarding showing app menu for you.
 */
class AppMenuButtonHelperImpl extends AccessibilityDelegate implements AppMenuButtonHelper {
    private final AppMenuHandlerImpl mMenuHandler;
    private Runnable mOnAppMenuShownListener;
    private boolean mIsTouchEventsBeingProcessed;
    private Runnable mOnClickRunnable;

    /**
     * @param menuHandler MenuHandler implementation that can show and get the app menu.
     */
    AppMenuButtonHelperImpl(AppMenuHandlerImpl menuHandler) {
        mMenuHandler = menuHandler;
    }

    // AppMenuButtonHelper implementation.

    @Override
    public void setOnAppMenuShownListener(Runnable onAppMenuShownListener) {
        mOnAppMenuShownListener = onAppMenuShownListener;
    }

    @Override
    public boolean isAppMenuActive() {
        return mIsTouchEventsBeingProcessed || mMenuHandler.isAppMenuShowing();
    }

    @Override
    public boolean onEnterKeyPress(View view) {
        return showAppMenu(view);
    }

    @Override
    public AccessibilityDelegate getAccessibilityDelegate() {
        return this;
    }

    @Override
    public void setOnClickRunnable(Runnable clickRunnable) {
        mOnClickRunnable = clickRunnable;
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouch(View view, MotionEvent event) {
        boolean isTouchEventConsumed = false;

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                isTouchEventConsumed = true;
                updateTouchEvent(view, true);
                break;
            case MotionEvent.ACTION_UP:
                isTouchEventConsumed = true;
                updateTouchEvent(view, false);
                if (mOnClickRunnable != null) mOnClickRunnable.run();
                showAppMenu(view);
                break;
            case MotionEvent.ACTION_CANCEL:
                isTouchEventConsumed = true;
                updateTouchEvent(view, false);
                break;
            default:
        }

        return isTouchEventConsumed;
    }

    // AccessibilityDelegate overrides

    @Override
    public boolean performAccessibilityAction(View host, int action, Bundle args) {
        if (action == AccessibilityNodeInfo.ACTION_CLICK) {
            if (!mMenuHandler.isAppMenuShowing()) {
                showAppMenu(host);
            } else {
                mMenuHandler.hideAppMenu();
            }
            host.playSoundEffect(SoundEffectConstants.CLICK);
            host.sendAccessibilityEvent(AccessibilityEvent.TYPE_VIEW_CLICKED);
            return true;
        }
        return super.performAccessibilityAction(host, action, args);
    }

    /**
     * Shows the app menu if it is not already shown.
     * @param view View that initiated showing this menu. Normally it is a menu button.
     * @return Whether or not if the app menu is successfully shown.
     */
    private boolean showAppMenu(View view) {
        if (!mMenuHandler.isAppMenuShowing() && mMenuHandler.showAppMenu(view, false)) {
            RecordUserAction.record("MobileUsingMenuBySwButtonTap");

            if (mOnAppMenuShownListener != null) {
                mOnAppMenuShownListener.run();
            }
            Log.d("touched", "From AppMenuButtonHelperImpl, If you get this that means it will always be called.");
            return true;
        }
        return false;
    }

    /**
     * Set whether touch event is being processed and view is pressed on touch event.
     * @param view View that received a touch event.
     * @param isActionDown Whether the touch event is a down action.
     */
    private void updateTouchEvent(View view, boolean isActionDown) {
        mIsTouchEventsBeingProcessed = isActionDown;
        view.setPressed(isActionDown);
    }
}
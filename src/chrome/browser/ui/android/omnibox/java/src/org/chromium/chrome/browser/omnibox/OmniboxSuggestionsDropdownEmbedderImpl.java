// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.content.ComponentCallbacks;
import android.content.Context;
import android.content.res.Configuration;
import android.view.View;
import android.view.View.OnLayoutChangeListener;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.WindowInsets;
import androidx.core.view.ViewCompat;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.Insets;
import androidx.core.view.WindowInsetsCompat;

import java.util.function.Supplier;

import javax.swing.ViewportLayout;

import org.chromium.base.BuildInfo;
import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.browser.omnibox.styles.OmniboxResourceProvider;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionsDropdown;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionsDropdownEmbedder;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionsDropdownEmbedder.OmniboxAlignment;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.base.WindowDelegate;
import org.chromium.ui.display.DisplayUtil;
import android.util.Log;
import org.chromium.ui.KeyboardVisibilityDelegate;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import org.chromium.base.ContextUtils;
import android.graphics.Rect;

/**
 * Implementation of {@link OmniboxSuggestionsDropdownEmbedder} that positions it using an "anchor"
 * and "horizontal alignment" view.
 */
public class OmniboxSuggestionsDropdownEmbedderImpl
        implements OmniboxSuggestionsDropdownEmbedder,
                OnLayoutChangeListener,
                OnGlobalLayoutListener,
                ComponentCallbacks {
    private final ObservableSupplierImpl<OmniboxAlignment> mOmniboxAlignmentSupplier =
            new ObservableSupplierImpl<>();
    private final @NonNull WindowAndroid mWindowAndroid;
    private final @NonNull WindowDelegate mWindowDelegate;
    private final @NonNull View mAnchorView;
    private final @NonNull View mAlignmentView;
    private final boolean mForcePhoneStyleOmnibox;
    private final @NonNull Context mContext;
    // Reusable int array to pass to positioning methods that operate on a two element int array.
    // Keeping it as a member lets us avoid allocating a temp array every time.
    private final int[] mPositionArray = new int[2];
    private int mVerticalOffsetInWindow;
    private int mWindowWidthDp;
    private int mWindowHeightDp;
    private WindowInsetsCompat mWindowInsetsCompat;
    private DeferredIMEWindowInsetApplicationCallback mDeferredIMEWindowInsetApplicationCallback;
    private @Nullable View mBaseChromeLayout;
    private KeyboardVisibilityDelegate mKeyboardVisibilityDelegate;
    private ValueAnimator mPaddingAnimator;
    private int mCurrentBottomPadding;
    private View mContentView;
    private boolean isKeyboardShowing;
    private int mKeyboardHeight;
    private boolean isWhitePatchVisible;
    private KeyboardVisibilityDelegate.KeyboardVisibilityListener mKeyboardVisibilityListener;
    private @Nullable View mCompositorViewHolder;
    /**
     * @param windowAndroid Window object in which the dropdown will be displayed.
     * @param windowDelegate Delegate object for performing window operations.
     * @param anchorView View to which the dropdown should be "anchored" i.e. vertically positioned
     *     next to and matching the width of. This must be a descendant of the top-level content
     *     (android.R.id.content) view.
     * @param alignmentView View to which: 1. The dropdown should be horizontally aligned to when
     *     its width is smaller than the anchor view. 2. The dropdown should vertically align to
     *     during animations. This must be a descendant of the anchor view.
     * @param baseChromeLayout The base view hosting Chrome that certain views (e.g. the omnibox
     *     suggestion list) will position themselves relative to. If null, the content view will be
     *     used.
     */
    OmniboxSuggestionsDropdownEmbedderImpl(
            @NonNull WindowAndroid windowAndroid,
            @NonNull WindowDelegate windowDelegate,
            @NonNull View anchorView,
            @NonNull View alignmentView,
            boolean forcePhoneStyleOmnibox,
            @Nullable View baseChromeLayout,
            @Nullable View compositorViewHolder
            ) {
        mWindowAndroid = windowAndroid;
        mWindowDelegate = windowDelegate;
        mAnchorView = anchorView;
        mAlignmentView = alignmentView;
        mForcePhoneStyleOmnibox = forcePhoneStyleOmnibox;
        mContext = mAnchorView.getContext();
        mContext.registerComponentCallbacks(this);
        Configuration configuration = mContext.getResources().getConfiguration();
        mWindowWidthDp = configuration.smallestScreenWidthDp;
        mWindowHeightDp = configuration.screenHeightDp;
        mBaseChromeLayout = baseChromeLayout;
        mCompositorViewHolder = compositorViewHolder;
        mKeyboardVisibilityDelegate = KeyboardVisibilityDelegate.getInstance();
        mCurrentBottomPadding = 0;
        mContentView = mAnchorView.getRootView().findViewById(android.R.id.content);
        isKeyboardShowing = false;
        mKeyboardHeight = 0;
        isWhitePatchVisible = true;
        mKeyboardVisibilityListener = (isShowing)-> {
            Log.d("Omni", "keyboard " + isShowing);
            if(isShowing){
                mKeyboardHeight = mKeyboardVisibilityDelegate.calculateKeyboardHeight(mCompositorViewHolder.getRootView());
                Log.d("Omni", "keyboardHeight " + mKeyboardHeight);
                recalculateOmniboxAlignment();
            }
            else{
                mKeyboardHeight = 0;
                Log.d("Omni", "keyboardHeight " + mKeyboardHeight);
                recalculateOmniboxAlignment();
            }
        };
        setupWindowAndroidKeyboardDelegate();
        recalculateOmniboxAlignment();
    }
    private void setupWindowAndroidKeyboardDelegate(){
        if(mWindowAndroid != null){
            mWindowAndroid.getKeyboardDelegate().addKeyboardVisibilityListener(mKeyboardVisibilityListener);
        }
    }
    private void onKeyboardVisibilityChanged(boolean opened) {
        // Log.d("Omni", "keyboard " + opened);
        recalculateOmniboxAlignment();
    }

    @Override
    public OmniboxAlignment addAlignmentObserver(Callback<OmniboxAlignment> obs) {
        return mOmniboxAlignmentSupplier.addObserver(obs);
    }

    @Override
    public void removeAlignmentObserver(Callback<OmniboxAlignment> obs) {
        mOmniboxAlignmentSupplier.removeObserver(obs);
    }
    @Override
    public View getAnchorView() {
        return mAnchorView;
    }
    @Nullable
    @Override
    public OmniboxAlignment getCurrentAlignment() {
        OmniboxAlignment currentAlignment = mOmniboxAlignmentSupplier.get();
        if (currentAlignment == null) {
            // Provide default alignment if none exists
            return new OmniboxAlignment(
                0,  // left
                0,  // top
                mAnchorView.getMeasuredWidth(),  // width
                mAnchorView.getMeasuredHeight(), // height
                0,  // paddingLeft
                0   // paddingRight
            );
        }
        return currentAlignment;
    }

    public void setWhitePatchVisible(boolean visible) {
        isWhitePatchVisible = visible;
        recalculateOmniboxAlignment();
    }

    public boolean isWhitePatchVisible() {
        return isWhitePatchVisible;
    }

    @Override
    public boolean isTablet() {
        if (mForcePhoneStyleOmnibox) return false;
        return mWindowWidthDp >= DeviceFormFactor.MINIMUM_TABLET_WIDTH_DP
                && DeviceFormFactor.isWindowOnTablet(mWindowAndroid);
    }

    public int calculateKeyboardHeightFromWindowInsets(View rootView) {

            if (rootView == null || rootView.getRootWindowInsets() == null) return 0;
            WindowInsetsCompat windowInsetsCompat =
                    WindowInsetsCompat.toWindowInsetsCompat(
                            rootView.getRootWindowInsets(), rootView);
            int imeHeightIncludingSystemBars =
                    windowInsetsCompat.getInsets(WindowInsetsCompat.Type.ime()).bottom;
            if (imeHeightIncludingSystemBars == 0) return 0;
            int bottomSystemBarsHeight =
                    windowInsetsCompat.getInsets(WindowInsetsCompat.Type.systemBars()).bottom;
            return imeHeightIncludingSystemBars - bottomSystemBarsHeight;
    }

    @Override
    public void onAttachedToWindow() {
        mAnchorView.addOnLayoutChangeListener(this);
        mAlignmentView.addOnLayoutChangeListener(this);
        mAnchorView.getViewTreeObserver().addOnGlobalLayoutListener(this);
        mDeferredIMEWindowInsetApplicationCallback =
                new DeferredIMEWindowInsetApplicationCallback(this::recalculateOmniboxAlignment);
        mDeferredIMEWindowInsetApplicationCallback.attach(mWindowAndroid);
        onConfigurationChanged(mContext.getResources().getConfiguration());
        recalculateOmniboxAlignment();
    }

    @Override
    public void onDetachedFromWindow() {
        // Reset padding when detached
        View contentView = mAnchorView.getRootView().findViewById(android.R.id.content);
        if (contentView != null) {
            ViewCompat.setPaddingRelative(contentView, 0, 0, 0, 0);
        }
        mKeyboardHeight = 0;
        recalculateOmniboxAlignment();
        mAnchorView.removeOnLayoutChangeListener(this);
        mAlignmentView.removeOnLayoutChangeListener(this);
        mAnchorView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
        if (mDeferredIMEWindowInsetApplicationCallback != null) {
            mDeferredIMEWindowInsetApplicationCallback.detach();
            mDeferredIMEWindowInsetApplicationCallback = null;
        }
    }

    @Override
    public @NonNull WindowDelegate getWindowDelegate() {
        return mWindowDelegate;
    }

    // View.OnLayoutChangeListener
    @Override
    public void onLayoutChange(
            View v,
            int left,
            int top,
            int right,
            int bottom,
            int oldLeft,
            int oldTop,
            int oldRight,
            int oldBottom) {
        recalculateOmniboxAlignment();
    }


    // OnGlobalLayoutListener
    @Override
    public void onGlobalLayout() {
        Log.d("Omni", "onGlobalLayout");
        if (offsetInWindowChanged(mAnchorView) || insetsHaveChanged(mAnchorView)) {
            mKeyboardHeight = mKeyboardVisibilityDelegate.calculateKeyboardHeight(mCompositorViewHolder.getRootView());
            recalculateOmniboxAlignment();
        }

    }

    // ComponentCallbacks
    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        int windowWidth = newConfig.screenWidthDp;
        int windowHeight = newConfig.screenHeightDp;
        if (windowWidth == mWindowWidthDp && mWindowHeightDp == windowHeight) return;
        mWindowWidthDp = windowWidth;
        mWindowHeightDp = windowHeight;

        recalculateOmniboxAlignment();
    }

    @Override
    public void onLowMemory() {}

    @Override
    public float getVerticalTranslationForAnimation() {
        return mAlignmentView.getTranslationY();
    }

    /**
     * Recalculates the desired alignment of the omnibox and sends the updated alignment data to any
     * observers. Currently will send an update message unconditionally. This method is called
     * during layout and should avoid memory allocations other than the necessary new
     * OmniboxAlignment(). The method aligns the omnibox dropdown as follows:
     *
     * <p>Case 1: Omnibox revamp enabled on tablet window.
     *
     * <pre>
     *  | anchor  [  alignment  ]       |
     *            |  dropdown   |
     * </pre>
     *
     * <p>Case 2: Omnibox revamp disabled on tablet window.
     *
     * <pre>
     *  | anchor    [alignment]         |
     *  |{pad_left} dropdown {pad_right}|
     * </pre>
     *
     * <p>Case 3: Phone window. Full width and no padding.
     *
     * <pre>
     *  | anchor     [alignment]        |
     *  |           dropdown            |
     * </pre>
     */
    public void recalculateOmniboxAlignment() {
        View contentView = mAnchorView.getRootView().findViewById(android.R.id.content);
        
        if(!mKeyboardVisibilityDelegate.isKeyboardShowing(mContext,contentView)) {
            // Immediately reset everything if keyboard is hidden
            mKeyboardHeight = 0;
            ViewCompat.setPaddingRelative(contentView, 0, 0, 0, 0);
            contentView.requestLayout();
            return; // Exit early to prevent any delayed adjustments
        }

        int contentViewTopPadding = contentView == null ? 0 : contentView.getPaddingTop();

        // If there is a base Chrome layout, calculate the relative position from it rather than
        // the content view. Sometimes, Chrome will add an intermediate layout to host certain
        // views above the toolbar, such as the top back button toolbar on automotive devices.
        // Since the omnibox alignment top padding will position the omnibox relative to this base
        // layout, rather than the content view, the base layout should be used here to avoid
        // "double counting" and creating a gap between the browser controls and omnibox
        // suggestions.
        View baseRelativeLayout = mBaseChromeLayout != null ? mBaseChromeLayout : contentView;
        ViewUtils.getRelativeLayoutPosition(baseRelativeLayout, mAnchorView, mPositionArray);

        int top = mPositionArray[1] + mAnchorView.getMeasuredHeight() - contentViewTopPadding;
        top -= mPositionArray[1];
        int left;
        int width;
        int paddingLeft;
        int paddingRight;
        if (isTablet()) {
            ViewUtils.getRelativeLayoutPosition(mAnchorView, mAlignmentView, mPositionArray);
            // Width equal to alignment view and left equivalent to left of alignment view. Top
            // minus a small overlap.
            top -=
                    mContext.getResources()
                            .getDimensionPixelSize(R.dimen.omnibox_suggestion_list_toolbar_overlap);
            int sideSpacing = OmniboxResourceProvider.getDropdownSideSpacing(mContext);
            width = mAlignmentView.getMeasuredWidth() + 2 * sideSpacing;

            if (mAnchorView.getLayoutDirection() == View.LAYOUT_DIRECTION_RTL) {
                // The view will be shifted to the left, so the adjustment needs to be negative.
                left = -(mAnchorView.getMeasuredWidth() - width - mPositionArray[0] + sideSpacing);
            } else {
                left = mPositionArray[0] - sideSpacing;
            }
            paddingLeft = 0;
            paddingRight = 0;
        } else {
            // Case 3: phones or phone-sized windows on tablets. Full bleed width with no padding or
            // positioning adjustments.
            left = 0;
            width = mAnchorView.getMeasuredWidth();
            paddingLeft = 0;
            paddingRight = 0;
        }

        // int keyboardHeight = 
                // mDeferredIMEWindowInsetApplicationCallback != null
                //         ? mDeferredIMEWindowInsetApplicationCallback.getCurrentKeyboardHeight() : 0;
                

        int windowHeight;
        if (BuildInfo.getInstance().isAutomotive
                && contentView != null
                && contentView.getRootWindowInsets() != null) {
            // Some automotive devices dismiss bottom system bars when bringing up the keyboard,
            // preventing the height of those bottom bars from being subtracted from the keyboard.
            // To avoid a bottom-bar-sized gap above the keyboard, Chrome needs to calculate a new
            // window height from the display with the new system bar insets, rather than rely on
            // the cached mWindowHeightDp (that implicitly assumes persistence of the now-dismissed
            // bottom system bars).
            WindowInsetsCompat windowInsets =
                    WindowInsetsCompat.toWindowInsetsCompat(contentView.getRootWindowInsets());
            Insets systemBars = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars());
            windowHeight =
                    mWindowAndroid.getDisplay().getDisplayHeight()
                            - systemBars.top
                            - systemBars.bottom;
        } else {
            windowHeight = DisplayUtil.dpToPx(mWindowAndroid.getDisplay(), mWindowHeightDp);
        }

        int minSpaceAboveWindowBottom =
                mContext.getResources()
                        .getDimensionPixelSize(R.dimen.omnibox_min_space_above_window_bottom);
        int windowSpace = Math.min(windowHeight - mKeyboardHeight, windowHeight - minSpaceAboveWindowBottom);
        // If content view is null, then omnibox might not be in the activity content.
        int contentSpace =
                contentView == null
                        ? Integer.MAX_VALUE
                        : (contentView.getMeasuredHeight() - mKeyboardHeight);
        int height = Math.min(windowSpace, Integer.MAX_VALUE/*contentSpace*/) - top;
        int offset = -25;
        Log.d("Omnibox", "windowSpace: " + windowSpace + 
        " contentSpace: " + contentSpace + 
        " height: " + height + 
        " KeyboardHeight: " + mKeyboardHeight +
        " windowHeight: " + windowHeight +
        " contentViewHeight: " + (contentView != null ? contentView.getMeasuredHeight() : "null"));

        if(mKeyboardVisibilityDelegate.isKeyboardShowing(mContext,contentView) || (mKeyboardHeight > 0)){
            // Only add padding if keyboard is showing and there's a gap
            if(windowSpace - mKeyboardHeight - contentSpace < offset){
                ViewCompat.setPaddingRelative(contentView, 0, 0, 0, mKeyboardHeight);
            }
        } else {
            // Reset padding when keyboard is hidden
            ViewCompat.setPaddingRelative(contentView, 0, 0, 0, 0);
            mKeyboardHeight = 0;
        }

        // Force layout refresh
        if (contentView != null) {
            contentView.requestLayout();
        }

        top = 0;
        OmniboxAlignment omniboxAlignment =
                new OmniboxAlignment(left, top, width, height, paddingLeft, paddingRight);
        mOmniboxAlignmentSupplier.set(omniboxAlignment);
    }

    /**
     * Returns whether the given view's position in the window has changed since the last call to
     * offsetInWindowChanged().
     */
    private boolean offsetInWindowChanged(View view) {
        view.getLocationInWindow(mPositionArray);
        boolean result = mVerticalOffsetInWindow != mPositionArray[1];
        mVerticalOffsetInWindow = mPositionArray[1];
        return result;
    }

    /**
     * Returns whether the window insets corresponding to the given view have changed since the last
     * call to insetsHaveChanged().
     */
    private boolean insetsHaveChanged(View view) {
        WindowInsets rootWindowInsets = view.getRootWindowInsets();
        if (rootWindowInsets == null) return false;
        WindowInsetsCompat windowInsetsCompat =
                WindowInsetsCompat.toWindowInsetsCompat(view.getRootWindowInsets(), view);
        boolean result = !windowInsetsCompat.equals(mWindowInsetsCompat);
        mWindowInsetsCompat = windowInsetsCompat;
        return result;
    }
}
// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTOFILL_BOTTOM_SHEET_PAYMENTS_SUGGESTION_BOTTOM_SHEET_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_AUTOFILL_BOTTOM_SHEET_PAYMENTS_SUGGESTION_BOTTOM_SHEET_DELEGATE_H_

#import <Foundation/Foundation.h>
#import "ios/chrome/browser/autofill/model/credit_card/credit_card_data.h"

// Delegate for the payments bottom sheet.
@protocol PaymentsSuggestionBottomSheetDelegate

// Request to disable the bottom sheet, potentially refocusing the field which
// originally triggered the bottom sheet after the bottom sheet has been
// disabled.
- (void)disableBottomSheetAndRefocus:(BOOL)refocus;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTOFILL_BOTTOM_SHEET_PAYMENTS_SUGGESTION_BOTTOM_SHEET_DELEGATE_H_

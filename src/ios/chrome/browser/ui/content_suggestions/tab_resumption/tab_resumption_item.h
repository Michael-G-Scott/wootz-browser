// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_TAB_RESUMPTION_TAB_RESUMPTION_ITEM_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_TAB_RESUMPTION_TAB_RESUMPTION_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/content_suggestions/magic_stack/magic_stack_module.h"

@protocol TabResumptionCommands;

namespace base {
class Time;
}  // namespace base

class GURL;

// Tab resumption item types.
enum TabResumptionItemType {
  // Last tab synced from another devices.
  kLastSyncedTab,
  // Most recent opened tab on the current device.
  kMostRecentTab,
};

// Item used to display the tab resumption tile.
@interface TabResumptionItem : MagicStackModule

// The type of the tab.
@property(nonatomic, readonly) TabResumptionItemType itemType;

// The name of the session to which the tab belongs.
@property(nonatomic, copy) NSString* sessionName;

// The title of the tab.
@property(nonatomic, copy) NSString* tabTitle;

// The URL of the tab.
@property(nonatomic, assign) GURL tabURL;

// The time when the tab was synced.
@property(nonatomic, assign) base::Time syncedTime;

// The favicon image of the tab.
@property(nonatomic, strong) UIImage* faviconImage;

// Command handler for user actions.
@property(nonatomic, weak) id<TabResumptionCommands> commandHandler;

// The Item's designated initializer.
- (instancetype)initWithItemType:(TabResumptionItemType)itemType
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_TAB_RESUMPTION_TAB_RESUMPTION_ITEM_H_

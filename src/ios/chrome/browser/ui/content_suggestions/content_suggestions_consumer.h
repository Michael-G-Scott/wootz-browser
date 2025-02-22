// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_CONSUMER_H_

#import "base/ios/block_types.h"

enum class ContentSuggestionsModuleType;
@class ContentSuggestionsWhatsNewItem;
@class MostVisitedTilesConfig;
@class SafetyCheckState;
@class SetUpListConfig;
enum class SetUpListItemType;
@class SetUpListItemViewData;
@class ShortcutsConfig;
@class ParcelTrackingItem;
@class TabResumptionItem;

// MagicStackOrderChange is used in `updateMagicStackOrder:withStatus:` to
// indicate what module has changed and how it needs to be updated.
struct MagicStackOrderChange {
  enum class Type {
    kInsert = 1,
    kRemove,
    kReplace,
  };
  Type type;
  // New Module. Will be set for kReplace and kInsert.
  ContentSuggestionsModuleType new_module;
  // Old Module. Will be set for kReplace and kRemove.
  ContentSuggestionsModuleType old_module;
  // The index of `newModule`, applies to `oldModule` for kRemove.
  NSInteger index;
};

// Supports adding/removing/updating UI elements to the ContentSuggestions
// UIViewController.
@protocol ContentSuggestionsConsumer

// Indicates to the consumer the current Most Visited tiles to show with
// `config`.
- (void)setMostVisitedTilesConfig:(MostVisitedTilesConfig*)config;

// Indicates to the consumer the current Shortcuts tiles `config`.
- (void)setShortcutTilesConfig:(ShortcutsConfig*)configs;

// Indicates to the consumer to set the Magic Stack module order as listed in
// `order`.
- (void)setMagicStackOrder:(NSArray<NSNumber*>*)order;

// Indicates to the consumer to update the Magic Stack module order for a given
// module `type` with the latest `status` change.
// TODO(crbug.com/40280443) Also pass the view configs through this API instead
// of having to call the feature-specific calls in this protocol.
- (void)updateMagicStackOrder:(MagicStackOrderChange)change;

// Indicates to the consumer to scroll to the next module because `moduleType`
// is completed.
- (void)scrollToNextMagicStackModuleForCompletedModule:
    (ContentSuggestionsModuleType)moduleType;

// Indicates to the consumer to display the SetUpList - a list of
// tasks that a new user may want to complete.
- (void)showSetUpListModuleWithConfigs:(NSArray<SetUpListConfig*>*)configs;

// Hides the Set Up List, if it is currently visible. The given `animations`
// block will be called as part of the animation that hides the Set Up list, to
// allow other things to be animated at the same time.
- (void)hideSetUpListWithAnimations:(ProceduralBlock)animations;

// Shows the "All Set" screen which indicates to the user that all items are
// complete. Calls `animations` to allow other things to be simultaneously
// animated.
- (void)showSetUpListDoneWithAnimations:(ProceduralBlock)animations;

// Shows the Safety Check (Magic Stack) module with `state`.
- (void)showSafetyCheck:(SafetyCheckState*)state;

// Indicates to the consumer to display the tab resumption tile with the given
// `item` configuration.
- (void)showTabResumptionWithItem:(TabResumptionItem*)item;

// Hides the tab resumption tile.
- (void)hideTabResumption;

// Indicates to the consumer to display the parcel tracking module with the
// `item` configuration.
- (void)showParcelTrackingItem:(ParcelTrackingItem*)item;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_CONSUMER_H_

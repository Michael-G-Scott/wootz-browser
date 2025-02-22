// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/bookmarks/model/bookmark_ios_unit_test_support.h"

#import <memory>

#import "base/feature_list.h"
#import "base/strings/sys_string_conversions.h"
#import "components/bookmarks/browser/bookmark_model.h"
#import "components/bookmarks/common/bookmark_features.h"
#import "components/bookmarks/common/bookmark_metrics.h"
#import "components/sync/base/features.h"
#import "ios/chrome/browser/bookmarks/model/account_bookmark_model_factory.h"
#import "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#import "ios/chrome/browser/bookmarks/model/legacy_bookmark_model.h"
#import "ios/chrome/browser/bookmarks/model/legacy_bookmark_model_test_helpers.h"
#import "ios/chrome/browser/bookmarks/model/local_or_syncable_bookmark_model_factory.h"
#import "ios/chrome/browser/bookmarks/model/managed_bookmark_service_factory.h"
#import "ios/chrome/browser/shared/model/browser/test/test_browser.h"
#import "ios/chrome/browser/shared/model/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/signin/model/authentication_service_factory.h"
#import "ios/chrome/browser/signin/model/fake_authentication_service_delegate.h"
#import "url/gurl.h"

using bookmarks::BookmarkNode;

BookmarkIOSUnitTestSupport::BookmarkIOSUnitTestSupport(
    bool wait_for_initialization)
    : wait_for_initialization_(wait_for_initialization) {}

BookmarkIOSUnitTestSupport::~BookmarkIOSUnitTestSupport() = default;

void BookmarkIOSUnitTestSupport::SetUp() {
  // Get a BookmarkModel from the test ChromeBrowserState.
  TestChromeBrowserState::Builder test_cbs_builder;
  test_cbs_builder.AddTestingFactory(
      AuthenticationServiceFactory::GetInstance(),
      AuthenticationServiceFactory::GetDefaultFactory());
  test_cbs_builder.AddTestingFactory(
      ios::LocalOrSyncableBookmarkModelFactory::GetInstance(),
      ios::LocalOrSyncableBookmarkModelFactory::GetDefaultFactory());
  test_cbs_builder.AddTestingFactory(
      ios::AccountBookmarkModelFactory::GetInstance(),
      ios::AccountBookmarkModelFactory::GetDefaultFactory());
  test_cbs_builder.AddTestingFactory(
      ios::BookmarkModelFactory::GetInstance(),
      ios::BookmarkModelFactory::GetDefaultFactory());
  test_cbs_builder.AddTestingFactory(
      ManagedBookmarkServiceFactory::GetInstance(),
      ManagedBookmarkServiceFactory::GetDefaultFactory());

  chrome_browser_state_ = test_cbs_builder.Build();

  SetUpBrowserStateBeforeCreatingServices();

  AuthenticationServiceFactory::CreateAndInitializeForBrowserState(
      chrome_browser_state_.get(),
      std::make_unique<FakeAuthenticationServiceDelegate>());

  local_or_syncable_bookmark_model_ =
      ios::LocalOrSyncableBookmarkModelFactory::GetForBrowserState(
          chrome_browser_state_.get());
  if (wait_for_initialization_) {
    WaitForLegacyBookmarkModelToLoad(local_or_syncable_bookmark_model_);
  }

  account_bookmark_model_ =
      ios::AccountBookmarkModelFactory::GetForBrowserState(
          chrome_browser_state_.get());
  if (wait_for_initialization_ && account_bookmark_model_) {
    WaitForLegacyBookmarkModelToLoad(account_bookmark_model_);
  }

  bookmark_model_ = ios::BookmarkModelFactory::GetForBrowserState(
      chrome_browser_state_.get());
  if (wait_for_initialization_) {
    // Waiting for the two underlying models, done earlier, should guarantee
    // that the merged view is also loaded.
    EXPECT_TRUE(bookmark_model_->loaded());
  }

  pref_service_ = chrome_browser_state_->GetPrefs();
  EXPECT_TRUE(pref_service_);

  if (wait_for_initialization_ &&
      base::FeatureList::IsEnabled(
          syncer::kEnableBookmarkFoldersForAccountStorage)) {
    // Some tests exercise account bookmarks. Make sure their permanent
    // folders exist.
    ios::BookmarkModelFactory::GetModelForBrowserStateIfUnificationEnabledOrDie(
        chrome_browser_state_.get())
        ->CreateAccountPermanentFolders();
  }

  browser_ = std::make_unique<TestBrowser>(chrome_browser_state_.get());
}

void BookmarkIOSUnitTestSupport::SetUpBrowserStateBeforeCreatingServices() {}

const BookmarkNode* BookmarkIOSUnitTestSupport::AddBookmark(
    const BookmarkNode* parent,
    const std::u16string& title) {
  GURL url(u"http://example.com/bookmark" + title);
  return AddBookmark(parent, title, url);
}

const BookmarkNode* BookmarkIOSUnitTestSupport::AddBookmark(
    const BookmarkNode* parent,
    const std::u16string& title,
    const GURL& url) {
  LegacyBookmarkModel* model = GetBookmarkModelForNode(parent);
  return model->AddURL(parent, parent->children().size(), title, url);
}

const BookmarkNode* BookmarkIOSUnitTestSupport::AddFolder(
    const BookmarkNode* parent,
    const std::u16string& title) {
  LegacyBookmarkModel* model = GetBookmarkModelForNode(parent);
  return model->AddFolder(parent, parent->children().size(), title);
}

void BookmarkIOSUnitTestSupport::ChangeTitle(const std::u16string& title,
                                             const BookmarkNode* node) {
  LegacyBookmarkModel* model = GetBookmarkModelForNode(node);
  model->SetTitle(node, title, bookmarks::metrics::BookmarkEditSource::kUser);
}

LegacyBookmarkModel* BookmarkIOSUnitTestSupport::GetBookmarkModelForNode(
    const BookmarkNode* node) {
  DCHECK(node);
  if (local_or_syncable_bookmark_model_->IsNodePartOfModel(node)) {
    return local_or_syncable_bookmark_model_;
  }
  DCHECK(account_bookmark_model_);
  DCHECK(account_bookmark_model_->IsNodePartOfModel(node));
  return account_bookmark_model_;
}

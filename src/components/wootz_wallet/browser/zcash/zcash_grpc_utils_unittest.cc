/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/wootz_wallet/browser/zcash/zcash_grpc_utils.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "components/services/wootz_wallet/public/cpp/utils/protobuf_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace wootz_wallet {

namespace {

class TestTxStreamHandler : public GRrpcMessageStreamHandler {
 public:
  TestTxStreamHandler() = default;
  ~TestTxStreamHandler() override = default;

  std::vector<std::string> messages() { return messages_; }
  std::optional<bool> complete_result() { return complete_result_; }
  void set_should_continue(bool r) { should_continue_ = r; }

 private:
  bool ProcessMessage(std::string_view message) override {
    messages_.emplace_back(message);
    return should_continue_;
  }

  void OnComplete(bool success) override { complete_result_ = success; }

  std::vector<std::string> messages_;
  std::optional<bool> complete_result_;
  bool should_continue_ = true;
};

}  // namespace

TEST(ZCashGrpcUtilsTest, TestTxStreamHandler) {
  TestTxStreamHandler handler;
  {
    std::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived("", std::move(resume_closure));
    EXPECT_EQ(0u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  auto message = GetPrefixedProtobuf("message1");
  // Not enough data to complete message
  {
    std::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(message.substr(0, 3), std::move(resume_closure));
    EXPECT_EQ(0u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  // Complete message
  {
    std::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(message.substr(3, message.size()),
                           std::move(resume_closure));
    EXPECT_EQ(1u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  // Another message, but almost received
  {
    std::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(message.substr(0, message.size() - 1),
                           std::move(resume_closure));
    EXPECT_EQ(1u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  // Complete
  {
    std::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(message.substr(message.size() - 1, 1),
                           std::move(resume_closure));
    EXPECT_EQ(2u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  auto bundled_message = base::StrCat(
      {GetPrefixedProtobuf("message1"), GetPrefixedProtobuf("message2")});
  // Several messages received at once
  {
    std::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(bundled_message, std::move(resume_closure));
    EXPECT_EQ(4u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }
}

}  // namespace wootz_wallet

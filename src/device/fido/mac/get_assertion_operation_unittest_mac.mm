// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Foundation/Foundation.h>
#include <Security/Security.h>

#include <array>

#include "base/strings/string_number_conversions.h"
#include "base/test/task_environment.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_test_data.h"
#include "device/fido/mac/authenticator_config.h"
#include "device/fido/mac/credential_store.h"
#include "device/fido/mac/get_assertion_operation.h"
#include "device/fido/mac/make_credential_operation.h"
#include "device/fido/test_callback_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device::fido::mac {

namespace {

using test::TestCallbackReceiver;

const std::string kRpId = "rp.example.com";
const std::vector<uint8_t> kUserId = {10, 11, 12, 13, 14, 15};
const char kKeychainAccessGroup[] =
    "EQHXZ8M8AV.com.google.chrome.webauthn.test";

CtapGetAssertionRequest MakeTestRequest() {
  return CtapGetAssertionRequest(kRpId, test_data::kClientDataJson);
}

bool MakeCredential() {
  TestCallbackReceiver<CtapDeviceResponseCode,
                       std::optional<AuthenticatorMakeCredentialResponse>>
      callback_receiver;
  auto request = CtapMakeCredentialRequest(
      test_data::kClientDataJson, PublicKeyCredentialRpEntity(kRpId),
      PublicKeyCredentialUserEntity(kUserId),
      PublicKeyCredentialParams(
          {{PublicKeyCredentialParams::
                CredentialInfo() /* defaults to ES-256 */}}));
  TouchIdCredentialStore credential_store(
      AuthenticatorConfig{"test-profile", kKeychainAccessGroup});
  MakeCredentialOperation op(request, &credential_store,
                             callback_receiver.callback());

  op.Run();
  callback_receiver.WaitForCallback();
  auto result = callback_receiver.TakeResult();
  CtapDeviceResponseCode error = std::get<0>(result);
  auto opt_responses = std::move(std::get<1>(result));
  return error == CtapDeviceResponseCode::kSuccess && opt_responses;
}

// For demo purposes only. This test does a Touch ID user prompt. It will fail
// on incompatible hardware and crash if not code signed or lacking the
// keychain-access-group entitlement.
TEST(GetAssertionOperationTest, DISABLED_TestRun) {
  base::test::TaskEnvironment task_environment;
  ASSERT_TRUE(MakeCredential());

  TestCallbackReceiver<CtapDeviceResponseCode,
                       std::vector<AuthenticatorGetAssertionResponse>>
      callback_receiver;
  auto request = MakeTestRequest();
  TouchIdCredentialStore credential_store(
      AuthenticatorConfig{"test-profile", kKeychainAccessGroup});
  GetAssertionOperation op(request, &credential_store,
                           callback_receiver.callback());

  op.Run();
  callback_receiver.WaitForCallback();
  auto result = callback_receiver.TakeResult();
  CtapDeviceResponseCode error = std::get<0>(result);
  EXPECT_EQ(CtapDeviceResponseCode::kSuccess, error);
  auto opt_responses = std::move(std::get<1>(result));
  ASSERT_EQ(opt_responses.size(), 1u);
  EXPECT_FALSE(opt_responses.at(0).credential->id.empty());
}

}  // namespace

}  // namespace device::fido::mac

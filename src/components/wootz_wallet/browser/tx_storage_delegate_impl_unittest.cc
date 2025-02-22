/* Copyright (c) 2024 The Wootzapp Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/wootz_wallet/browser/tx_storage_delegate_impl.h"

#include <optional>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "components/wootz_wallet/browser/wootz_wallet_prefs.h"
#include "components/wootz_wallet/browser/pref_names.h"
#include "components/wootz_wallet/browser/test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;
using base::test::ParseJsonDict;

namespace wootz_wallet {

namespace {
constexpr char kLegacyFormatTransactionsDict[] = R"({
    "chain_id_migrated": true,
    "ethereum": {
        "goerli": {
            "a336ef2c-9716-4cb7-8bb2-7fce8704a661": {
                "chain_id": "0x5",
                "confirmed_time": "13324786394428041",
                "tx": {
                    "data": "",
                },
            },
            "c6d9bc1a-b8a2-4abe-919e-3f6c1dc78ef4": {
                "chain_id": "0x5",
            }
        },
        "mainnet": {
            "71a841a4-83dc-4286-9acd-9b7f50e90fda": {
                "chain_id": "0x1",
                "confirmed_time": "0",
                "tx": {
                    "data": "",
                },
                "tx_hash": "",
                "tx_receipt": {
                    "block_hash": "",
                }
            }
        }
    },
    "solana": {
        "devnet": {
            "40fa081e-55c8-4052-a7e9-e32ffaa44ba9": {
                "chain_id": "0x67",
                "confirmed_time": "0",
                "signature_status": {
                    "confirmation_status": "",
                    "confirmations": "0",
                    "err": "",
                    "slot": "0"
                },
                "status": 2,
                "submitted_time": "0",
                "tx_hash": ""
            }
        }
    }
    })";

constexpr char kCurrentFormatTransactionsDict[] = R"({
    "a336ef2c-9716-4cb7-8bb2-7fce8704a661": {
        "coin": 60,
        "chain_id": "0x5",
        "confirmed_time": "13324786394428041",
        "tx": {
            "data": "",
        },
    },
    "c6d9bc1a-b8a2-4abe-919e-3f6c1dc78ef4": {
        "coin": 60,
        "chain_id": "0x5",
    },
    "71a841a4-83dc-4286-9acd-9b7f50e90fda": {
        "coin": 60,
        "chain_id": "0x1",
        "confirmed_time": "0",
        "tx": {
            "data": "",
        },
        "tx_hash": "",
        "tx_receipt": {
            "block_hash": "",
        }
    },
    "40fa081e-55c8-4052-a7e9-e32ffaa44ba9": {
        "coin": 501,
        "chain_id": "0x67",
        "confirmed_time": "0",
        "signature_status": {
            "confirmation_status": "",
            "confirmations": "0",
            "err": "",
            "slot": "0"
        },
        "status": 2,
        "submitted_time": "0",
        "tx_hash": ""
    }
    })";
}  // namespace

class TxStorageDelegateImplUnitTest : public testing::Test {
 public:
  TxStorageDelegateImplUnitTest() {}

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    factory_ = GetTestValueStoreFactory(temp_dir_);
  }

  std::optional<base::Value> GetTxsFromDB(TxStorageDelegateImpl* delegate) {
    base::RunLoop run_loop;
    std::optional<base::Value> value_out;
    delegate->store_->Get(
        "transactions",
        base::BindLambdaForTesting([&](std::optional<base::Value> value) {
          value_out = std::move(value);
          run_loop.Quit();
        }));
    run_loop.Run();
    return value_out;
  }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
};

TEST_F(TxStorageDelegateImplUnitTest,
       WootzWalletTransactionsFromPrefsToDBMigrated) {
  {  // Nothing to migrate, ex. fresh profile
    ASSERT_FALSE(
        prefs_.GetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated));
    ASSERT_FALSE(prefs_.HasPrefPath(kWootzWalletTransactions));
    auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
    EXPECT_TRUE(delegate->IsInitialized());
    EXPECT_TRUE(
        prefs_.GetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated));
    EXPECT_FALSE(prefs_.HasPrefPath(kWootzWalletTransactions));
    prefs_.ClearPref(kWootzWalletTransactionsFromPrefsToDBMigrated);
  }
  {  // already migrated
    prefs_.SetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated, true);
    auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
    EXPECT_TRUE(delegate->IsInitialized());
    EXPECT_TRUE(
        prefs_.GetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated));
    prefs_.ClearPref(kWootzWalletTransactionsFromPrefsToDBMigrated);
  }
  {  // migration happened
    ASSERT_FALSE(
        prefs_.GetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated));

    base::Value::Dict txs_value = ParseJsonDict(kLegacyFormatTransactionsDict);
    prefs_.Set(kWootzWalletTransactions, base::Value(txs_value.Clone()));
    auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
    auto txs_from_db = GetTxsFromDB(delegate.get());
    ASSERT_TRUE(txs_from_db);
    EXPECT_EQ(txs_from_db->GetDict(), txs_value);
    EXPECT_EQ(delegate->GetTxs(), txs_value);
    EXPECT_TRUE(delegate->IsInitialized());
    EXPECT_TRUE(
        prefs_.GetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated));
    // We don't clear pref transactions for this migration
    EXPECT_TRUE(prefs_.HasPrefPath(kWootzWalletTransactions));
  }
}

TEST_F(TxStorageDelegateImplUnitTest, WootzWalletTransactionsDBFormatMigrated) {
  prefs_.SetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated, true);

  {  // Nothing to migrate, ex. fresh profile
    ASSERT_FALSE(prefs_.GetBoolean(kWootzWalletTransactionsDBFormatMigrated));
    ASSERT_FALSE(prefs_.HasPrefPath(kWootzWalletTransactions));
    auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
    EXPECT_TRUE(delegate->IsInitialized());
    EXPECT_TRUE(prefs_.GetBoolean(kWootzWalletTransactionsDBFormatMigrated));

    prefs_.ClearPref(kWootzWalletTransactionsDBFormatMigrated);
  }

  {  // fill db with legacy formatted transactions dict
    auto store = TxStorageDelegateImpl::MakeValueStoreFrontend(
        factory_, base::SequencedTaskRunner::GetCurrentDefault());
    store->Set("transactions", ParseJson(kLegacyFormatTransactionsDict));
  }

  {  // migration happened
    ASSERT_FALSE(prefs_.GetBoolean(kWootzWalletTransactionsDBFormatMigrated));

    base::Value::Dict txs_value = ParseJsonDict(kCurrentFormatTransactionsDict);
    auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
    auto txs_from_db = GetTxsFromDB(delegate.get());
    ASSERT_TRUE(txs_from_db);
    EXPECT_EQ(txs_from_db->GetDict(), txs_value);
    EXPECT_EQ(delegate->GetTxs(), txs_value);
    EXPECT_TRUE(delegate->IsInitialized());
    EXPECT_TRUE(prefs_.GetBoolean(kWootzWalletTransactionsDBFormatMigrated));
  }

  {  // no double migration
    auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
    EXPECT_TRUE(delegate->IsInitialized());
    EXPECT_TRUE(prefs_.GetBoolean(kWootzWalletTransactionsDBFormatMigrated));
    EXPECT_EQ(delegate->GetTxs(),
              ParseJsonDict(kCurrentFormatTransactionsDict));
  }
}

TEST_F(TxStorageDelegateImplUnitTest, DBFormatMigrationAfterPrefsMigration) {
  ASSERT_FALSE(
      prefs_.GetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated));
  ASSERT_FALSE(prefs_.GetBoolean(kWootzWalletTransactionsDBFormatMigrated));

  prefs_.Set(kWootzWalletTransactions,
             ParseJson(kLegacyFormatTransactionsDict));

  auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
  EXPECT_EQ(delegate->GetTxs(), ParseJsonDict(kCurrentFormatTransactionsDict));

  EXPECT_TRUE(prefs_.GetBoolean(kWootzWalletTransactionsFromPrefsToDBMigrated));
  EXPECT_TRUE(prefs_.GetBoolean(kWootzWalletTransactionsDBFormatMigrated));
}

TEST_F(TxStorageDelegateImplUnitTest, ReadWriteAndClear) {
  auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
  // OnTxRead with empty txs
  auto& txs = delegate->GetTxs();
  EXPECT_TRUE(txs.empty());
  txs.Set("key1", 123);
  txs.Set("key2", base::Value::Dict().Set("nest", "wootz"));
  delegate->ScheduleWrite();
  auto txs_from_db = GetTxsFromDB(delegate.get());
  ASSERT_TRUE(txs_from_db);
  const auto& txs_from_cache = delegate->GetTxs();
  EXPECT_EQ(txs_from_cache, txs_from_db);
  EXPECT_EQ(txs, txs_from_db);

  // simulate reading from existing database (with same
  // value_store::ValueStoreFrontend)
  delegate->initialized_ = false;
  delegate->txs_.clear();
  ASSERT_FALSE(delegate->IsInitialized());
  delegate->Initialize();
  WaitForTxStorageDelegateInitialized(delegate.get());
  ASSERT_TRUE(delegate->IsInitialized());
  EXPECT_EQ(delegate->GetTxs(), txs);

  // clear
  delegate->Clear();
  EXPECT_TRUE(delegate->IsInitialized());
  EXPECT_TRUE(delegate->GetTxs().empty());
  EXPECT_FALSE(GetTxsFromDB(delegate.get()));
}

}  // namespace wootz_wallet

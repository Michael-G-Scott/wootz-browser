// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CONNECTION_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CONNECTION_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/services/storage/privileged/mojom/indexed_db_client_state_checker.mojom.h"
#include "components/services/storage/public/cpp/buckets/bucket_info.h"
#include "components/services/storage/public/cpp/buckets/bucket_locator.h"
#include "content/browser/indexed_db/indexed_db_bucket_context_handle.h"
#include "content/browser/indexed_db/indexed_db_database.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/blink/public/common/indexeddb/indexeddb_key.h"
#include "third_party/blink/public/common/indexeddb/indexeddb_key_path.h"
#include "third_party/blink/public/mojom/indexeddb/indexeddb.mojom.h"

namespace blink {
class IndexedDBKeyRange;
}

namespace content {
class IndexedDBDatabaseCallbacks;
class IndexedDBDatabaseError;
class IndexedDBTransaction;
class IndexedDBBucketContext;

class CONTENT_EXPORT IndexedDBConnection : public blink::mojom::IDBDatabase {
 public:
  // Transfers ownership of an existing `connection` instance to a self owned
  // receiver. `IndexedDBConnection` instances begin life owned by a
  // `unique_ptr` in a pending state without any bound mojo remotes. IndexedDB
  // open database operations use this function to establish the connection
  // after the database is ready for use.
  static mojo::PendingAssociatedRemote<blink::mojom::IDBDatabase>
  MakeSelfOwnedReceiverAndBindRemote(
      std::unique_ptr<IndexedDBConnection> connection);

  IndexedDBConnection(IndexedDBBucketContext& bucket_context,
                      base::WeakPtr<IndexedDBDatabase> database,
                      base::RepeatingClosure on_version_change_ignored,
                      base::OnceCallback<void(IndexedDBConnection*)> on_close,
                      std::unique_ptr<IndexedDBDatabaseCallbacks> callbacks,
                      mojo::Remote<storage::mojom::IndexedDBClientStateChecker>
                          client_state_checker,
                      uint64_t client_id);

  IndexedDBConnection(const IndexedDBConnection&) = delete;
  IndexedDBConnection& operator=(const IndexedDBConnection&) = delete;

  ~IndexedDBConnection() override;

  base::WeakPtr<IndexedDBConnection> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  base::WeakPtr<IndexedDBDatabase> database() const { return database_; }
  IndexedDBDatabaseCallbacks* callbacks() const { return callbacks_.get(); }
  uint64_t client_id() const { return client_id_; }
  const std::map<int64_t, std::unique_ptr<IndexedDBTransaction>>& transactions()
      const {
    return transactions_;
  }

  bool IsConnected() const;

  IndexedDBTransaction* CreateVersionChangeTransaction(
      int64_t id,
      const std::set<int64_t>& scope,
      IndexedDBBackingStore::Transaction* backing_store_transaction);

  // Checks if the client is in inactive state and disallow it from activation
  // if so. This is called when the client is not supposed to be inactive,
  // otherwise it may affect the IndexedDB service (e.g. blocking others from
  // acquiring the locks).
  void DisallowInactiveClient(
      storage::mojom::DisallowInactiveClientReason reason,
      base::OnceCallback<void(bool)> callback);

  // We ignore calls where the id doesn't exist to facilitate the AbortAll call.
  // TODO(dmurph): Change that so this doesn't need to ignore unknown ids.
  void RemoveTransaction(int64_t id);

  void AbortTransactionAndTearDownOnError(IndexedDBTransaction* transaction,
                                          const IndexedDBDatabaseError& error);
  void CloseAndReportForceClose();

 private:
  friend class IndexedDBTransactionTest;
  FRIEND_TEST_ALL_PREFIXES(IndexedDBDatabaseTest, ForcedClose);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBDatabaseTest, PendingDelete);
  FRIEND_TEST_ALL_PREFIXES(IndexedDBTransactionTest,
                           PostedStartTaskRunAfterAbort);

  // blink::mojom::IDBDatabase implementation
  void RenameObjectStore(int64_t transaction_id,
                         int64_t object_store_id,
                         const std::u16string& new_name) override;
  void CreateTransaction(
      mojo::PendingAssociatedReceiver<blink::mojom::IDBTransaction>
          transaction_receiver,
      int64_t transaction_id,
      const std::vector<int64_t>& object_store_ids,
      blink::mojom::IDBTransactionMode mode,
      blink::mojom::IDBTransactionDurability durability) override;
  void VersionChangeIgnored() override;
  void Get(int64_t transaction_id,
           int64_t object_store_id,
           int64_t index_id,
           const blink::IndexedDBKeyRange& key_range,
           bool key_only,
           blink::mojom::IDBDatabase::GetCallback callback) override;
  void GetAll(int64_t transaction_id,
              int64_t object_store_id,
              int64_t index_id,
              const blink::IndexedDBKeyRange& key_range,
              bool key_only,
              int64_t max_count,
              blink::mojom::IDBDatabase::GetAllCallback callback) override;
  void SetIndexKeys(
      int64_t transaction_id,
      int64_t object_store_id,
      const blink::IndexedDBKey& primary_key,
      const std::vector<blink::IndexedDBIndexKeys>& index_keys) override;
  void SetIndexesReady(int64_t transaction_id,
                       int64_t object_store_id,
                       const std::vector<int64_t>& index_ids) override;
  void OpenCursor(
      int64_t transaction_id,
      int64_t object_store_id,
      int64_t index_id,
      const blink::IndexedDBKeyRange& key_range,
      blink::mojom::IDBCursorDirection direction,
      bool key_only,
      blink::mojom::IDBTaskType task_type,
      blink::mojom::IDBDatabase::OpenCursorCallback callback) override;
  void Count(int64_t transaction_id,
             int64_t object_store_id,
             int64_t index_id,
             const blink::IndexedDBKeyRange& key_range,
             CountCallback callback) override;
  void DeleteRange(int64_t transaction_id,
                   int64_t object_store_id,
                   const blink::IndexedDBKeyRange& key_range,
                   DeleteRangeCallback success_callback) override;
  void GetKeyGeneratorCurrentNumber(
      int64_t transaction_id,
      int64_t object_store_id,
      GetKeyGeneratorCurrentNumberCallback callback) override;
  void Clear(int64_t transaction_id,
             int64_t object_store_id,
             ClearCallback callback) override;
  void CreateIndex(int64_t transaction_id,
                   int64_t object_store_id,
                   int64_t index_id,
                   const std::u16string& name,
                   const blink::IndexedDBKeyPath& key_path,
                   bool unique,
                   bool multi_entry) override;
  void DeleteIndex(int64_t transaction_id,
                   int64_t object_store_id,
                   int64_t index_id) override;
  void RenameIndex(int64_t transaction_id,
                   int64_t object_store_id,
                   int64_t index_id,
                   const std::u16string& new_name) override;
  void Abort(int64_t transaction_id) override;
  void DidBecomeInactive() override;

  // It is an error to call either of these after `IsConnected()`
  // is no longer true.
  const storage::BucketInfo& GetBucketInfo();
  storage::BucketLocator GetBucketLocator();
  IndexedDBTransaction* GetTransaction(int64_t id) const;

  enum class CloseErrorHandling {
    // Returns from the function on the first encounter with an error.
    kReturnOnFirstError,
    // Continues to call Abort() on all transactions despite any errors.
    // The last error encountered is returned.
    kAbortAllReturnLastError,
  };

  // The return value is `callbacks_`, passing ownership.
  std::unique_ptr<IndexedDBDatabaseCallbacks> AbortTransactionsAndClose(
      CloseErrorHandling error_handling);

  // Returns the last error that occurred, if there is any.
  leveldb::Status AbortAllTransactionsAndIgnoreErrors(
      const IndexedDBDatabaseError& error);

  leveldb::Status AbortAllTransactions(const IndexedDBDatabaseError& error);

  IndexedDBBucketContext* bucket_context() {
    return bucket_context_handle_.bucket_context();
  }

  const int32_t id_;

  // Keeps the factory for this bucket alive.
  IndexedDBBucketContextHandle bucket_context_handle_;

  base::WeakPtr<IndexedDBDatabase> database_;
  base::RepeatingClosure on_version_change_ignored_;
  base::OnceCallback<void(IndexedDBConnection*)> on_close_;

  // The connection owns transactions created on this connection. It's important
  // to preserve ordering.
  std::map<int64_t, std::unique_ptr<IndexedDBTransaction>> transactions_;

  // The callbacks_ member is cleared when the connection is closed.
  // May be nullptr in unit tests.
  std::unique_ptr<IndexedDBDatabaseCallbacks> callbacks_;

  mojo::Remote<storage::mojom::IndexedDBClientStateChecker>
      client_state_checker_;

  mojo::RemoteSet<storage::mojom::IndexedDBClientKeepActive>
      client_keep_active_remotes_;

  // Uniquely identifies the RenderFrameHost or WorkerHost that owns the other
  // side of this connection, i.e. the "client" of `client_state_checker_`, only
  // within the IDB bucket that this connection belongs to. Since multiple
  // transactions/connections associated with a single client should never cause
  // that client to be ineligible for BFCache, this ID is used to avoid
  // unnecessary calls to `DisallowInactiveClient()`.
  uint64_t client_id_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<IndexedDBConnection> weak_factory_{this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_CONNECTION_H_

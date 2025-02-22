// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ML_ML_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ML_ML_H_

#include "components/ml/mojom/ml_service.mojom-blink.h"
#include "services/webnn/public/mojom/webnn_context_provider.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_observer.h"
#include "third_party/blink/renderer/modules/ml/ml_context.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"
#include "third_party/blink/renderer/platform/mojo/heap_mojo_remote.h"

namespace blink {

class MLContextOptions;
class ScriptState;

// This class represents the "Machine Learning" object "navigator.ml" and will
// be shared between the Model Loader API and WebNN API.
class MODULES_EXPORT ML final : public ScriptWrappable,
                                public ExecutionContextClient {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit ML(ExecutionContext* execution_context);

  ML(const ML&) = delete;
  ML& operator=(const ML&) = delete;

  void CreateModelLoader(
      ScriptState* script_state,
      ml::model_loader::mojom::blink::CreateModelLoaderOptionsPtr options,
      ml::model_loader::mojom::blink::MLService::CreateModelLoaderCallback
          callback);

  // Create `WebNNContext` message pipe with `WebNNContextProvider` mojo
  // interface.
  void CreateWebNNContext(
      webnn::mojom::blink::CreateContextOptionsPtr options,
      webnn::mojom::blink::WebNNContextProvider::CreateWebNNContextCallback
          callback);

  void Trace(blink::Visitor*) const override;

  // IDL interface:
  ScriptPromise<MLContext> createContext(ScriptState* state,
                                         MLContextOptions* option,
                                         ExceptionState& exception_state);

  void RecordPendingResolver(ScriptPromiseResolver<MLContext>* resolver);

  void RemovePendingResolver(ScriptPromiseResolver<MLContext>* resolver);

 private:
  // Binds the ModelLoader Mojo connection to browser process if needed.
  // Caller is responsible to ensure `script_state` has a valid
  // `ExecutionContext`.
  void EnsureModelLoaderServiceConnection(ScriptState* script_state);
  HeapMojoRemote<ml::model_loader::mojom::blink::MLService>
      model_loader_service_;

  // Reset the remote of `WebNNContextProvider` if the remote is cut off from
  // its receiver.
  void OnWebNNServiceConnectionError();

  // There is only one WebNN service running out of renderer process to access
  // the hardware accelerated OS machine learning API. Every `navigator.ml`
  // object has one `WebNNContextProvider` message pipe to create `WebNNContext`
  // mojo interface.
  void EnsureWebNNServiceConnection();

  // WebNN support multiple types of neural network inference hardware
  // acceleration such as CPU, GPU and ML specialized accelerator, the context
  // of webnn in service is used to map different device and represent a state
  // of graph execution processes.
  HeapMojoRemote<webnn::mojom::blink::WebNNContextProvider>
      webnn_context_provider_;

  // Keep a set of unresolved `ScriptPromiseResolver`s which will be
  // rejected when the Mojo pipe is unexpectedly disconnected.
  HeapHashSet<Member<ScriptPromiseResolver<MLContext>>> pending_resolvers_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ML_ML_H_

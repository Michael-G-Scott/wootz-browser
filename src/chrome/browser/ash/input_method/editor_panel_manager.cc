// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/input_method/editor_panel_manager.h"

#include <string_view>
#include <utility>

#include "ash/constants/ash_features.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/notreached.h"
#include "chrome/browser/ash/input_method/editor_consent_enums.h"
#include "chrome/browser/ash/input_method/editor_metrics_enums.h"
#include "chrome/browser/ash/input_method/editor_metrics_recorder.h"
#include "chromeos/crosapi/mojom/editor_panel.mojom.h"

namespace ash::input_method {

namespace {

crosapi::mojom::EditorPanelPresetQueryCategory ToEditorPanelQueryCategory(
    const orca::mojom::PresetTextQueryType query_type) {
  switch (query_type) {
    case orca::mojom::PresetTextQueryType::kUnknown:
      return crosapi::mojom::EditorPanelPresetQueryCategory::kUnknown;
    case orca::mojom::PresetTextQueryType::kShorten:
      return crosapi::mojom::EditorPanelPresetQueryCategory::kShorten;
    case orca::mojom::PresetTextQueryType::kElaborate:
      return crosapi::mojom::EditorPanelPresetQueryCategory::kElaborate;
    case orca::mojom::PresetTextQueryType::kRephrase:
      return crosapi::mojom::EditorPanelPresetQueryCategory::kRephrase;
    case orca::mojom::PresetTextQueryType::kFormalize:
      return crosapi::mojom::EditorPanelPresetQueryCategory::kFormalize;
    case orca::mojom::PresetTextQueryType::kEmojify:
      return crosapi::mojom::EditorPanelPresetQueryCategory::kEmojify;
    case orca::mojom::PresetTextQueryType::kProofread:
      return crosapi::mojom::EditorPanelPresetQueryCategory::kProofread;
  }
}

crosapi::mojom::EditorPanelMode GetEditorPanelMode(EditorMode editor_mode) {
  switch (editor_mode) {
    case EditorMode::kBlocked:
      return crosapi::mojom::EditorPanelMode::kBlocked;
    case EditorMode::kConsentNeeded:
      return crosapi::mojom::EditorPanelMode::kPromoCard;
    case EditorMode::kRewrite:
      return crosapi::mojom::EditorPanelMode::kRewrite;
    case EditorMode::kWrite:
      return crosapi::mojom::EditorPanelMode::kWrite;
  }
}

crosapi::mojom::EditorPanelPresetTextQueryPtr ToEditorPanelQuery(
    const orca::mojom::PresetTextQueryPtr& orca_query) {
  auto editor_panel_query = crosapi::mojom::EditorPanelPresetTextQuery::New();
  editor_panel_query->text_query_id = orca_query->id;
  editor_panel_query->name = orca_query->label;
  editor_panel_query->description = orca_query->description;
  editor_panel_query->category = ToEditorPanelQueryCategory(orca_query->type);
  return editor_panel_query;
}

}  // namespace

EditorPanelManager::EditorPanelManager(Delegate* delegate)
    : delegate_(delegate) {}

EditorPanelManager::~EditorPanelManager() = default;

void EditorPanelManager::BindReceiver(
    mojo::PendingReceiver<crosapi::mojom::EditorPanelManager>
        pending_receiver) {
  receivers_.Add(this, std::move(pending_receiver));
}

void EditorPanelManager::BindEditorClient() {
  if (!editor_client_remote_.is_bound()) {
    delegate_->BindEditorClient(
        editor_client_remote_.BindNewPipeAndPassReceiver());

    editor_client_remote_.reset_on_disconnect();
  }
}

void EditorPanelManager::GetEditorPanelContext(
    GetEditorPanelContextCallback callback) {
  // Force fetching and updating the input context.
  // TODO: b:332605855 - Remove this hack after input context is fetched within
  // GetEditorPanelMode / GetEditorMode.
  if (base::FeatureList::IsEnabled(
          ash::features::kOrcaForceFetchContextOnGetEditorPanelContext)) {
    delegate_->FetchAndUpdateInputContext();
  }

  // Cache the current text context, so that any input fields that are part of
  // the editor panel do not interfere with the context.
  delegate_->CacheContext();

  // TODO(b/295059934): Get the panel mode from the editor mediator.
  const auto editor_panel_mode = GetEditorPanelMode(delegate_->GetEditorMode());

  // TODO(b/295059934): Bind the editor client before getting the preset text
  // queries.
  if (editor_panel_mode == crosapi::mojom::EditorPanelMode::kRewrite &&
      editor_client_remote_.is_bound()) {
    editor_client_remote_->GetPresetTextQueries(
        base::BindOnce(&EditorPanelManager::OnGetPresetTextQueriesResult,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    auto context = crosapi::mojom::EditorPanelContext::New();
    context->editor_panel_mode = editor_panel_mode;
    std::move(callback).Run(std::move(context));
  }
}

void EditorPanelManager::OnPromoCardDismissed() {}

void EditorPanelManager::OnPromoCardDeclined() {
  delegate_->OnPromoCardDeclined();
  delegate_->GetMetricsRecorder()->LogEditorState(
      EditorStates::kPromoCardExplicitDismissal);
}

void EditorPanelManager::StartEditingFlow() {
  delegate_->HandleTrigger(/*preset_query_id=*/std::nullopt,
                           /*freeform_text=*/std::nullopt);
}

void EditorPanelManager::StartEditingFlowWithPreset(
    const std::string& text_query_id) {
  delegate_->HandleTrigger(/*preset_query_id=*/text_query_id,
                           /*freeform_text=*/std::nullopt);
}

void EditorPanelManager::StartEditingFlowWithFreeform(const std::string& text) {
  delegate_->HandleTrigger(/*preset_query_id=*/std::nullopt,
                           /*freeform_text=*/text);
}

void EditorPanelManager::OnGetPresetTextQueriesResult(
    GetEditorPanelContextCallback callback,
    std::vector<orca::mojom::PresetTextQueryPtr> queries) {
  auto context = crosapi::mojom::EditorPanelContext::New();
  context->editor_panel_mode = crosapi::mojom::EditorPanelMode::kRewrite;
  for (const auto& query : queries) {
    context->preset_text_queries.push_back(ToEditorPanelQuery(query));
  }
  std::move(callback).Run(std::move(context));
}

void EditorPanelManager::OnEditorMenuVisibilityChanged(bool visible) {
  is_editor_menu_visible_ = visible;
}

bool EditorPanelManager::IsEditorMenuVisible() const {
  return is_editor_menu_visible_;
}

void EditorPanelManager::LogEditorMode(crosapi::mojom::EditorPanelMode mode) {
  EditorOpportunityMode opportunity_mode =
      delegate_->GetEditorOpportunityMode();
  EditorMetricsRecorder* logger = delegate_->GetMetricsRecorder();
  logger->SetMode(opportunity_mode);
  logger->SetTone(EditorTone::kUnset);
  if (opportunity_mode == EditorOpportunityMode::kRewrite ||
      opportunity_mode == EditorOpportunityMode::kWrite) {
    logger->LogEditorState(EditorStates::kNativeUIShowOpportunity);
  }

  if (mode == crosapi::mojom::EditorPanelMode::kRewrite ||
      mode == crosapi::mojom::EditorPanelMode::kWrite) {
    logger->LogEditorState(EditorStates::kNativeUIShown);
  }

  if (mode == crosapi::mojom::EditorPanelMode::kBlocked) {
    logger->LogEditorState(EditorStates::kBlocked);
    for (EditorBlockedReason blocked_reason : delegate_->GetBlockedReasons()) {
      logger->LogEditorState(ToEditorStatesMetric(blocked_reason));
    }
  }

  if (mode == crosapi::mojom::EditorPanelMode::kPromoCard) {
    logger->LogEditorState(EditorStates::kPromoCardImpression);
  }
}

void EditorPanelManager::BindEditorObserver(
    mojo::PendingRemote<crosapi::mojom::EditorObserver>
        pending_observer_remote) {
  observer_remotes_.Add(std::move(pending_observer_remote));
}

void EditorPanelManager::AddObserver(EditorPanelManager::Observer* observer) {
  observers_.AddObserver(observer);
}

void EditorPanelManager::RemoveObserver(
    EditorPanelManager::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void EditorPanelManager::NotifyEditorModeChanged(const EditorMode& mode) {
  for (const mojo::Remote<crosapi::mojom::EditorObserver>& obs :
       observer_remotes_) {
    obs->OnEditorPanelModeChanged(GetEditorPanelMode(mode));
  }
  for (EditorPanelManager::Observer& obs : observers_) {
    obs.OnEditorModeChanged(mode);
  }
}

}  // namespace ash::input_method

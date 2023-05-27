// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/xwalk_autofill_client.h"

#include <string>

#include "base/logging.h"
#include "components/autofill/core/browser/ui/autofill_popup_delegate.h"
#include "components/autofill/core/browser/ui/suggestion.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/ssl_status.h"
#include "ui/gfx/geometry/rect_f.h"
#include "xwalk/runtime/browser/xwalk_browser_context.h"
#include "xwalk/runtime/browser/xwalk_content_browser_client.h"

using content::WebContents;
using base::android::JavaParamRef;

namespace xwalk {

XWalkAutofillClient::XWalkAutofillClient(WebContents* contents)
    : web_contents_(contents), save_form_data_(false) {
}

XWalkAutofillClient::~XWalkAutofillClient() {
}

void XWalkAutofillClient::SetSaveFormData(bool enabled) {
  save_form_data_ = enabled;
}

bool XWalkAutofillClient::GetSaveFormData() {
  return save_form_data_;
}

PrefService* XWalkAutofillClient::GetPrefs() {
  return user_prefs::UserPrefs::Get(
      XWalkBrowserContext::GetDefault());
}

syncer::SyncService* XWalkAutofillClient::GetSyncService() {
  NOTIMPLEMENTED();
  return nullptr;
}

signin::IdentityManager* XWalkAutofillClient::GetIdentityManager() {
  return nullptr;
}

autofill::FormDataImporter* XWalkAutofillClient::GetFormDataImporter() {
  return nullptr;
}

autofill::payments::PaymentsClient* XWalkAutofillClient::GetPaymentsClient() {
  return nullptr;
}

autofill::StrikeDatabase* XWalkAutofillClient::GetStrikeDatabase() {
  return nullptr;
}

//rappor::RapporServiceImpl* XWalkAutofillClient::GetRapporServiceImpl() {
//  return nullptr;
//}

ukm::UkmRecorder* XWalkAutofillClient::GetUkmRecorder() {
  return nullptr;
}

ukm::SourceId XWalkAutofillClient::GetUkmSourceId() {
  // UKM recording is not supported for WebViews.
  return ukm::kInvalidSourceId;
}

autofill::AddressNormalizer* XWalkAutofillClient::GetAddressNormalizer() {
  // TODO(iotto) : Implement
  LOG(WARNING) << __func__ << " not_implemented";
  return nullptr;
}

security_state::SecurityLevel
XWalkAutofillClient::GetSecurityLevelForUmaHistograms() {
  // The metrics are not recorded for Android webview, so return the count value
  // which will not be recorded.
  return security_state::SecurityLevel::SECURITY_LEVEL_COUNT;
}

autofill::PersonalDataManager* XWalkAutofillClient::GetPersonalDataManager() {
  return nullptr;
}

autofill::AutocompleteHistoryManager*
XWalkAutofillClient::GetAutocompleteHistoryManager() {
  // TODO(iotto): implement
  return nullptr;
//  return AwBrowserContext::FromWebContents(web_contents_)
//      ->GetAutocompleteHistoryManager();
}

//scoped_refptr<autofill::AutofillWebDataService>
//XWalkAutofillClient::GetDatabase() {
//  xwalk::XWalkFormDatabaseService* service =
//      static_cast<xwalk::XWalkBrowserContext*>(
//          web_contents_->GetBrowserContext())->GetFormDatabaseService();
//  return service->get_autofill_webdata_service();
//}

void XWalkAutofillClient::HideAutofillPopup() {
  delegate_.reset();
  HideAutofillPopupImpl();
}

void XWalkAutofillClient::ShowAutofillPopup(const gfx::RectF& element_bounds, base::i18n::TextDirection text_direction,
                                            const std::vector<autofill::Suggestion>& suggestions,
                                            bool autoselect_first_suggestion,
                                            autofill::PopupType popup_type,
                                            base::WeakPtr<autofill::AutofillPopupDelegate> delegate) {
  // TODO(iotto): check and fix!
  suggestions_ = suggestions;
  delegate_ = delegate;

  // Convert element_bounds to be in screen space.
  gfx::Rect client_area = web_contents_->GetContainerBounds();
  gfx::RectF element_bounds_in_screen_space = element_bounds + client_area.OffsetFromOrigin();

  ShowAutofillPopupImpl(element_bounds_in_screen_space, text_direction, suggestions);
}

void XWalkAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  // Leaving as an empty method since updating autofill popup window
  // dynamically does not seem to be a useful feature for xwalkview.
  // See crrev.com/18102002 if need to implement.
}

bool XWalkAutofillClient::IsAutocompleteEnabled() {
  return GetSaveFormData();
}

void XWalkAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) {
}

void XWalkAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {
}

//void XWalkAutofillClient::DidInteractWithNonsecureCreditCardInput() {
//  // TODO(iotto) : Implement
//  LOG(WARNING) << __func__ << " not_implemented";
//}

bool XWalkAutofillClient::IsContextSecure() {
  content::SSLStatus ssl_status;
  content::NavigationEntry* navigation_entry =
      web_contents_->GetController().GetLastCommittedEntry();
  if (!navigation_entry)
     return false;

  ssl_status = navigation_entry->GetSSL();
  // Note: The implementation below is a copy of the one in
  // ChromeAutofillClient::IsContextSecure, and should be kept in sync
  // until crbug.com/505388 gets implemented.
  return navigation_entry->GetURL().SchemeIsCryptographic() &&
         ssl_status.certificate &&
         (!net::IsCertStatusError(ssl_status.cert_status) ||
          net::IsCertStatusMinorError(ssl_status.cert_status)) &&
         !(ssl_status.content_status &
           content::SSLStatus::RAN_INSECURE_CONTENT);
}

bool XWalkAutofillClient::ShouldShowSigninPromo() {
  return false;
}

bool XWalkAutofillClient::AreServerCardsSupported() {
  return true;
}

//bool XWalkAutofillClient::IsAutofillSupported() {
//  return false;
//}

/**
 *
 */
void XWalkAutofillClient::ExecuteCommand(int id) {

}

void XWalkAutofillClient::Dismissed(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj) {
  anchor_view_.Reset();
}

void XWalkAutofillClient::SuggestionSelected(int position) {
  if (delegate_) {
    delegate_->DidAcceptSuggestion(suggestions_[position].value,
                                   suggestions_[position].frontend_id,
                                   position);
  }
}

void XWalkAutofillClient::ShowAutofillSettings(bool show_credit_card_settings) {
  // TODO(iotto): maybe implement
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ShowUnmaskPrompt(
    const autofill::CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<autofill::CardUnmaskDelegate> delegate) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::OnUnmaskVerificationResult(PaymentsRpcResult result) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ShowLocalCardMigrationDialog(
    base::OnceClosure show_migration_dialog_closure) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ConfirmMigrateLocalCardToCloud(
    std::unique_ptr<base::DictionaryValue> legal_message, const std::string& user_email,
    const std::vector<autofill::MigratableCreditCard>& migratable_credit_cards,
    LocalCardMigrationCallback start_migrating_cards_callback) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ShowLocalCardMigrationResults(
    const bool has_server_error, const base::string16& tip_message,
    const std::vector<autofill::MigratableCreditCard>& migratable_credit_cards,
    MigrationDeleteCardCallback delete_local_card_callback) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ConfirmSaveAutofillProfile(const autofill::AutofillProfile& profile,
                                                     base::OnceClosure callback) {
  // Since there is no confirmation needed to save an Autofill Profile,
  // running |callback| will proceed with saving |profile|.
  std::move(callback).Run();
}

void XWalkAutofillClient::ConfirmSaveCreditCardLocally(const autofill::CreditCard& card,
                                                       AutofillClient::SaveCreditCardOptions options,
                                                       LocalSaveCardPromptCallback callback) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ConfirmExpirationDateFixFlow(
    const autofill::CreditCard& card, base::OnceCallback<void(const base::string16&, const base::string16&)> callback) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ConfirmAccountNameFixFlow(base::OnceCallback<void(const base::string16&)> callback) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ConfirmSaveCreditCardToCloud(const autofill::CreditCard& card,
                                                       std::unique_ptr<base::DictionaryValue> legal_message,
                                                       SaveCreditCardOptions options,
                                                       UploadSaveCardPromptCallback callback) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::CreditCardUploadCompleted(bool card_saved) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::ConfirmCreditCardFillAssist(const autofill::CreditCard& card, base::OnceClosure callback) {
  NOTIMPLEMENTED();
}

void XWalkAutofillClient::LoadRiskData(base::OnceCallback<void(const std::string&)> callback) {
  NOTIMPLEMENTED();
}

bool XWalkAutofillClient::HasCreditCardScanFeature() {
  return false;
}

void XWalkAutofillClient::ScanCreditCard(
    const CreditCardScanCallback& callback) {
  NOTIMPLEMENTED();
}

//autofill::SaveCardBubbleController* XWalkAutofillClient::GetSaveCardBubbleController() {
//  // TODO (iotto) check if need to implement
//  NOTIMPLEMENTED();
//  return nullptr;
//}

}  // namespace xwalk

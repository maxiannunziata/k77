// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_XWALK_AUTOFILL_CLIENT_H_
#define XWALK_RUNTIME_BROWSER_XWALK_AUTOFILL_CLIENT_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service_factory.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/android/view_android.h"

namespace autofill {
class AutofillMetrics;
class AutofillPopupDelegate;
class CardUnmaskDelegate;
class CreditCard;
class FormStructure;
class PasswordGenerator;
class PersonalDataManager;
struct FormData;
}

namespace content {
class WebContents;
}

namespace gfx {
class RectF;
}

class PersonalDataManager;
class PrefService;

namespace xwalk {

// Manager delegate for the autofill functionality. XWalkView
// supports enabling autocomplete feature for each XWalkView instance
// (different than the browser which supports enabling/disabling for
// a profile). Since there is only one pref service for a given browser
// context, we cannot enable this feature via UserPrefs. Rather, we always
// keep the feature enabled at the pref service, and control it via
// the delegates.
class XWalkAutofillClient : public autofill::AutofillClient {
 public:
  ~XWalkAutofillClient() override;

  void SetSaveFormData(bool enabled);
  bool GetSaveFormData();

  // AutofillClient:
  autofill::PersonalDataManager* GetPersonalDataManager() override;
  autofill::AutocompleteHistoryManager* GetAutocompleteHistoryManager() override;
  // TODO(iotto): move to BrowserContext
//  scoped_refptr<autofill::AutofillWebDataService> GetDatabase() override;
  PrefService* GetPrefs() override;
  syncer::SyncService* GetSyncService() override;
  signin::IdentityManager* GetIdentityManager() override;
  autofill::FormDataImporter* GetFormDataImporter() override;
  autofill::payments::PaymentsClient* GetPaymentsClient() override;
  autofill::StrikeDatabase* GetStrikeDatabase() override;

//  rappor::RapporServiceImpl* GetRapporServiceImpl() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  ukm::SourceId GetUkmSourceId() override;

  // Gets an AddressNormalizer instance (can be null).
  autofill::AddressNormalizer* GetAddressNormalizer() override;
  security_state::SecurityLevel GetSecurityLevelForUmaHistograms() override;
  void ShowAutofillSettings(bool show_credit_card_settings) override;
  void ShowUnmaskPrompt(
      const autofill::CreditCard& card,
      UnmaskCardReason reason,
      base::WeakPtr<autofill::CardUnmaskDelegate> delegate) override;
  void OnUnmaskVerificationResult(PaymentsRpcResult result) override;
  void ShowLocalCardMigrationDialog(base::OnceClosure show_migration_dialog_closure) override;
  void ConfirmMigrateLocalCardToCloud(std::unique_ptr<base::DictionaryValue> legal_message,
                                      const std::string& user_email,
                                      const std::vector<autofill::MigratableCreditCard>& migratable_credit_cards,
                                      LocalCardMigrationCallback start_migrating_cards_callback) override;
  void ShowLocalCardMigrationResults(const bool has_server_error, const base::string16& tip_message,
                                     const std::vector<autofill::MigratableCreditCard>& migratable_credit_cards,
                                     MigrationDeleteCardCallback delete_local_card_callback) override;
  void ConfirmSaveAutofillProfile(const autofill::AutofillProfile& profile, base::OnceClosure callback) override;
  void ConfirmSaveCreditCardLocally(const autofill::CreditCard& card, AutofillClient::SaveCreditCardOptions options,
                                    LocalSaveCardPromptCallback callback) override;
  void ConfirmAccountNameFixFlow(base::OnceCallback<void(const base::string16&)> callback) override;
  void ConfirmExpirationDateFixFlow(const autofill::CreditCard& card,
                                    base::OnceCallback<void(const base::string16&, const base::string16&)> callback)
                                        override;
  void ConfirmSaveCreditCardToCloud(const autofill::CreditCard& card,
                                    std::unique_ptr<base::DictionaryValue> legal_message, SaveCreditCardOptions options,
                                    UploadSaveCardPromptCallback callback) override;
  void CreditCardUploadCompleted(bool card_saved) override;
  void ConfirmCreditCardFillAssist(const autofill::CreditCard& card,
                                   base::OnceClosure callback) override;
  void LoadRiskData(
      base::OnceCallback<void(const std::string&)> callback) override;
  bool HasCreditCardScanFeature() override;
  void ScanCreditCard(const CreditCardScanCallback& callback) override;
  void HideAutofillPopup() override;
  void ShowAutofillPopup(
      const gfx::RectF& element_bounds,
      base::i18n::TextDirection text_direction,
      const std::vector<autofill::Suggestion>& suggestions,
      bool autoselect_first_suggestion,
      autofill::PopupType popup_type,
      base::WeakPtr<autofill::AutofillPopupDelegate> delegate) override;
  void UpdateAutofillPopupDataListValues(
      const std::vector<base::string16>& values,
      const std::vector<base::string16>& labels) override;
  bool IsAutocompleteEnabled() override;
  void PropagateAutofillPredictions(
      content::RenderFrameHost* rfh,
      const std::vector<autofill::FormStructure*>& forms) override;
  void DidFillOrPreviewField(
      const base::string16& autofilled_value,
      const base::string16& profile_full_name) override;
  // TODO(iotto): removed
//  void DidInteractWithNonsecureCreditCardInput() override;
  bool IsContextSecure() override;
  bool ShouldShowSigninPromo() override;
  bool AreServerCardsSupported() override;
//  void StartSigninFlow() override;
//  void ShowHttpNotSecureExplanation() override;
  // Whether Autofill is currently supported by the client. If false, all
  // features of Autofill are disabled, including Autocomplete.
  // TODO(iotto): removed
//  bool IsAutofillSupported() override;

  // Handles simple actions for the autofill popups.
  void ExecuteCommand(int id) override;
  // **************************

  void Dismissed(JNIEnv* env,
                 const base::android::JavaParamRef<jobject>& obj);

  void SuggestionSelected(int position);

  virtual void ShowAutofillPopupImpl(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<autofill::Suggestion>& suggestions) = 0;

  virtual void HideAutofillPopupImpl() = 0;
//
//  // Gets the SaveCardBubbleController instance associated with the client.
//  // May return nullptr if the save card bubble has not been shown yet.
//  autofill::SaveCardBubbleController* GetSaveCardBubbleController() override;

 protected:
  explicit XWalkAutofillClient(content::WebContents* web_contents);

  // The web_contents associated with this delegate.
  content::WebContents* web_contents_;
  base::WeakPtr<autofill::AutofillPopupDelegate> delegate_;
   ui::ViewAndroid::ScopedAnchorView anchor_view_;
   // The current Autofill query values.
   std::vector<autofill::Suggestion> suggestions_;
 private:
  friend class content::WebContentsUserData<XWalkAutofillClient>;

  bool save_form_data_;

  DISALLOW_COPY_AND_ASSIGN(XWalkAutofillClient);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_XWALK_AUTOFILL_CLIENT_H_

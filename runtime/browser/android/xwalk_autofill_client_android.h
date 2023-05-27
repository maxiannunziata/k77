// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_AUTOFILL_CLIENT_ANDROID_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_AUTOFILL_CLIENT_ANDROID_H_

#include <jni.h>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/compiler_specific.h"
#include "xwalk/runtime/browser/xwalk_autofill_client.h"

namespace gfx {
class RectF;
}

namespace xwalk {

// Manager delegate for the autofill functionality. XWalkView
// supports enabling autocomplete feature for each XWalkView instance
// (different than the browser which supports enabling/disabling for
// a profile). Since there is only one pref service for a given browser
// context, we cannot enable this feature via UserPrefs. Rather, we always
// keep the feature enabled at the pref service, and control it via
// the delegates.
class XWalkAutofillClientAndroid : public XWalkAutofillClient,
                                   public content::WebContentsUserData<XWalkAutofillClientAndroid> {
 public:
  void SuggestionSelected(JNIEnv* env, jobject obj, jint position);

  static XWalkAutofillClientAndroid* FromWebContents(content::WebContents* web_contents) {
    return static_cast<XWalkAutofillClientAndroid*>(web_contents->GetUserData(UserDataKey()));
  }
 protected:
//  using content::WebContentsUserData<AwAutofillClient>::FromWebContents;
//  using content::WebContentsUserData<AwAutofillClient>::CreateForWebContents;
//  using content::WebContentsUserData<AwAutofillClient>::UserDataKey;

  explicit XWalkAutofillClientAndroid(content::WebContents* web_contents);
  friend class content::WebContentsUserData<XWalkAutofillClientAndroid>;

  void ShowAutofillPopupImpl(const gfx::RectF& element_bounds,
                             base::i18n::TextDirection text_direction,
                             const std::vector<autofill::Suggestion>& suggestions) override;
  void HideAutofillPopupImpl() override;

  JavaObjectWeakGlobalRef java_ref_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
  DISALLOW_COPY_AND_ASSIGN(XWalkAutofillClientAndroid);
};

bool RegisterXWalkAutofillClient(JNIEnv* env);

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_AUTOFILL_CLIENT_ANDROID_H_

// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_SETTINGS_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_SETTINGS_H_

#include <jni.h>

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
struct WebPreferences;
}

namespace xwalk {

class XWalkRenderViewHostExt;

class XWalkSettings : public content::WebContentsObserver {
 public:
  XWalkSettings(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj, content::WebContents* web_contents);
  ~XWalkSettings() override;

  static XWalkSettings* FromWebContents(content::WebContents* web_contents);

  bool GetAllowThirdPartyCookies();

  // Called from Java.
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void PopulateWebPreferencesLocked(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj, jlong webPrefsPtr);
  void ResetScrollAndScaleState(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateEverythingLocked(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateInitialPageScale(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateUserAgent(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateWebkitPreferencesLocked(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateCookiePolicyLocked(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateAcceptLanguages(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateFormDataPreferences(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void UpdateRendererPreferencesLocked(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  bool GetJavaScriptCanOpenWindowsAutomatically();
  void PopulateWebPreferences(content::WebPreferences* webPrefs);
  bool GetAllowFileAccess();

 private:
  struct FieldIds;

  void WebContentsDestroyed() override;

  XWalkRenderViewHostExt* GetXWalkRenderViewHostExt();
  void UpdateEverything();
  void UpdatePreferredSizeMode();
  PrefService* GetPrefs();

  // WebContentsObserver overrides:
  void RenderViewHostChanged(content::RenderViewHost* old_host, content::RenderViewHost* new_host) override;

  void RenderFrameForInterstitialPageCreated(content::RenderFrameHost* render_frame_host) override;

  // Java field references for accessing the values in the Java object.
  std::unique_ptr<FieldIds> field_ids_;

  JavaObjectWeakGlobalRef xwalk_settings_;

  bool _javascript_can_open_windows_automatically;
  bool renderer_prefs_initialized_;
  bool allow_third_party_cookies_;
};

bool RegisterXWalkSettings(JNIEnv* env);

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_SETTINGS_H_

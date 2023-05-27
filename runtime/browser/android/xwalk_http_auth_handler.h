// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_HTTP_AUTH_HANDLER_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_HTTP_AUTH_HANDLER_H_

#include <jni.h>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
} // namesapce content

namespace net {
class AuthChallengeInfo;
} // namespace net

namespace xwalk {
class XWalkLoginDelegate;
// Native class for Java class of same name and owns an instance
// of that Java object.
class XWalkHttpAuthHandler : public content::LoginDelegate,
                             public content::WebContentsObserver {
 public:
  XWalkHttpAuthHandler(const net::AuthChallengeInfo& auth_info,
                       content::WebContents* web_contents,
                       bool first_auth_attempt,
                       LoginAuthRequiredCallback callback);
  ~XWalkHttpAuthHandler() override;

  void Proceed(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
               const base::android::JavaParamRef<jstring>& username,
               const base::android::JavaParamRef<jstring>& password);
  void Cancel(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  void Start();

  base::android::ScopedJavaGlobalRef<jobject> http_auth_handler_;
  std::string host_;
  std::string realm_;

  LoginAuthRequiredCallback callback_;
  base::WeakPtrFactory<XWalkHttpAuthHandler> weak_factory_;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_HTTP_AUTH_HANDLER_H_

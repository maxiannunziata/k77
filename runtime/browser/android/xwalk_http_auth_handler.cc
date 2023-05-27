// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/xwalk_http_auth_handler.h"

#include "xwalk/runtime/browser/android/xwalk_content.h"
#include "xwalk/runtime/browser/android/xwalk_contents_client_bridge.h"
#include "xwalk/runtime/browser/android/xwalk_login_delegate.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/auth.h"
#include "xwalk/runtime/android/core_refactor/xwalk_refactor_native_jni/XWalkHttpAuthHandler_jni.h"

using base::android::ConvertJavaStringToUTF16;
using base::android::JavaParamRef;
using content::BrowserThread;

namespace xwalk {

XWalkHttpAuthHandler::XWalkHttpAuthHandler(const net::AuthChallengeInfo& auth_info,
                                           content::WebContents* web_contents,
                                           bool first_auth_attempt,
                                           LoginAuthRequiredCallback callback)
    : WebContentsObserver(web_contents),
      host_(auth_info.challenger.host()),
      realm_(auth_info.realm),
      callback_(std::move(callback)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = base::android::AttachCurrentThread();
  http_auth_handler_.Reset(Java_XWalkHttpAuthHandler_create(
      env, reinterpret_cast<intptr_t>(this), first_auth_attempt));

  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&XWalkHttpAuthHandler::Start, weak_factory_.GetWeakPtr()));
}

XWalkHttpAuthHandler:: ~XWalkHttpAuthHandler() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Java_XWalkHttpAuthHandler_handlerDestroyed(base::android::AttachCurrentThread(),
                                          http_auth_handler_);
}

void XWalkHttpAuthHandler::Proceed(JNIEnv* env, const JavaParamRef<jobject>& obj, const JavaParamRef<jstring>& user,
                                   const JavaParamRef<jstring>& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (callback_) {
    std::move(callback_).Run(
        net::AuthCredentials(ConvertJavaStringToUTF16(env, user),
                             ConvertJavaStringToUTF16(env, password)));
  }
}

void XWalkHttpAuthHandler::Cancel(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (callback_) {
    std::move(callback_).Run(base::nullopt);
  }
}

void XWalkHttpAuthHandler::Start() {
  DCHECK(web_contents());
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // The WebContents may have been destroyed during the PostTask.
  if (!web_contents()) {
    std::move(callback_).Run(base::nullopt);
    return;
  }

  XWalkContent* xwalk_content = XWalkContent::FromWebContents(web_contents());
  if (!xwalk_content->GetContentsClientBridge()->OnReceivedHttpAuthRequest(http_auth_handler_, host_, realm_)) {
    std::move(callback_).Run(base::nullopt);
  }
}

}  // namespace xwalk

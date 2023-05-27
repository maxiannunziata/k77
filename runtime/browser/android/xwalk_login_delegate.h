// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_LOGIN_DELEGATE_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_LOGIN_DELEGATE_H_

#include <memory>

#include "xwalk/runtime/browser/android/xwalk_http_auth_handler_base.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/web_contents_observer.h"

namespace net {
class AuthChallengeInfo;
class URLRequest;
}

namespace xwalk {

class XWalkLoginDelegate
    : public content::LoginDelegate,
      public content::WebContentsObserver {
 public:
  XWalkLoginDelegate(const net::AuthChallengeInfo& auth_info, content::WebContents* web_contents, bool first_auth_attempt,
                     LoginAuthRequiredCallback callback);

  virtual void Proceed(const base::string16& user,
                       const base::string16& password);
  virtual void Cancel();

 private:
  ~XWalkLoginDelegate() override;
  void HandleHttpAuthRequestOnUIThread(bool first_auth_attempt);
  void CancelOnIOThread();
  void ProceedOnIOThread(const base::string16& user,
                         const base::string16& password);
  void DeleteAuthHandlerSoon();

  std::unique_ptr<XWalkHttpAuthHandlerBase> xwalk_http_auth_handler_;
  scoped_refptr<net::AuthChallengeInfo> auth_info_;
  net::URLRequest* request_;
  int render_process_id_;
  int render_frame_id_;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_LOGIN_DELEGATE_H_

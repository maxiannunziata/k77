// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_CONTENTS_CLIENT_BRIDGE_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_CONTENTS_CLIENT_BRIDGE_H_

#include <jni.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/containers/id_map.h"
#include "base/supports_user_data.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/resource_request_info.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "xwalk/runtime/browser/android/xwalk_icon_helper.h"

namespace gfx {
class Size;
}

namespace net {
class X509Certificate;
}

namespace blink {
struct NotificationResources;
struct PlatformNotificationData;
}  // namespace blink

namespace content {
class RenderFrameHost;
class WebContents;
}

class SkBitmap;

namespace xwalk {

struct XWalkWebResourceRequest;

// A class that handles the Java<->Native communication for the
// XWalkContentsClient. XWalkContentsClientBridge is created and owned by
// native XWalkViewContents class and it only has a weak reference to the
// its Java peer. Since the Java XWalkContentsClientBridge can have
// indirect refs from the Application (via callbacks) and so can outlive
// XWalkView, this class notifies it before being destroyed and to nullify
// any references.
// TODO(iotto) : Move IconHelper
class XWalkContentsClientBridge : public XWalkIconHelper::Listener {
 public:
  using CertErrorCallback = base::OnceCallback<void(content::CertificateRequestResultType)>;
  // Used to package up information needed by OnReceivedHttpError for transfer
  // between IO and UI threads.
  struct HttpErrorInfo {
    HttpErrorInfo();
    ~HttpErrorInfo();

    int status_code;
    std::string status_text;
    std::string mime_type;
    std::string encoding;
    std::vector<std::string> response_header_names;
    std::vector<std::string> response_header_values;
  };

  // Adds the handler to the UserData registry.
  static void Associate(content::WebContents* web_contents, XWalkContentsClientBridge* handler);
  static XWalkContentsClientBridge* FromWebContents(content::WebContents* web_contents);
  static XWalkContentsClientBridge* FromWebContentsGetter(
      const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter);
  static XWalkContentsClientBridge* FromID(int render_process_id, int render_frame_id);
  static XWalkContentsClientBridge* FromRenderViewID(int render_process_id, int render_view_id);
  static XWalkContentsClientBridge* FromRenderFrameID(int render_process_id, int render_frame_id);
  static XWalkContentsClientBridge* FromRenderFrameHost(content::RenderFrameHost* render_frame_host);

  XWalkContentsClientBridge(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj,
                            content::WebContents* web_contents);
  ~XWalkContentsClientBridge() override;

  void AllowCertificateError(int cert_error, net::X509Certificate* cert, const GURL& request_url,
                             CertErrorCallback callback, bool* cancel_request);

  void RunJavaScriptDialog(content::JavaScriptDialogType dialog_type, const GURL& origin_url,
                           const base::string16& message_text, const base::string16& default_prompt_text,
                           content::JavaScriptDialogManager::DialogClosedCallback callback);
  void RunBeforeUnloadDialog(const GURL& origin_url,
                             content::JavaScriptDialogManager::DialogClosedCallback callback);
  void ShowNotification(const blink::PlatformNotificationData& notification_data,
                        const blink::NotificationResources& notification_resources);
  void OnWebLayoutPageScaleFactorChanged(float page_scale_factor);

  bool OnReceivedHttpAuthRequest(const base::android::JavaRef<jobject>& handler, const std::string& host,
                                 const std::string& realm);
  bool ShouldOverrideUrlLoading(const base::string16& url, bool has_user_gesture, bool is_redirect, bool is_main_frame);

  bool RewriteUrlIfNeeded(const std::string& url, ui::PageTransition transition_type, std::string* new_url);

  void NewDownload(const GURL& url, const std::string& user_agent, const std::string& content_disposition,
                   const std::string& mime_type, int64_t content_length);

  void NewLoginRequest(const std::string& realm, const std::string& account, const std::string& args);

  // Called when a resource loading error has occured (e.g. an I/O error,
  // host name lookup failure etc.)
  void OnReceivedError(const XWalkWebResourceRequest& request, int error_code, bool safebrowsing_hit);
  // Called when a response from the server is received with status code >= 400.
  void OnReceivedHttpError(const XWalkWebResourceRequest& request, std::unique_ptr<HttpErrorInfo> error_info);

  // This should be called from IO thread.
  static std::unique_ptr<HttpErrorInfo> ExtractHttpErrorInfo(const net::HttpResponseHeaders* response_headers);

  // Methods called from Java.
  void ProceedSslError(JNIEnv* env, jobject obj, jboolean proceed, jint id);
  void ConfirmJsResult(JNIEnv*, jobject, int id, jstring prompt);
  void CancelJsResult(JNIEnv*, jobject, int id);
  void NotificationDisplayed(JNIEnv*, jobject, jint id);
  void NotificationClicked(JNIEnv*, jobject, jint id);
  void NotificationClosed(JNIEnv*, jobject, jint id, bool by_user);
  void DownloadIcon(JNIEnv* env, jobject obj, jstring url);

  // XWalkIconHelper::Listener Interface
  void OnIconAvailable(const GURL& icon_url) override;
  void OnReceivedIcon(const GURL& icon_url, const SkBitmap& bitmap) override;

  void ProvideClientCertificateResponse(JNIEnv* env, jobject object,
      jint request_id, 
      const base::android::JavaRef<jobjectArray>& encoded_chain_ref,
      const base::android::JavaRef<jobject>& private_key_ref);

  void SelectClientCertificate(net::SSLCertRequestInfo* cert_request_info,
                               std::unique_ptr<content::ClientCertificateDelegate> delegate);

//  void HandleErrorInClientCertificateResponse(int id);

  void ClearClientCertPreferences(
      JNIEnv*, jobject,
      const base::android::JavaParamRef<jobject>& callback);

  void ClientCertificatesCleared(
      base::android::ScopedJavaGlobalRef<jobject>* callback);

 private:
  JavaObjectWeakGlobalRef java_ref_;

  base::IDMap<std::unique_ptr<CertErrorCallback>> pending_cert_error_callbacks_;
  base::IDMap<std::unique_ptr<content::JavaScriptDialogManager::DialogClosedCallback>>
      pending_js_dialog_callbacks_;
  base::IDMap<std::unique_ptr<content::ClientCertificateDelegate>>
      pending_client_cert_request_delegates_;

  typedef std::pair<int, content::RenderFrameHost*>
    NotificationDownloadRequestInfos;

  std::unique_ptr<XWalkIconHelper> icon_helper_;
};

bool RegisterXWalkContentsClientBridge(JNIEnv* env);

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_CONTENTS_CLIENT_BRIDGE_H_

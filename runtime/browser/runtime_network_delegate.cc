// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/runtime_network_delegate.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "net/base/net_errors.h"
#include "net/base/static_cookie_policy.h"
#include "net/url_request/url_request.h"
#include "meta_logging.h"

#if defined(OS_ANDROID)
#include "xwalk/runtime/browser/android/xwalk_contents_io_thread_client.h"
#include "xwalk/runtime/browser/android/xwalk_cookie_access_policy.h"
#endif

using content::BrowserThread;

namespace xwalk {

RuntimeNetworkDelegate::RuntimeNetworkDelegate() {
}

RuntimeNetworkDelegate::~RuntimeNetworkDelegate() {
}

int RuntimeNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  LOG(WARNING) << "iotto " << __func__ << " move WillSendRequest here!";
  return net::OK;
}

int RuntimeNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    net::CompletionOnceCallback callback,
    net::HttpRequestHeaders* headers) {
  return net::OK;
}

void RuntimeNetworkDelegate::OnStartTransaction(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
}

int RuntimeNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
//  LOG(WARNING) << "iotto " << __func__ << " reinstate";
//  return net::OK;
#if defined(OS_ANDROID)
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int render_process_id, render_frame_id;
  if (content::ResourceRequestInfo::GetRenderFrameForRequest(
      request, &render_process_id, &render_frame_id)) {
    std::unique_ptr<XWalkContentsIoThreadClient> io_thread_client =
        XWalkContentsIoThreadClient::FromID(render_process_id, render_frame_id);
    if (io_thread_client.get()) {
      io_thread_client->OnReceivedResponseHeaders(request,
          original_response_headers);
    }
  }
#endif
  return net::OK;
}

void RuntimeNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                              const GURL& new_location) {
  TENTA_LOG_NET(INFO) << __func__ << " url=" << new_location.spec();
}

void RuntimeNetworkDelegate::OnResponseStarted(net::URLRequest* request, int net_error) {
}

void RuntimeNetworkDelegate::OnNetworkBytesReceived(net::URLRequest* request,
                                                    int64_t bytes_received) {
}

void RuntimeNetworkDelegate::OnCompleted(net::URLRequest* request,
                                         bool started, int net_error) {
}

void RuntimeNetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {
}

void RuntimeNetworkDelegate::OnPACScriptError(int line_number,
                                              const base::string16& error) {
}

RuntimeNetworkDelegate::AuthRequiredResponse RuntimeNetworkDelegate::OnAuthRequired(
    net::URLRequest* request, const net::AuthChallengeInfo& auth_info, AuthCallback callback,
    net::AuthCredentials* credentials) {
  return AUTH_REQUIRED_RESPONSE_NO_ACTION;
}

bool RuntimeNetworkDelegate::OnCanGetCookies(
    const net::URLRequest& request, const net::CookieList& cookie_list, bool allowed_from_caller) {
#if defined(OS_ANDROID)
  return allowed_from_caller && XWalkCookieAccessPolicy::GetInstance()->AllowCookies(request);
#else
  return true;
#endif
}

bool RuntimeNetworkDelegate::OnCanSetCookie(const net::URLRequest& request, const net::CanonicalCookie& cookie,
                                            net::CookieOptions* options, bool allowed_from_caller) {
#if defined(OS_ANDROID)
  return allowed_from_caller && XWalkCookieAccessPolicy::GetInstance()->AllowCookies(request);
#else
  return true;
#endif
}

bool RuntimeNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request, const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  return true;
}

}  // namespace xwalk

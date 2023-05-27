// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_RUNTIME_NETWORK_DELEGATE_H_
#define XWALK_RUNTIME_BROWSER_RUNTIME_NETWORK_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "net/base/network_delegate_impl.h"

namespace xwalk {

class RuntimeNetworkDelegate : public net::NetworkDelegateImpl {
 public:
  RuntimeNetworkDelegate();
  ~RuntimeNetworkDelegate() override;

 private:
  // net::NetworkDelegate implementation.
  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers) override;
  void OnStartTransaction(net::URLRequest* request,
                          const net::HttpRequestHeaders& headers) override;
  int OnHeadersReceived(
      net::URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnNetworkBytesReceived(net::URLRequest* request,
                              int64_t bytes_received) override;
  void OnCompleted(net::URLRequest* request, bool started, int net_error) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void OnPACScriptError(int line_number,
                        const base::string16& error) override;
  AuthRequiredResponse OnAuthRequired(net::URLRequest* request, const net::AuthChallengeInfo& auth_info,
                                      AuthCallback callback, net::AuthCredentials* credentials) override;
  bool OnCanGetCookies(const net::URLRequest& request, const net::CookieList& cookie_list, bool allowed_from_caller)
      override;
  bool OnCanSetCookie(const net::URLRequest& request,
                      const net::CanonicalCookie& cookie,
                      net::CookieOptions* options,
                      bool allowed_from_caller) override;
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& original_path,
                       const base::FilePath& absolute_path) const override;

  DISALLOW_COPY_AND_ASSIGN(RuntimeNetworkDelegate);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_RUNTIME_NETWORK_DELEGATE_H_

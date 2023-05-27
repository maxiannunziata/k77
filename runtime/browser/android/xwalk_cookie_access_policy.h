// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_COOKIE_ACCESS_POLICY_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_COOKIE_ACCESS_POLICY_H_

#include <string>

#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"
#include "net/cookies/canonical_cookie.h"

namespace content {
class ResourceContext;
}

namespace net {
class CookieOptions;
class URLRequest;
}

class GURL;

namespace xwalk {

// Manages the cookie access (both setting and getting) policy for WebView.
class XWalkCookieAccessPolicy {
 public:
  static XWalkCookieAccessPolicy* GetInstance();

  // These manage the global access state shared across requests regardless of
  // source (i.e. network or JavaScript).
  bool GetShouldAcceptCookies();
//  bool GetGlobalAllowAccess();

  void SetGlobalAllowAccess(bool allow);

  // Can we read/write third party cookies?
  // |render_process_id| and |render_frame_id| must be valid.
  // Navigation requests are not associated with a renderer process. In this
  // case, |frame_tree_node_id| must be valid instead. Can only be called from
  // the IO thread.
  bool GetShouldAcceptThirdPartyCookies(int render_process_id, int render_frame_id, int frame_tree_node_id);
  bool GetShouldAcceptThirdPartyCookies(const net::URLRequest& request);

  // Whether or not to allow cookies to bet sent or set for |request|. Can only
  // be called from the IO thread.
  bool AllowCookies(const net::URLRequest& request);

  // Whether or not to allow cookies for requests with these parameters.
  bool AllowCookies(const GURL& url, const GURL& first_party, int render_process_id, int render_frame_id);

 private:
  friend struct base::LazyInstanceTraitsBase<XWalkCookieAccessPolicy>;

  XWalkCookieAccessPolicy();
  ~XWalkCookieAccessPolicy();

  bool CanAccessCookies(const GURL& url, const GURL& site_for_cookies, bool accept_third_party_cookies);

  bool accept_cookies_;
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(XWalkCookieAccessPolicy);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_COOKIE_ACCESS_POLICY_H_

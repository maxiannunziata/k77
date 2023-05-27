// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/xwalk_cookie_access_policy.h"

#include <memory>

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "net/base/static_cookie_policy.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/features.h"
#include "xwalk/runtime/browser/android/xwalk_contents_io_thread_client.h"

using base::AutoLock;
using content::BrowserThread;
using content::ResourceRequestInfo;
using content::WebSocketHandshakeRequestInfo;

namespace xwalk {

namespace {
base::LazyInstance<XWalkCookieAccessPolicy>::Leaky g_lazy_instance;
}  // namespace

XWalkCookieAccessPolicy::~XWalkCookieAccessPolicy() {
}

XWalkCookieAccessPolicy::XWalkCookieAccessPolicy()
    : accept_cookies_(true) {
}

XWalkCookieAccessPolicy* XWalkCookieAccessPolicy::GetInstance() {
  return g_lazy_instance.Pointer();
}

bool XWalkCookieAccessPolicy::GetShouldAcceptCookies() {
  AutoLock lock(lock_);
  return accept_cookies_;
}

void XWalkCookieAccessPolicy::SetGlobalAllowAccess(bool allow) {
  AutoLock lock(lock_);
  accept_cookies_ = allow;
}

bool XWalkCookieAccessPolicy::GetShouldAcceptThirdPartyCookies(
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::unique_ptr<XWalkContentsIoThreadClient> io_thread_client =
      (frame_tree_node_id != content::RenderFrameHost::kNoFrameTreeNodeId)
          ? XWalkContentsIoThreadClient::FromID(frame_tree_node_id)
          : XWalkContentsIoThreadClient::FromID(render_process_id,
                                             render_frame_id);

  if (!io_thread_client) {
    return false;
  }
  return io_thread_client->ShouldAcceptThirdPartyCookies();
}

bool XWalkCookieAccessPolicy::GetShouldAcceptThirdPartyCookies(const net::URLRequest& request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int child_id = 0;
  int render_frame_id = 0;
  int frame_tree_node_id = content::RenderFrameHost::kNoFrameTreeNodeId;
  ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(&request);
  if (info) {
    child_id = info->GetChildID();
    render_frame_id = info->GetRenderFrameID();
    frame_tree_node_id = info->GetFrameTreeNodeId();
  } else {
    WebSocketHandshakeRequestInfo* websocket_info = WebSocketHandshakeRequestInfo::ForRequest(&request);
    if (!websocket_info)
      return false;
    child_id = websocket_info->GetChildId();
    render_frame_id = websocket_info->GetRenderFrameId();
  }
  return GetShouldAcceptThirdPartyCookies(child_id, render_frame_id, frame_tree_node_id);
}

bool XWalkCookieAccessPolicy::AllowCookies(const net::URLRequest& request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!base::FeatureList::IsEnabled(network::features::kNetworkService));
  bool third_party = GetShouldAcceptThirdPartyCookies(request);
  return CanAccessCookies(request.url(), request.site_for_cookies(), third_party);
}

bool XWalkCookieAccessPolicy::AllowCookies(const GURL& url, const GURL& first_party, int render_process_id,
                                           int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool third_party = GetShouldAcceptThirdPartyCookies(render_process_id, render_frame_id,
                                                      content::RenderFrameHost::kNoFrameTreeNodeId);
  return CanAccessCookies(url, first_party, third_party);
}

bool XWalkCookieAccessPolicy::CanAccessCookies(const GURL& url,
                                            const GURL& site_for_cookies,
                                            bool accept_third_party_cookies) {
  if (!accept_cookies_)
    return false;

  if (accept_third_party_cookies)
    return true;

  // File URLs are a special case. We want file URLs to be able to set cookies
  // but (for the purpose of cookies) Chrome considers different file URLs to
  // come from different origins so we use the 'allow all' cookie policy for
  // file URLs.
  if (url.SchemeIsFile())
    return true;

  // Otherwise, block third-party cookies.
  return net::StaticCookiePolicy(
             net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES)
             .CanAccessCookies(url, site_for_cookies) == net::OK;
}

}  // namespace xwalk

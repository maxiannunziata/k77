// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/renderer/android/xwalk_permission_client.h"

#include "content/public/common/url_constants.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/gurl.h"

namespace xwalk {

XWalkPermissionClient::XWalkPermissionClient(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetWebFrame()->SetContentSettingsClient(this);
}

XWalkPermissionClient::~XWalkPermissionClient() {
}

void XWalkPermissionClient::OnDestruct() {
  delete this;
}

bool XWalkPermissionClient::AllowDatabase() {
  return true;
}

bool XWalkPermissionClient::AllowIndexedDB(const blink::WebSecurityOrigin&) {
  return true;
}

bool XWalkPermissionClient::AllowStorage(bool local) {
  return true;
}

}  // namespace xwalk

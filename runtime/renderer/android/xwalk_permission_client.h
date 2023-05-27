// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_RENDERER_ANDROID_XWALK_PERMISSION_CLIENT_H_
#define XWALK_RUNTIME_RENDERER_ANDROID_XWALK_PERMISSION_CLIENT_H_

#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

namespace xwalk {

class XWalkPermissionClient : public content::RenderFrameObserver,
                              public blink::WebContentSettingsClient {
 public:
  explicit XWalkPermissionClient(content::RenderFrame* render_view);

 private:
  ~XWalkPermissionClient() override;

  // content::RenderFrameObserver implementation.
  void OnDestruct() override;

  // blink::WebContentSettingsClient
  bool AllowDatabase() override;
  bool AllowIndexedDB(const blink::WebSecurityOrigin&) override;
  bool AllowStorage(bool local) override;

  DISALLOW_COPY_AND_ASSIGN(XWalkPermissionClient);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_RENDERER_ANDROID_XWALK_PERMISSION_CLIENT_H_

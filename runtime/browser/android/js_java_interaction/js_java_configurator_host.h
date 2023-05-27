// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_JS_JAVA_INTERACTION_JS_JAVA_CONFIGURATOR_HOST_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_JS_JAVA_INTERACTION_JS_JAVA_CONFIGURATOR_HOST_H_

#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/proxy_resolution/proxy_bypass_rules.h"
#include "services/network/public/mojom/proxy_config.mojom.h"
#include "url/origin.h"
#include "xwalk/runtime/common/js_java_interaction/interfaces.mojom.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace xwalk {

// This class is 1:1 with WebContents, when SetJsApiService is called, it stores
// the information in this class and send them to renderer side
// JsJavaConfigurator if there is any. When RenderFrameCreated() gets called, it
// needs to configure that new RenderFrame with the information stores in this
// class.
class JsJavaConfiguratorHost : public content::WebContentsObserver {
 public:
  explicit JsJavaConfiguratorHost(content::WebContents* web_contents);
  ~JsJavaConfiguratorHost() override;

  base::android::ScopedJavaLocalRef<jstring> SetJsApiService(
      JNIEnv* env, bool need_to_inject_js_object, const base::android::JavaParamRef<jstring>& js_object_name,
      const base::android::JavaParamRef<jobjectArray>& allowed_origin_rules);

  bool IsOriginAllowedForOnPostMessage(const url::Origin& origin);

  // content::WebContentsObserver implementations
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;

 private:
  void NotifyFrame(content::RenderFrameHost* render_frame_host);

  bool need_to_inject_js_object_ = false;
  std::string js_object_name_;
  // We use ProxyBypassRules because it has the functionality that suitable
  // here, but it is not for proxy bypass.
  net::ProxyBypassRules allowed_origin_rules_;

  DISALLOW_COPY_AND_ASSIGN(JsJavaConfiguratorHost)
  ;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_JS_JAVA_INTERACTION_JS_JAVA_CONFIGURATOR_HOST_H_

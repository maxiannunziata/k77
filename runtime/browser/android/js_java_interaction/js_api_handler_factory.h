// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_JS_JAVA_INTERACTION_JS_API_HANDLER_FACTORY_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_JS_JAVA_INTERACTION_JS_API_HANDLER_FACTORY_H_

#include <map>

#include "base/supports_user_data.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "xwalk/runtime/common/js_java_interaction/interfaces.mojom.h"

namespace content {
class WebContents;
}  // namespace content

namespace xwalk {

class JsApiHandler;

// A WebContentsObserver that creates JsApiHandler for each RenderFrameHost. It
// also destroy the corresponding JsApiHandler when that RenderFrameHost has
// been deleted. It also works as a broker, passes message binding to the
// correct JsApiHandler.
class JsApiHandlerFactory : public content::WebContentsObserver,
                            public base::SupportsUserData::Data {
 public:
  ~JsApiHandlerFactory() override;

  static JsApiHandlerFactory* FromWebContents(content::WebContents* contents);

  // Finds the corresponding JsApiHandler from |render_frame_host| and
  // dispatches the |pending_receiver| to it.
  static void BindJsApiHandler(mojo::PendingAssociatedReceiver<mojom::JsApiHandler> pending_receiver,
                               content::RenderFrameHost* render_frame_host);

  // content::WebContentsObserver
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

 private:
  explicit JsApiHandlerFactory(content::WebContents* web_contents);

  JsApiHandler* JsApiHandlerForFrame(content::RenderFrameHost* render_frame_host);

  std::map<content::RenderFrameHost*, std::unique_ptr<JsApiHandler>> frame_map_;

  DISALLOW_COPY_AND_ASSIGN(JsApiHandlerFactory)
  ;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_JS_JAVA_INTERACTION_JS_API_HANDLER_FACTORY_H_

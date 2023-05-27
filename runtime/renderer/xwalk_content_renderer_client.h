// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_RENDERER_XWALK_CONTENT_RENDERER_CLIENT_H_
#define XWALK_RUNTIME_RENDERER_XWALK_CONTENT_RENDERER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file.h"
#include "base/strings/string16.h"
#include "content/public/renderer/content_renderer_client.h"
#include "media/mojo/buildflags.h"
#include "ui/base/page_transition_types.h"
#include "xwalk/extensions/renderer/xwalk_extension_renderer_controller.h"

#if defined(OS_ANDROID)
#include "xwalk/runtime/renderer/android/xwalk_render_thread_observer.h"
#else
#include "xwalk/runtime/renderer/xwalk_render_thread_observer_generic.h"
#endif

namespace visitedlink {
class VisitedLinkSlave;
}

namespace web_cache {
class WebCacheImpl;
}

namespace xwalk {

class XWalkRenderThreadObserver;

// When implementing a derived class, make sure to update
// `in_process_browser_test.cc` and `xwalk_main_delegate.cc`.
class XWalkContentRendererClient :
    public content::ContentRendererClient
    /*,public extensions::XWalkExtensionRendererController::Delegate*/ {
 public:
  static XWalkContentRendererClient* Get();

  XWalkContentRendererClient();
  ~XWalkContentRendererClient() override;

  // ContentRendererClient implementation.
  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RenderViewCreated(content::RenderView* render_view) override;
  bool IsExternalPepperPlugin(const std::string& module_name) override;
  uint64_t VisitedLinkHash(const char* canonical_url, size_t length) override;
  bool IsLinkVisited(uint64_t link_hash) override;

  void WillSendRequest(blink::WebLocalFrame* frame, ui::PageTransition transition_type, const blink::WebURL& url,
                       const url::Origin* initiator_origin, GURL* new_url,
                       bool* attach_same_site_cookies) override;

  void AddSupportedKeySystems(std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) override;

  bool ShouldReportDetailedMessageForSource(const base::string16& source) override;

  // Returns true if the embedder has an error page to show for the given http
  // status code. If so |error_domain| should be set to according to WebURLError
  // and the embedder's GetNavigationErrorHtml will be called afterwards to get
  // the error html.
  bool HasErrorPage(int http_status_code) override;

#if defined(OS_ANDROID)
  bool HandleNavigation(content::RenderFrame* render_frame, bool is_content_initiated,
                        bool render_view_was_created_by_renderer, blink::WebFrame* frame,
                        const blink::WebURLRequest& request, blink::WebNavigationType type,
                        blink::WebNavigationPolicy default_policy, bool is_redirect) override;
#endif

 protected:
  std::unique_ptr<XWalkRenderThreadObserver> xwalk_render_thread_observer_;

 private:
  // XWalkExtensionRendererController::Delegate implementation.
//  void DidCreateModuleSystem(extensions::XWalkModuleSystem* module_system) override;

//  std::unique_ptr<extensions::XWalkExtensionRendererController> extension_controller_;

  std::unique_ptr<visitedlink::VisitedLinkSlave> visited_link_slave_;

  void GetErrorDescription(const blink::WebURLError& error, const std::string& http_method,
                           base::string16* error_description) override;
  void PrepareErrorPage(content::RenderFrame* render_frame, const blink::WebURLError& error,
                        const std::string& http_method,
                        bool ignoring_cache,
                        std::string* error_html) override;
  void PrepareErrorPageForHttpStatusError(content::RenderFrame* render_frame, const GURL& unreachable_url,
                                          const std::string& http_method,
                                          bool ignoring_cache,
                                          int http_status, std::string* error_html) override;

//  std::unique_ptr<web_cache::WebCacheImpl> web_cache_impl_;

  DISALLOW_COPY_AND_ASSIGN(XWalkContentRendererClient)
  ;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_RENDERER_XWALK_CONTENT_RENDERER_CLIENT_H_

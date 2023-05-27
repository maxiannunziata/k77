// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/renderer_host/xwalk_render_view_host_ext.h"

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/logging.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_frame_host.h"
//#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/frame_navigate_params.h"
#include "content/public/browser/navigation_handle.h"
#include "xwalk/runtime/browser/android/xwalk_contents_client_bridge.h"
#include "xwalk/runtime/browser/xwalk_browser_context.h"
#include "xwalk/runtime/common/android/xwalk_render_view_messages.h"

#include "meta_logging.h"

namespace xwalk {

XWalkRenderViewHostExt::XWalkRenderViewHostExt(content::WebContents* contents)
    : content::WebContentsObserver(contents),
      has_new_hit_test_data_(false),
      is_render_view_created_(false),
      background_color_(SK_ColorWHITE){
}

XWalkRenderViewHostExt::~XWalkRenderViewHostExt() {}

void XWalkRenderViewHostExt::DocumentHasImages(DocumentHasImagesResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!web_contents()->GetRenderViewHost()) {
    result.Run(false);
    return;
  }
  static int next_id = 1;
  int this_id = next_id++;

  // Send the message to the main frame, instead of the whole frame tree,
  // because it only makes sense on the main frame.
  if (web_contents()->GetMainFrame()->Send(new XWalkViewMsg_DocumentHasImages(
          web_contents()->GetMainFrame()->GetRoutingID(), this_id))) {
    pending_document_has_images_requests_[this_id] = result;
  } else {
    // Still have to respond to the API call WebView#docuemntHasImages.
    // Otherwise the listener of the response may be starved.
    result.Run(false);
  }
}

void XWalkRenderViewHostExt::ClearCache() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetRenderViewHost()->Send(new XWalkViewMsg_ClearCache);
}

// TODO(iotto) : Implement!!!
//void AwRenderViewHostExt::KillRenderProcess() {
//  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
//  web_contents()->GetRenderViewHost()->Send(new AwViewMsg_KillProcess);
//}

bool XWalkRenderViewHostExt::HasNewHitTestData() const {
  return has_new_hit_test_data_;
}

void XWalkRenderViewHostExt::MarkHitTestDataRead() {
  has_new_hit_test_data_ = false;
}

void XWalkRenderViewHostExt::RequestNewHitTestDataAt(
    const gfx::PointF& touch_center,
    const gfx::SizeF& touch_area) {
  TENTA_LOG(INFO) << "iotto " << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // We only need to get blink::WebView on the renderer side to invoke the
  // blink hit test API, so sending this IPC to main frame is enough.
  web_contents()->GetMainFrame()->Send(
      new XWalkViewMsg_DoHitTest(web_contents()->GetMainFrame()->GetRoutingID(),
                              touch_center, touch_area));
}

const XWalkHitTestData& XWalkRenderViewHostExt::GetLastHitTestData() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return last_hit_test_data_;
}

void XWalkRenderViewHostExt::SetTextZoomLevel(double level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  web_contents()->GetMainFrame()->Send(
      new XWalkViewMsg_SetTextZoomLevel(web_contents()->GetMainFrame()->GetRoutingID(), level));
}

void XWalkRenderViewHostExt::ResetScrollAndScaleState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  web_contents()->GetMainFrame()->Send(
      new XWalkViewMsg_ResetScrollAndScaleState(web_contents()->GetMainFrame()->GetRoutingID()));
}

void XWalkRenderViewHostExt::SetInitialPageScale(double page_scale_factor) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetMainFrame()->Send(
      new XWalkViewMsg_SetInitialPageScale(web_contents()->GetMainFrame()->GetRoutingID(), page_scale_factor));
}

void XWalkRenderViewHostExt::RenderViewCreated(content::RenderViewHost* render_view_host) {
  web_contents()->GetMainFrame()->Send(
      new XWalkViewMsg_SetBackgroundColor(web_contents()->GetMainFrame()->GetRoutingID(), background_color_));

  if (!pending_base_url_.empty()) {
    web_contents()->GetRenderViewHost()->Send(new XWalkViewMsg_SetOriginAccessWhitelist(
        pending_base_url_, pending_match_patterns_));
  }
  is_render_view_created_ = true;
}

void XWalkRenderViewHostExt::SetJsOnlineProperty(bool network_up) {
  web_contents()->GetRenderViewHost()->Send(new XWalkViewMsg_SetJsOnlineProperty(network_up));
}

void XWalkRenderViewHostExt::RenderProcessGone(base::TerminationStatus status) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (std::map<int, DocumentHasImagesResult>::iterator pending_req =
           pending_document_has_images_requests_.begin();
       pending_req != pending_document_has_images_requests_.end();
      ++pending_req) {
    pending_req->second.Run(false);
  }
}

void XWalkRenderViewHostExt::DidFinishNavigation(content::NavigationHandle* navigation_handle) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!navigation_handle->HasCommitted() ||
      (!navigation_handle->IsInMainFrame() &&
       !navigation_handle->HasSubframeNavigationEntryCommitted()))
    return;

  XWalkBrowserContext::FromWebContents(web_contents())->AddVisitedURLs(navigation_handle->GetRedirectChain());
}

void XWalkRenderViewHostExt::OnPageScaleFactorChanged(float page_scale_factor) {
  XWalkContentsClientBridge* client_bridge = XWalkContentsClientBridge::FromWebContents(web_contents());
  if (client_bridge != NULL)
    client_bridge->OnWebLayoutPageScaleFactorChanged(page_scale_factor);
}

bool XWalkRenderViewHostExt::OnMessageReceived(
    const IPC::Message& message, content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(XWalkRenderViewHostExt, message, render_frame_host)
    IPC_MESSAGE_HANDLER(XWalkViewHostMsg_DocumentHasImagesResponse,
                        OnDocumentHasImagesResponse)
    IPC_MESSAGE_HANDLER(XWalkViewHostMsg_UpdateHitTestData,
                        OnUpdateHitTestData)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled ? true : WebContentsObserver::OnMessageReceived(message);
}

void XWalkRenderViewHostExt::OnDocumentHasImagesResponse(
    content::RenderFrameHost* render_frame_host, int msg_id, bool has_images) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::map<int, DocumentHasImagesResult>::iterator pending_req =
      pending_document_has_images_requests_.find(msg_id);
  if (pending_req == pending_document_has_images_requests_.end()) {
    DLOG(WARNING) << "unexpected DocumentHasImages Response: " << msg_id;
  } else {
    pending_req->second.Run(has_images);
    pending_document_has_images_requests_.erase(pending_req);
  }
}

void XWalkRenderViewHostExt::OnUpdateHitTestData(content::RenderFrameHost* render_frame_host,
                                                 const XWalkHitTestData& hit_test_data) {
  content::RenderFrameHost* main_frame_host = render_frame_host;
  while (main_frame_host->GetParent())
    main_frame_host = main_frame_host->GetParent();

  // Make sense from any frame of the current frame tree, because a focused
  // node could be in either the mainframe or a subframe.
  if (main_frame_host != web_contents()->GetMainFrame())
    return;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TENTA_LOG(INFO) << "iotto " << __func__ << " hitTestDataType=" << hit_test_data.type;

  last_hit_test_data_ = hit_test_data;
  has_new_hit_test_data_ = true;
}

void XWalkRenderViewHostExt::SetOriginAccessWhitelist(
    const std::string& base_url,
    const std::string& match_patterns) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  pending_base_url_ = base_url;
  pending_match_patterns_ = match_patterns;

  if (is_render_view_created_) {
    web_contents()->GetRenderViewHost()->Send(new XWalkViewMsg_SetOriginAccessWhitelist(
        pending_base_url_, pending_match_patterns_));
  }
}

void XWalkRenderViewHostExt::SetBackgroundColor(SkColor c) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (background_color_ == c)
    return;
  background_color_ = c;
  if (web_contents()->GetRenderViewHost()) {
    web_contents()->GetMainFrame()->Send(
        new XWalkViewMsg_SetBackgroundColor(web_contents()->GetMainFrame()->GetRoutingID(), background_color_));
  }
}

void XWalkRenderViewHostExt::SetTextZoomFactor(float factor) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  web_contents()->GetMainFrame()->Send(
      new XWalkViewMsg_SetTextZoomFactor(web_contents()->GetMainFrame()->GetRoutingID(), factor));
}

}  // namespace xwalk

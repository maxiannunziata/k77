// Copyright (c) 2016 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/xwalk_presentation_service_delegate.h"

#include <string>
#include <vector>

#include "base/guid.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/presentation_screen_availability_listener.h"
#include "content/public/browser/presentation_request.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "xwalk/application/browser/application_system.h"
#include "xwalk/application/browser/application_service.h"
#include "xwalk/runtime/browser/runtime.h"
#include "xwalk/runtime/browser/xwalk_browser_context.h"
#include "xwalk/runtime/browser/xwalk_runner.h"
#include "xwalk/runtime/browser/xwalk_presentation_service_helper.h"

namespace xwalk {

XWalkPresentationServiceDelegate::XWalkPresentationServiceDelegate(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

XWalkPresentationServiceDelegate::~XWalkPresentationServiceDelegate() {}

void XWalkPresentationServiceDelegate::AddObserver(int render_process_id,
                                                   int render_frame_id,
                                                   Observer* observer) {
  RenderFrameHostId id(render_process_id, render_frame_id);
  PresentationFrame* presentation_frame = GetOrAddPresentationFrame(id);
  presentation_frame->set_delegate_observer(observer);
}

void XWalkPresentationServiceDelegate::RemoveObserver(int render_process_id,
                                                      int render_frame_id) {
  RenderFrameHostId id(render_process_id, render_frame_id);
  
  auto it = presentation_frames_.find(id);
  if ( it != presentation_frames_.end() ) {
    PresentationFrame* presentation_frame = it->second.get();
    if (presentation_frame) {
      presentation_frame->set_delegate_observer(nullptr);
    }
    presentation_frames_.erase(it);
  }
}

bool XWalkPresentationServiceDelegate::AddScreenAvailabilityListener(
    int render_process_id,
    int render_frame_id,
    PresentationScreenAvailabilityListener* listener) {
  DCHECK(listener);
  RenderFrameHostId id(render_process_id, render_frame_id);
  PresentationFrame* presentation_frame = GetOrAddPresentationFrame(id);
  return presentation_frame->SetScreenAvailabilityListener(listener);
}

void XWalkPresentationServiceDelegate::RemoveScreenAvailabilityListener(
    int render_process_id,
    int render_frame_id,
    PresentationScreenAvailabilityListener* listener) {
  DCHECK(listener);
  RenderFrameHostId id(render_process_id, render_frame_id);
  auto it = presentation_frames_.find(id);
  if ( it != presentation_frames_.end() ) {
    PresentationFrame* presentation_frame = it->second.get();
    if ( presentation_frame) {
      presentation_frame->RemoveScreenAvailabilityListener(listener);
    }
  }
}

void XWalkPresentationServiceDelegate::Reset(int render_process_id,
                                             int render_frame_id) {
  RenderFrameHostId id(render_process_id, render_frame_id);

  auto it = presentation_frames_.find(id);
  if ( it != presentation_frames_.end() ) {
    PresentationFrame* presentation_frame = it->second.get();
    if ( presentation_frame) {
      presentation_frame->Reset();
    }
  }
}

void XWalkPresentationServiceDelegate::SetDefaultPresentationUrls(
    const content::PresentationRequest& request,
    content::DefaultPresentationConnectionCallback callback) {

  // TODO(iotto) :chromium code; see how to adapt to xwalk
//  if (request.presentation_urls.empty()) {
//    ClearDefaultPresentationRequest();
//    return;
//  }
//  DCHECK(!callback.is_null());
//  default_presentation_started_callback_ = std::move(callback);
//  default_presentation_request_ = request;
//  for (auto& observer : default_presentation_request_observers_)
//    observer.OnDefaultPresentationChanged(*default_presentation_request_);

  const auto& render_frame_host_id = request.render_frame_host_id;
  const auto& presentation_urls = request.presentation_urls;

  PresentationFrame* presentation_frame = GetOrAddPresentationFrame(render_frame_host_id);
// TODO(iotto) default_presentation_urls is a vector
  presentation_frame->set_default_presentation_url(presentation_urls[0].spec());
}

void XWalkPresentationServiceDelegate::OnSessionStarted(
    const RenderFrameHostId& id,
    PresentationConnectionCallback success_cb,
    PresentationConnectionErrorCallback error_cb,
    scoped_refptr<PresentationSession> session,
    const std::string& error) {
  auto it = presentation_frames_.find(id);
  if ( it != presentation_frames_.end() ) {
    PresentationFrame* presentation_frame = it->second.get();
    if (presentation_frame && session) {
      presentation_frame->OnPresentationSessionStarted(session);
      std::move(success_cb).Run(session->session_info());
      return;
    }
  }
  std::move(error_cb).Run(
      content::PresentationError(content::PRESENTATION_ERROR_UNKNOWN, error));
}

void XWalkPresentationServiceDelegate::ReconnectPresentation(
    const content::PresentationRequest& request,
    const std::string& presentation_id,
    content::PresentationConnectionCallback success_cb,
    content::PresentationConnectionErrorCallback error_cb) {

  const auto& presentation_urls = request.presentation_urls;
  const auto& render_frame_host_id = request.render_frame_host_id;

  auto it = presentation_frames_.find(render_frame_host_id);
  if ( it != presentation_frames_.end() ) {
    PresentationFrame* presentation_frame = it->second.get();

    CHECK(presentation_frame);

    for (auto& frame : presentation_frames_) {
      PresentationSession* session = frame.second->session();
      if ( session != nullptr ) {
// TODO(iotto) use presentation_urls full vector
        if (session->session_info().presentation_id == presentation_id &&
            session->session_info().presentation_url == presentation_urls[0]) {
          presentation_frame->OnPresentationSessionStarted(session);
          SessionInfo info(presentation_urls[0], presentation_id);
          std::move(success_cb).Run(info);
          return;
        }
      }
    }
  }

  std::move(error_cb).Run(content::PresentationError(
      content::PRESENTATION_ERROR_NO_PRESENTATION_FOUND,
      "There is no session with id: " + presentation_id + ", and URL: " +
          presentation_urls[0].spec()));
}

void XWalkPresentationServiceDelegate::Terminate(
    int render_process_id,
    int render_frame_id,
    const std::string& presentation_id) {
  CloseConnection(render_process_id, render_frame_id, presentation_id);
}

void XWalkPresentationServiceDelegate::CloseConnection(
    int render_process_id,
    int render_frame_id,
    const std::string& presentation_id) {
  RenderFrameHostId id(render_process_id, render_frame_id);

  auto it = presentation_frames_.find(id);
  if ( it != presentation_frames_.end() ) {
    PresentationFrame* presentation_frame = it->second.get();
    PresentationSession* session = presentation_frame->session();
    if (session != nullptr ) {
      session->Close();
    }
  }
}

void XWalkPresentationServiceDelegate::ListenForConnectionStateChange(
    int render_process_id,
    int render_frame_id,
    const content::PresentationInfo& connection,
    const PresentationConnectionStateChangedCallback& state_changed_cb) {
  RenderFrameHostId id(render_process_id, render_frame_id);
  PresentationFrame* presentation_frame = GetOrAddPresentationFrame(id);
  presentation_frame->ListenForSessionStateChange(state_changed_cb);
}

PresentationFrame* XWalkPresentationServiceDelegate::GetOrAddPresentationFrame(
    const RenderFrameHostId& render_frame_host_id) {
  auto it = presentation_frames_.find(render_frame_host_id);

  if (it == presentation_frames_.end()) {
    presentation_frames_[render_frame_host_id] =
                             PresentationFrame::Create(render_frame_host_id);
  }
  return presentation_frames_[render_frame_host_id].get();
}

}  // namespace xwalk

// Copyright (c) 2016 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_XWALK_PRESENTATION_SERVICE_DELEGATE_H_
#define XWALK_RUNTIME_BROWSER_XWALK_PRESENTATION_SERVICE_DELEGATE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace xwalk {

class PresentationFrame;
class PresentationSession;
class XWalkBrowserContext;

class XWalkPresentationServiceDelegate
    : public content::ControllerPresentationServiceDelegate {
 public:
  using RenderFrameHostId = std::pair<int, int>;

  ~XWalkPresentationServiceDelegate() override;

 protected:
  explicit XWalkPresentationServiceDelegate(content::WebContents* web_contents);

 public:
  void AddObserver(int render_process_id,
                   int render_frame_id,
                   Observer* observer) override;

  void RemoveObserver(int render_process_id, int render_frame_id) override;

  bool AddScreenAvailabilityListener(
      int render_process_id,
      int render_frame_id,
      content::PresentationScreenAvailabilityListener* listener) override;

  void RemoveScreenAvailabilityListener(
      int render_process_id,
      int render_frame_id,
      content::PresentationScreenAvailabilityListener* listener) override;

  void Reset(int render_process_id, int render_frame_id) override;

  void SetDefaultPresentationUrls(
      const content::PresentationRequest& request,
      content::DefaultPresentationConnectionCallback callback) override;

  void ReconnectPresentation(
      const content::PresentationRequest& request,
      const std::string& presentation_id,
      content::PresentationConnectionCallback success_cb,
      content::PresentationConnectionErrorCallback error_cb) override;

  void CloseConnection(int render_process_id,
                       int render_frame_id,
                       const std::string& presentation_id) override;

  void Terminate(int render_process_id,
                 int render_frame_id,
                 const std::string& presentation_id) override;

  void ListenForConnectionStateChange(
      int render_process_id,
      int render_frame_id,
      const blink::mojom::PresentationInfo& connection,
      const content::PresentationConnectionStateChangedCallback&
          state_changed_cb) override;

  void OnSessionStarted(
      const RenderFrameHostId& id,
      content::PresentationConnectionCallback success_cb,
      content::PresentationConnectionErrorCallback error_cb,
      scoped_refptr<PresentationSession> session,
      const std::string& error);
  // Connect |controller_connection| owned by the controlling frame to the
  // offscreen presentation represented by |session|.
  // |render_process_id|, |render_frame_id|: ID of originating frame.
  // |controller_connection|: Pointer to controller's presentation connection,
  // ownership passed from controlling frame to the offscreen presentation.
  // |receiver_connection_request|: Mojo InterfaceRequest to be bind to receiver
  // page's presentation connection.
  void StartPresentation(
      const content::PresentationRequest& request,
      content::PresentationConnectionPtr controller_connection_ptr,
      content::PresentationConnectionRequest receiver_connection_request) override {}
  void StartPresentation(
      const content::PresentationRequest& request,
      content::PresentationConnectionCallback success_cb,
      content::PresentationConnectionErrorCallback error_cb) override;
 protected:
  PresentationFrame* GetOrAddPresentationFrame(
      const RenderFrameHostId& render_frame_host_id);

  content::WebContents* web_contents_;
  std::map<RenderFrameHostId, std::unique_ptr<PresentationFrame>>
      presentation_frames_;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_XWALK_PRESENTATION_SERVICE_DELEGATE_H_

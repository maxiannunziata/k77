// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
#define XWALK_RUNTIME_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "content/public/browser/media_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/media_stream_request.h"

namespace xwalk {
// This singleton is used to receive updates about media events from the content
// layer. Based on chrome/browser/media/media_capture_devices_dispatcher.[h|cc].
class XWalkMediaCaptureDevicesDispatcher : public content::MediaObserver {
 public:
  class Observer {
   public:
    // Handle an information update consisting of a up-to-date audio capture
    // device lists. This happens when a microphone is plugged in or unplugged.
    virtual void OnUpdateAudioDevices(
        const blink::MediaStreamDevices& devices) {}

    // Handle an information update consisting of a up-to-date video capture
    // device lists. This happens when a camera is plugged in or unplugged.
    virtual void OnUpdateVideoDevices(
        const blink::MediaStreamDevices& devices) {}

    // Handle an information update related to a media stream request.
    virtual void OnRequestUpdate(
        int render_process_id,
        int render_frame_id,
        blink::mojom::MediaStreamType stream_type,
        const content::MediaRequestState state) {}

    virtual ~Observer() {}
  };

  static XWalkMediaCaptureDevicesDispatcher* GetInstance();

  static void RunRequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback);

  // Methods for observers. Called on UI thread.
  // Observers should add themselves on construction and remove themselves
  // on destruction.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  const blink::MediaStreamDevices& GetAudioCaptureDevices();
  const blink::MediaStreamDevices& GetVideoCaptureDevices();

  // Helper for picking the device that was requested for an OpenDevice request.
  // If the device requested is not available it will revert to using the first
  // available one instead or will return an empty list if no devices of the
  // requested kind are present.
  void GetRequestedDevice(const std::string& requested_audio_device_id,
                          const std::string& requested_video_device_id,
                          bool audio,
                          bool video,
                          blink::MediaStreamDevices* devices);

  // Overridden from content::MediaObserver:
  void OnAudioCaptureDevicesChanged() override;
  void OnVideoCaptureDevicesChanged() override;
  void OnMediaRequestStateChanged(
      int render_process_id,
      int render_frame_id,
      int page_request_id,
      const GURL& security_origin,
      blink::mojom::MediaStreamType stream_type,
      content::MediaRequestState state) override;
  void OnCreatingAudioStream(int render_process_id,
                             int render_view_id) override {}
  void OnSetCapturingLinkSecured(int render_process_id,
                                 int render_frame_id,
                                 int page_request_id,
                                 blink::mojom::MediaStreamType stream_type,
                                 bool is_secure) override {}

  // Only for testing.
  void SetTestAudioCaptureDevices(const blink::MediaStreamDevices& devices);
  void SetTestVideoCaptureDevices(const blink::MediaStreamDevices& devices);

 private:
  friend struct
      base::DefaultSingletonTraits<XWalkMediaCaptureDevicesDispatcher>;

  XWalkMediaCaptureDevicesDispatcher();
  ~XWalkMediaCaptureDevicesDispatcher() override;

#if !defined (OS_ANDROID)
  static void OnPermissionRequestFinished(
      const content::MediaResponseCallback& callback,
      const content::MediaStreamRequest& request,
      content::WebContents* web_contents,
      bool success);
  static void RequestPermissionToUser(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback);
#endif
  // Called by the MediaObserver() functions, executed on UI thread.
  void NotifyAudioDevicesChangedOnUIThread();
  void NotifyVideoDevicesChangedOnUIThread();
  void UpdateMediaReqStateOnUIThread(
      int render_process_id,
      int render_frame_id,
      const GURL& security_origin,
      blink::mojom::MediaStreamType stream_type,
      content::MediaRequestState state);

  // Only for testing, a list of cached audio capture devices.
  blink::MediaStreamDevices test_audio_devices_;

  // Only for testing, a list of video audio capture devices.
  blink::MediaStreamDevices test_video_devices_;

  // A list of observers for the device update notifications.
  base::ObserverList<Observer>::Unchecked observers_;
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_

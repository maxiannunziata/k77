// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/xwalk_presentation_service_delegate_android.h"
#include "xwalk/runtime/browser/xwalk_presentation_service_helper_android.h"
#include "content/public/browser/presentation_request.h"

#include <string>

WEB_CONTENTS_USER_DATA_KEY_IMPL(
    xwalk::XWalkPresentationServiceDelegateAndroid);

namespace xwalk {

XWalkPresentationServiceDelegateAndroid::
    XWalkPresentationServiceDelegateAndroid(content::WebContents* web_contents)
    : XWalkPresentationServiceDelegate(web_contents) {}

XWalkPresentationServiceDelegateAndroid::
    ~XWalkPresentationServiceDelegateAndroid() {}

content::ControllerPresentationServiceDelegate*
XWalkPresentationServiceDelegateAndroid::GetOrCreateForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);

  // CreateForWebContents does nothing if the delegate instance already exists.
  XWalkPresentationServiceDelegateAndroid::CreateForWebContents(web_contents);
  return XWalkPresentationServiceDelegateAndroid::FromWebContents(web_contents);
}

void XWalkPresentationServiceDelegateAndroid::StartPresentation(
    const content::PresentationRequest& request,
    content::PresentationConnectionCallback success_cb,
    content::PresentationConnectionErrorCallback error_cb) {

  const auto& render_frame_host_id = request.render_frame_host_id;
  const std::vector<GURL>& presentation_urls = request.presentation_urls;
  if (presentation_urls.empty()) {
    std::move(error_cb).Run(
        content::PresentationError(content::PRESENTATION_ERROR_UNKNOWN,
                                   "Invalid presentation arguments."));
    return;
  }

  // TODO(crbug.com/670848): Improve handling of invalid URLs in
  // PresentationService::start().
  if (std::find_if_not(presentation_urls.begin(), presentation_urls.end(),
                       IsValidPresentationUrl) != presentation_urls.end()) {
    std::move(error_cb).Run(content::PresentationError(
        content::PRESENTATION_ERROR_NO_PRESENTATION_FOUND,
        "Invalid presentation URL."));
    return;
  }


  const DisplayInfo* available_monitor =
  DisplayInfoManager::GetInstance()->FindAvailable();
  if (!available_monitor) {
    std::move(error_cb).Run(content::PresentationError(
            content::PRESENTATION_ERROR_NO_AVAILABLE_SCREENS,
            "No available monitors"));
    return;
  }

  const std::string& presentation_id = base::GenerateGUID();

  PresentationSession::CreateParams params = {};
  params.display_info = *available_monitor;
  params.presentation_id = presentation_id;
  params.presentation_url = presentation_urls[0].spec();
  params.web_contents = web_contents_;
  params.render_process_id = render_frame_host_id.first;
  params.render_frame_id = render_frame_host_id.second;
  params.application = nullptr;

  auto callback =
  base::BindOnce(&XWalkPresentationServiceDelegate::OnSessionStarted,
      AsWeakPtr(), render_frame_host_id, std::move(success_cb), std::move(error_cb));

  PresentationSessionAndroid::Create(params, std::move(callback));

//  base::Callback<void(scoped_refptr<PresentationSession>,
//                      const std::string& error)>;

}

}  // namespace xwalk

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/runtime_javascript_dialog_manager.h"

#include <string>

#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_ANDROID)
#include "xwalk/runtime/browser/android/xwalk_contents_client_bridge.h"
#endif  // defined(OS_ANDROID)

namespace xwalk {

RuntimeJavaScriptDialogManager::RuntimeJavaScriptDialogManager() {
}

RuntimeJavaScriptDialogManager::~RuntimeJavaScriptDialogManager() {
}

void RuntimeJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* render_frame_host,
    content::JavaScriptDialogType javascript_dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
#if defined(OS_ANDROID)
  XWalkContentsClientBridge* bridge =
      XWalkContentsClientBridge::FromWebContents(web_contents);
  if (!bridge) {
    std::move(callback).Run(false, base::string16());
    return;
  }
  bridge->RunJavaScriptDialog(javascript_dialog_type,
                              render_frame_host->GetLastCommittedURL(),
                              message_text,
                              default_prompt_text,
                              std::move(callback));
#else
  *did_suppress_message = true;
  NOTIMPLEMENTED();
#endif
}

void RuntimeJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* render_frame_host,
    bool is_reload,
    DialogClosedCallback callback) {
#if defined(OS_ANDROID)
  XWalkContentsClientBridge* bridge =
      XWalkContentsClientBridge::FromWebContents(web_contents);
  bridge->RunBeforeUnloadDialog(web_contents->GetURL(),
                                std::move(callback));
#else
  NOTIMPLEMENTED();
  callback.Run(true, base::string16());
  return;
#endif
}

void RuntimeJavaScriptDialogManager::CancelDialogs(
    content::WebContents* web_contents,
    bool reset_state) {
  NOTIMPLEMENTED();
}

}  // namespace xwalk

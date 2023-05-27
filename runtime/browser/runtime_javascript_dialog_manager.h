// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_RUNTIME_JAVASCRIPT_DIALOG_MANAGER_H_
#define XWALK_RUNTIME_BROWSER_RUNTIME_JAVASCRIPT_DIALOG_MANAGER_H_

#include <string>

#include "content/public/browser/javascript_dialog_manager.h"

namespace xwalk {

class RuntimeJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  RuntimeJavaScriptDialogManager();
  ~RuntimeJavaScriptDialogManager() override;

  void RunJavaScriptDialog(
      content::WebContents* web_contents,
      content::RenderFrameHost* render_frame_host,
      content::JavaScriptDialogType javascript_dialog_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      DialogClosedCallback callback,
      bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(
      content::WebContents* web_contents,
      content::RenderFrameHost* render_frame_host,
      bool is_reload,
      DialogClosedCallback callback) override;
  void CancelDialogs(
      content::WebContents* web_contents,
      bool reset_state) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RuntimeJavaScriptDialogManager);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_RUNTIME_JAVASCRIPT_DIALOG_MANAGER_H_

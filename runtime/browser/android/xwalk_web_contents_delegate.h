// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_WEB_CONTENTS_DELEGATE_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_WEB_CONTENTS_DELEGATE_H_

#include <jni.h>
#include <string>

#include "components/embedder_support/android/delegate/web_contents_delegate_android.h"

namespace xwalk {

extern const int kFileChooserModeOpenMultiple;
extern const int kFileChooserModeOpenFolder;

class XWalkWebContentsDelegate
    : public web_contents_delegate_android::WebContentsDelegateAndroid {
 public:
  XWalkWebContentsDelegate(JNIEnv* env, jobject obj);
  ~XWalkWebContentsDelegate() override;

  void AddNewContents(content::WebContents* source,
                      std::unique_ptr<content::WebContents> new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;

  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;
  void UpdatePreferredSize(content::WebContents* web_contents, const gfx::Size& pref_size) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      std::unique_ptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;

  content::JavaScriptDialogManager* GetJavaScriptDialogManager(content::WebContents* web_contents) override;

  void RequestMediaAccessPermission(content::WebContents* web_contents, const content::MediaStreamRequest& request,
                                    content::MediaResponseCallback callback) override;
  void RendererUnresponsive(content::WebContents* source, content::RenderWidgetHost* render_widget_host,
                            base::RepeatingClosure hang_monitor_restarter) override;
  void RendererResponsive(content::WebContents* source, content::RenderWidgetHost* render_widget_host) override;
  bool DidAddMessageToConsole(
      content::WebContents* source, blink::mojom::ConsoleMessageLevel log_level, const base::string16& message,
      int32_t line_no, const base::string16& source_id) override;
  bool HandleKeyboardEvent(content::WebContents* source, const content::NativeWebKeyboardEvent& event) override;
  void ShowRepostFormWarningDialog(content::WebContents* source) override;
  void EnterFullscreenModeForTab(content::WebContents* web_contents, const GURL& origin,
                                 const blink::WebFullscreenOptions& options) override;

  void ExitFullscreenModeForTab(content::WebContents* web_contents) override;
  bool IsFullscreenForTabOrPending(const content::WebContents* web_contents) override;

  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;

  void FindReply(content::WebContents* web_contents, int request_id, int number_of_matches,
                 const gfx::Rect& selection_rect, int active_match_ordinal,
                 bool final_update) override;

  void NavigationStateChanged(content::WebContents* source, content::InvalidateTypes changed_flags) override;
  void LoadingStateChanged(content::WebContents* source, bool to_different_document) override;
  bool ShouldResumeRequestsForCreatedWindow() override;
  void SetOverlayMode(bool useOverlayMode) override;
  content::PictureInPictureResult EnterPictureInPicture(content::WebContents* web_contents, const viz::SurfaceId&,
                                                        const gfx::Size& natural_size) override;

  std::unique_ptr<content::FileSelectListener> TakeFileSelectListener();
 private:
  std::unique_ptr<content::JavaScriptDialogManager> javascript_dialog_manager_;
  // Maintain a FileSelectListener instance passed to RunFileChooser() until
  // a callback is called.
  std::unique_ptr<content::FileSelectListener> file_select_listener_;
  DISALLOW_COPY_AND_ASSIGN(XWalkWebContentsDelegate)
  ;
};

bool RegisterXWalkWebContentsDelegate(JNIEnv* env);

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_ANDROID_XWALK_WEB_CONTENTS_DELEGATE_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_EXTENSIONS_RENDERER_XWALK_EXTENSION_RENDERER_CONTROLLER_H_
#define XWALK_EXTENSIONS_RENDERER_XWALK_EXTENSION_RENDERER_CONTROLLER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/synchronization/waitable_event.h"
#include "content/public/renderer/render_thread_observer.h"
#include "third_party/blink/public/web/web_frame.h"
#include "v8/include/v8.h"

namespace content {
class RenderView;
}

namespace IPC {
struct ChannelHandle;
class SyncChannel;
}

namespace blink {
class WebFrame;
}

namespace xwalk {
namespace extensions {

class XWalkExtensionClient;
class XWalkModuleSystem;

// Renderer controller for XWalk extensions keeps track of the extensions
// registered into the system. It also watches for new render views to attach
// the extensions handlers to them.
class XWalkExtensionRendererController : public content::RenderThreadObserver {
 public:
  struct Delegate {
    // Allows external code to register extra modules to the module
    // system. It will be called for every module system created.
    virtual void DidCreateModuleSystem(XWalkModuleSystem* module_system) = 0;
   protected:
    ~Delegate() {}
  };

  explicit XWalkExtensionRendererController(Delegate* delegate);
  ~XWalkExtensionRendererController() override;

  // To be called in XWalkContentRendererClient so we can create and
  // destroy extensions contexts appropriatedly.
  void DidCreateScriptContext(blink::WebLocalFrame* frame,
                              v8::Handle<v8::Context> context);
  void WillReleaseScriptContext(blink::WebLocalFrame* frame,
                                v8::Handle<v8::Context> context);

  // RenderThreadObserver implementation.
  bool OnControlMessageReceived(const IPC::Message& message) override;
  // TODO(iotto): Check how to signal event
  // shutdown isn't called see commits
  // bbfdd9f0669c9856883ffbf2cd9909e2a4df9dcf
  // 8dbd83c66912ffb764d8a2b7e4eb3a88f9c9be16
//  void OnRenderProcessShutdown() override;

 private:
  void SetupBrowserProcessClient(IPC::SyncChannel* browser_channel);

  // We use the browser_channel to ask for the handle to setup the extension
  // channel and plug the external_extensions_client_ into it.
  void SetupExtensionProcessClient(IPC::SyncChannel* browser_channel);

  std::unique_ptr<XWalkExtensionClient> in_browser_process_extensions_client_;
  std::unique_ptr<XWalkExtensionClient> external_extensions_client_;

  base::WaitableEvent shutdown_event_;
  std::unique_ptr<IPC::SyncChannel> extension_process_channel_;
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(XWalkExtensionRendererController);
};

}  // namespace extensions
}  // namespace xwalk

#endif  // XWALK_EXTENSIONS_RENDERER_XWALK_EXTENSION_RENDERER_CONTROLLER_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/extensions/renderer/xwalk_extension_renderer_controller.h"

#include "base/command_line.h"
#include "base/values.h"
#include "content/public/renderer/v8_value_converter.h"
#include "content/public/renderer/render_thread.h"
#include "grit/xwalk_extensions_resources.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sync_channel.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "v8/include/v8.h"
#include "xwalk/extensions/common/xwalk_extension_messages.h"
#include "xwalk/extensions/common/xwalk_extension_switches.h"
#include "xwalk/extensions/renderer/xwalk_extension_client.h"
#include "xwalk/extensions/renderer/xwalk_extension_module.h"
#include "xwalk/extensions/renderer/xwalk_js_module.h"
#include "xwalk/extensions/renderer/xwalk_module_system.h"
#include "xwalk/extensions/renderer/xwalk_v8tools_module.h"

namespace xwalk {
namespace extensions {

const GURL kAboutBlankURL = GURL("about:blank");

XWalkExtensionRendererController::XWalkExtensionRendererController(
    Delegate* delegate)
    : shutdown_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                      base::WaitableEvent::InitialState::NOT_SIGNALED),
      delegate_(delegate) {
  content::RenderThread* thread = content::RenderThread::Get();
  thread->AddObserver(this);
  IPC::SyncChannel* browser_channel = thread->GetChannel();
  SetupBrowserProcessClient(browser_channel);

  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kXWalkDisableExtensionProcess)){
#if TENTA_LOG_ENABLE == 1
    LOG(INFO) << "EXTENSION PROCESS DISABLED.";
#endif
  }  else
    SetupExtensionProcessClient(browser_channel);
}

XWalkExtensionRendererController::~XWalkExtensionRendererController() {
  // FIXME(cmarcelo): These call is causing crashes on shutdown with Chromium
  //                  29.0.1547.57 and had to be commented out.
  // content::RenderThread::Get()->RemoveObserver(this);
}

namespace {

void CreateExtensionModules(XWalkExtensionClient* client,
                            XWalkModuleSystem* module_system) {
  const XWalkExtensionClient::ExtensionAPIMap& extensions =
      client->extension_apis();
  XWalkExtensionClient::ExtensionAPIMap::const_iterator it = extensions.begin();
  for (; it != extensions.end(); ++it) {
    XWalkExtensionClient::ExtensionCodePoints* codepoint = it->second.get();
    if (codepoint->api.empty())
      continue;
    std::unique_ptr<XWalkExtensionModule> module(
        new XWalkExtensionModule(client, module_system,
                                 it->first, codepoint->api));
    module_system->RegisterExtensionModule(std::move(module),
                                           codepoint->entry_points);
  }
}

}  // namespace

void XWalkExtensionRendererController::DidCreateScriptContext(
    blink::WebLocalFrame* frame, v8::Handle<v8::Context> context) {
  XWalkModuleSystem* module_system = new XWalkModuleSystem(context);
  XWalkModuleSystem::SetModuleSystemInContext(
      std::unique_ptr<XWalkModuleSystem>(module_system), context);

  module_system->RegisterNativeModule(
      "v8tools", std::unique_ptr<XWalkNativeModule>(new XWalkV8ToolsModule));
  module_system->RegisterNativeModule(
      "internal", CreateJSModuleFromResource(
          IDR_XWALK_EXTENSIONS_INTERNAL_API));

  module_system->RegisterNativeModule(
      "jsStub", CreateJSModuleFromResource(
          IDR_XWALK_EXTENSIONS_JS_STUB_WRAPPER));

  delegate_->DidCreateModuleSystem(module_system);

  CreateExtensionModules(in_browser_process_extensions_client_.get(),
                         module_system);

  if (external_extensions_client_) {
    CreateExtensionModules(external_extensions_client_.get(),
                           module_system);
  }

  module_system->Initialize();
}

void XWalkExtensionRendererController::WillReleaseScriptContext(
    blink::WebLocalFrame* frame, v8::Handle<v8::Context> context) {
  v8::Context::Scope contextScope(context);
  XWalkModuleSystem::ResetModuleSystemFromContext(context);
}

bool XWalkExtensionRendererController::OnControlMessageReceived(
    const IPC::Message& message) {
  return in_browser_process_extensions_client_->OnMessageReceived(message);
}

// TODO(iotto): Check how to signal event
// shutdown isn't called see commits
// bbfdd9f0669c9856883ffbf2cd9909e2a4df9dcf
// 8dbd83c66912ffb764d8a2b7e4eb3a88f9c9be16
//void XWalkExtensionRendererController::OnRenderProcessShutdown() {
//  shutdown_event_.Signal();
//}

void XWalkExtensionRendererController::SetupBrowserProcessClient(
    IPC::SyncChannel* browser_channel) {
  in_browser_process_extensions_client_.reset(new XWalkExtensionClient);
  in_browser_process_extensions_client_->Initialize(browser_channel);
}

void XWalkExtensionRendererController::SetupExtensionProcessClient(
    IPC::SyncChannel* browser_channel) {
  IPC::ChannelHandle handle;
  browser_channel->Send(
      new XWalkExtensionProcessHostMsg_GetExtensionProcessChannel(&handle));
  // FIXME(cmarcelo): Need to account for failure in creating the channel.

  external_extensions_client_.reset(new XWalkExtensionClient);
  extension_process_channel_ = IPC::SyncChannel::Create(handle,
      IPC::Channel::MODE_CLIENT, external_extensions_client_.get(),
      content::RenderThread::Get()->GetIOTaskRunner(),
      base::ThreadTaskRunnerHandle::Get(), true,
      &shutdown_event_);

  external_extensions_client_->Initialize(extension_process_channel_.get());
}


}  // namespace extensions
}  // namespace xwalk

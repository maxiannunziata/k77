// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/extensions/extension_process/xwalk_extension_process.h"

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "content/public/common/mojo_channel_switches.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_sync_channel.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "xwalk/extensions/common/xwalk_extension_messages.h"


namespace xwalk {
namespace extensions {

XWalkExtensionProcess::XWalkExtensionProcess(
    const mojo::edk::NamedPlatformHandle& channel_handle)
    : shutdown_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                      base::WaitableEvent::InitialState::NOT_SIGNALED),
      io_thread_("XWalkExtensionProcess_IOThread") {
  io_thread_.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  extensions_server_.set_permissions_delegate(this);
  CreateBrowserProcessChannel(channel_handle);
}

XWalkExtensionProcess::~XWalkExtensionProcess() {
  // FIXME(jeez): Move this to OnChannelClosing/Error/Disconnected when we have
  // our MessageFilter set.
  extensions_server_.Invalidate();

  shutdown_event_.Signal();
  io_thread_.Stop();
}

bool XWalkExtensionProcess::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(XWalkExtensionProcess, message)
    IPC_MESSAGE_HANDLER(XWalkExtensionProcessMsg_RegisterExtensions,
                        OnRegisterExtensions)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

namespace {

void ToValueMap(base::ListValue* lv, base::DictionaryValue::DictStorage* vm) {
  vm->clear();

  for (base::ListValue::iterator it = lv->begin(); it != lv->end(); it++) {
    base::DictionaryValue* dv;
    if (!it->GetAsDictionary(&dv))
      continue;
    for (base::DictionaryValue::Iterator dit(*dv);
        !dit.IsAtEnd(); dit.Advance())
      (*vm)[dit.key()] = base::WrapUnique(dit.value().DeepCopy());
  }
}

}  // namespace

void XWalkExtensionProcess::OnRegisterExtensions(
    const base::FilePath& path, const base::ListValue& browser_variables_lv) {
  if (!path.empty()) {
    std::unique_ptr<base::DictionaryValue::DictStorage> browser_variables(
      new base::DictionaryValue::DictStorage);

    ToValueMap(&const_cast<base::ListValue&>(browser_variables_lv),
          browser_variables.get());

    RegisterExternalExtensionsInDirectory(&extensions_server_, path,
                                          std::move(browser_variables));
  }
  CreateRenderProcessChannel();
}

void XWalkExtensionProcess::CreateBrowserProcessChannel(
    const mojo::edk::NamedPlatformHandle& channelHandle) {
/*  if (channel_handle.name.empty()) {
    std::string channel_id =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kProcessChannelID);
    browser_process_channel_ = IPC::SyncChannel::Create(
        channel_id, IPC::Channel::MODE_CLIENT, this,
        io_thread_.task_runner(), true, &shutdown_event_);
  } else {

    browser_process_channel_ = IPC::SyncChannel::Create(
        channel_handle, IPC::Channel::MODE_CLIENT, this,
        io_thread_.task_runner(), true, &shutdown_event_);
  }
*/

 // TODO(iotto) see implementation details in content/child/child_thread_impl.cc
  // ChildThreadImpl::ConnectChannel()
  // ChildThreadImpl::Init
  std::string channel_token;
  mojo::ScopedMessagePipeHandle handle;

  channel_token = base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(switches::kServiceRequestChannelToken);
//  channel_token = base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(switches::kMojoChannelToken);
//  handle = mojo::edk::CreateChildMessagePipe(channel_token);

    auto invitation =
      mojo::edk::IncomingBrokerClientInvitation::AcceptFromCommandLine(
          mojo::edk::TransportProtocol::kLegacy);
    mojo::ScopedMessagePipeHandle mojo_handle = invitation->ExtractMessagePipe(channel_token);

//  +  auto invitation =
//  +      mojo::edk::IncomingBrokerClientInvitation::AcceptFromCommandLine(
//  +          mojo::edk::TransportProtocol::kLegacy);


//  -  mojo::ScopedMessagePipeHandle mojo_handle =
//  -      mojo::edk::CreateChildMessagePipe(
//  -          base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
//  -              switches::kMojoChannelToken));

//  +  auto invitation = mojo::edk::IncomingBrokerClientInvitation::Accept(
//  +      mojo::edk::ConnectionParams(
//  +          mojo::edk::TransportProtocol::kLegacy,
//  +          mojo::edk::ScopedPlatformHandle(mojo::edk::PlatformHandle(
//  +              kMojoIPCChannel + base::GlobalDescriptors::kBaseDescriptor))));
//  +  mojo::ScopedMessagePipeHandle mojo_handle = invitation->ExtractMessagePipe(
//  +      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
//  +          switches::kMojoChannelToken));



  browser_process_channel_ = IPC::SyncChannel::Create(mojo_handle.release(),
                                      IPC::Channel::MODE_CLIENT,
                                      this,  // As a Listener.
                                      io_thread_.task_runner(),
                                      base::ThreadTaskRunnerHandle::Get(),
                                      true,  // Create pipe now.
                                      &shutdown_event_);


//  mojo::edk::ScopedPlatformHandle parent_pipe =
//      mojo::edk::PlatformChannelPair::PassClientHandleFromParentProcess(
//          *command_line);
//  if (!parent_pipe.is_valid()) {
//    parent_pipe =
//        mojo::edk::NamedPlatformChannelPair::PassClientHandleFromParentProcess(
//            *command_line);
//  }
//  if (!parent_pipe.is_valid()) {
//    return kInvalidCommandLineExitCode;
//  }
}

void XWalkExtensionProcess::CreateRenderProcessChannel() {
/*  IPC::ChannelHandle handle(IPC::Channel::GenerateVerifiedChannelID(
      std::string()));
  rp_channel_handle_ = handle;

  render_process_channel_ = IPC::SyncChannel::Create(rp_channel_handle_,
      IPC::Channel::MODE_SERVER, &extensions_server_,
      io_thread_.task_runner(), true, &shutdown_event_);
*/
  mojo::MessagePipe pipe;

  render_process_channel_ = IPC::SyncChannel::Create(pipe.handle0.release(),
                                      IPC::Channel::MODE_SERVER,
                                      &extensions_server_,
                                      io_thread_.task_runner(),
                                      base::ThreadTaskRunnerHandle::Get(),
                                      true, &shutdown_event_);


/*#if defined(OS_POSIX)
    // On POSIX, pass the server-side file descriptor. We use
    // TakeClientFileDescriptor() instead of GetClientFileDescriptor()
    // since the client-side channel will take ownership of the fd.
    rp_channel_handle_.socket = base::FileDescriptor(
      render_process_channel_->TakeClientFileDescriptor());
#endif
*/
  extensions_server_.Initialize(render_process_channel_.get());

  browser_process_channel_->Send(
      new XWalkExtensionProcessHostMsg_RenderProcessChannelCreated(
          pipe.handle1.release()));
}

bool XWalkExtensionProcess::CheckAPIAccessControl(
    const std::string& extension_name,
    const std::string& api_name) {
  PermissionCacheType::iterator iter =
      permission_cache_.find(extension_name + api_name);
  if (iter != permission_cache_.end())
    return iter->second != ALLOW_ONCE;

  RuntimePermission result = UNDEFINED_RUNTIME_PERM;
  browser_process_channel_->Send(
      new XWalkExtensionProcessHostMsg_CheckAPIAccessControl(
          extension_name, api_name, &result));
  DLOG(INFO) << extension_name << "." << api_name << "() --> " << result;
  if (result == ALLOW_SESSION ||
      result == ALLOW_ALWAYS ||
      result == DENY_SESSION ||
      result == DENY_ALWAYS) {
    permission_cache_[extension_name+api_name] = result;
    return (result == ALLOW_SESSION || result == ALLOW_ALWAYS);
  }

  // Could be allow/deny once or undefined here.
  return (result == ALLOW_ONCE);
}

bool XWalkExtensionProcess::RegisterPermissions(
    const std::string& extension_name,
    const std::string& perm_table) {
  bool result = false;
  browser_process_channel_->Send(
      new XWalkExtensionProcessHostMsg_RegisterPermissions(
          extension_name, perm_table, &result));
  return result;
}

}  // namespace extensions
}  // namespace xwalk

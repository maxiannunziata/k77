// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/devtools/remote_debugging_server.h"

#include <map>
#include <vector>

#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/devtools/devtools_http_handler.h"
//#include "components/devtools_http_handler/devtools_http_handler_delegate.h"
#include "content/public/browser/devtools_manager_delegate.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "grit/xwalk_resources.h"
#include "net/base/net_errors.h"
#include "net/socket/tcp_server_socket.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/snapshot/snapshot.h"
#include "xwalk/runtime/browser/xwalk_browser_context.h"
#include "xwalk/runtime/common/xwalk_content_client.h"

using content::DevToolsAgentHost;
using content::RenderViewHost;
using content::RenderWidgetHostView;
using content::WebContents;

namespace xwalk {

class TCPServerSocketFactory
    : public content::DevToolsSocketFactory {
 public:
  TCPServerSocketFactory(
    const std::string& address, int port, int backlog)
    : address_(address)
    , backlog_(backlog)
    , port_(port) {
  }

 private:
  // devtools_http_handler::DevToolsHttpHandler::ServerSocketFactory.
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    std::unique_ptr<net::ServerSocket> socket(
        new net::TCPServerSocket(NULL, net::NetLogSource()));
    if (socket->ListenWithAddressAndPort(address_, port_, backlog_) != net::OK)
      return std::unique_ptr<net::ServerSocket>();
    return socket;
  }
    // Creates a named socket for reversed tethering implementation (used with
  // remote debugging, primarily for mobile).
  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* out_name) override {
    return std::unique_ptr<net::ServerSocket>();
  }

  std::string address_;
  int backlog_;
  int port_;
  DISALLOW_COPY_AND_ASSIGN(TCPServerSocketFactory);
};

class XWalkDevToolsHttpHandlerDelegate :
    public content::DevToolsManagerDelegate {
 public:
  XWalkDevToolsHttpHandlerDelegate();
  ~XWalkDevToolsHttpHandlerDelegate() override;

  // DevToolsHttpHandlerDelegate implementation.
  std::string GetTargetDescription(content::WebContents* web_contents) override;
  content::DevToolsAgentHost::List RemoteDebuggingTargets() override;
  std::string GetDiscoveryPageHTML() override;
  bool IsBrowserTargetDiscoverable() override;

  using ThumbnailMap = std::map<GURL, std::string>;
  ThumbnailMap thumbnail_map_;

  base::WeakPtrFactory<XWalkDevToolsHttpHandlerDelegate> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(XWalkDevToolsHttpHandlerDelegate);
};

XWalkDevToolsHttpHandlerDelegate::XWalkDevToolsHttpHandlerDelegate()
    : weak_factory_(this) {
}

XWalkDevToolsHttpHandlerDelegate::~XWalkDevToolsHttpHandlerDelegate() {
}

// TODO(iotto): Implement
//std::string DevToolsManagerDelegateAndroid::GetTargetType(
//    content::WebContents* web_contents) {
//  TabAndroid* tab = web_contents ? TabAndroid::FromWebContents(web_contents)
//      : nullptr;
//  return tab ? DevToolsAgentHost::kTypePage :
//      DevToolsAgentHost::kTypeOther;
//}

std::string XWalkDevToolsHttpHandlerDelegate::GetTargetDescription(content::WebContents* web_contents) {
  LOG(WARNING) << "iotto " << __func__ << " IMPLEMENT";
  return std::string();
}

content::DevToolsAgentHost::List XWalkDevToolsHttpHandlerDelegate::RemoteDebuggingTargets() {
  LOG(ERROR) << "iotto " << __func__ << " IMPLEMENT";
  // see: chrome/browser/android/devtools_manager_delegate_android.cc

  return content::DevToolsAgentHost::List();
//  DevToolsAgentHost::List result;
//  std::set<WebContents*> tab_web_contents;
//  for (TabModelList::const_iterator iter = TabModelList::begin();
//      iter != TabModelList::end(); ++iter) {
//    TabModel* model = *iter;
//    for (int i = 0; i < model->GetTabCount(); ++i) {
//      TabAndroid* tab = model->GetTabAt(i);
//      if (!tab)
//        continue;
//
//      if (tab->web_contents())
//        tab_web_contents.insert(tab->web_contents());
//      result.push_back(DevToolsAgentHostForTab(tab));
//    }
//  }
//
//  // Add descriptors for targets not associated with any tabs.
//  DevToolsAgentHost::List agents = DevToolsAgentHost::GetOrCreateAll();
//  for (DevToolsAgentHost::List::iterator it = agents.begin();
//       it != agents.end(); ++it) {
//    if (WebContents* web_contents = (*it)->GetWebContents()) {
//      if (tab_web_contents.find(web_contents) != tab_web_contents.end())
//        continue;
//    }
//    result.push_back(*it);
//  }
//
//  return result;
}

std::string XWalkDevToolsHttpHandlerDelegate::GetDiscoveryPageHTML() {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_DEVTOOLS_FRONTEND_PAGE_HTML).as_string();
}

bool XWalkDevToolsHttpHandlerDelegate::IsBrowserTargetDiscoverable() {
  return true;
}

RemoteDebuggingServer::RemoteDebuggingServer(
    XWalkBrowserContext* browser_context,
    const std::string& ip,
    int port,
    const std::string& frontend_url) {

  LOG(WARNING) << "iotto " << __func__ << " REFACTOR!";
  base::FilePath output_dir;
  std::unique_ptr<content::DevToolsSocketFactory> factory(new TCPServerSocketFactory(ip, port, 1));
  DevToolsAgentHost::StartRemoteDebuggingServer(
      std::move(factory),
      base::FilePath(), base::FilePath());

  port_ = port;
}

RemoteDebuggingServer::~RemoteDebuggingServer() {
}

}  // namespace xwalk

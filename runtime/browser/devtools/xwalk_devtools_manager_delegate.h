// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_DEVTOOLS_XWALK_DEVTOOLS_MANAGER_DELEGATE_H_
#define XWALK_RUNTIME_BROWSER_DEVTOOLS_XWALK_DEVTOOLS_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "content/browser/devtools/devtools_http_handler.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace content {
class DevToolsHttpHandler;
}

namespace xwalk {

class XWalkBrowserContext;

class XWalkDevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  explicit XWalkDevToolsManagerDelegate(XWalkBrowserContext* browserContext);

  static void StartHttpHandler(XWalkBrowserContext* browserContext);
  static void StopHttpHandler();
  static int GetHttpHandlerPort();
  
  scoped_refptr<content::DevToolsAgentHost> CreateNewTarget(const GURL& url) override;
  content::DevToolsAgentHost::List RemoteDebuggingTargets() override;
  std::string GetTargetType(content::WebContents* web_contents) override;
  std::string GetTargetDescription(content::WebContents* web_contents) override;
  std::string GetDiscoveryPageHTML() override;
  bool IsBrowserTargetDiscoverable() override;

  ~XWalkDevToolsManagerDelegate() override;

 private:
  XWalkBrowserContext* _browser_context;
  DISALLOW_COPY_AND_ASSIGN(XWalkDevToolsManagerDelegate);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_DEVTOOLS_XWALK_DEVTOOLS_MANAGER_DELEGATE_H_

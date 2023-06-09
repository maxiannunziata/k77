// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_APPLICATION_BROWSER_APPLICATION_PROTOCOLS_H_
#define XWALK_APPLICATION_BROWSER_APPLICATION_PROTOCOLS_H_

#include <memory>
#include "net/url_request/url_request_job_factory.h"
#include "xwalk/application/browser/application_system.h"

namespace xwalk {
namespace application {

class ApplicationService;

// Creates the handlers for the app:// scheme.
std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
CreateApplicationProtocolHandler(ApplicationService* service);

}  // namespace application
}  // namespace xwalk

#endif  // XWALK_APPLICATION_BROWSER_APPLICATION_PROTOCOLS_H_

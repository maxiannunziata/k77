// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_RUNTIME_URL_REQUEST_CONTEXT_GETTER_H_
#define XWALK_RUNTIME_BROWSER_RUNTIME_URL_REQUEST_CONTEXT_GETTER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/browser_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory.h"
#include "services/network/public/cpp/network_quality_tracker.h"
#include "services/network/public/mojom/network_service.mojom-forward.h"

namespace base {
class MessageLoop;
}

namespace net {
class HostResolver;
class MappedHostResolver;
class NetworkDelegate;
class NetworkQualityEstimator;
class ProxyConfigService;
class RTTAndThroughputEstimatesObserver;
class URLRequestContextStorage;
class URLRequestJobFactory;
}

namespace xwalk {

class RuntimeURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  RuntimeURLRequestContextGetter(
      bool ignore_certificate_errors,
      const base::FilePath& base_path,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors);

  // net::URLRequestContextGetter implementation.
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner>
      GetNetworkTaskRunner() const override;

  net::HostResolver* host_resolver();
  void UpdateAcceptLanguages(const std::string& accept_languages);

 private:
  ~RuntimeURLRequestContextGetter() override;
  void InitializeURLRequestContext();

  bool ignore_certificate_errors_;
  base::FilePath base_path_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;

  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  std::unique_ptr<net::NetworkDelegate> network_delegate_;
  std::unique_ptr<net::URLRequestContextStorage> storage_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;

  std::unique_ptr<network::NetworkQualityTracker> network_quality_tracker_;

  // Listens to NetworkQualityTracker and sends network quality updates to the
  // renderer.
  std::unique_ptr<network::NetworkQualityTracker::RTTAndThroughputEstimatesObserver> network_quality_observer_;

  DISALLOW_COPY_AND_ASSIGN(RuntimeURLRequestContextGetter);
};

}  // namespace xwalk

#endif  // XWALK_RUNTIME_BROWSER_RUNTIME_URL_REQUEST_CONTEXT_GETTER_H_

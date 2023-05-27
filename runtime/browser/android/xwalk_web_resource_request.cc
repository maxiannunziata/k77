/*
 * xwalk_web_resource_request.cc
 *
 *  Created on: Feb 22, 2020
 *      Author: iotto
 */

#include "xwalk/runtime/browser/android/xwalk_web_resource_request.h"

#include "content/public/browser/resource_request_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "ui/base/page_transition_types.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::ToJavaArrayOfStrings;

namespace xwalk {
namespace {

void ConvertRequestHeadersToVectors(const net::HttpRequestHeaders& headers, std::vector<std::string>* header_names,
                                    std::vector<std::string>* header_values) {
  DCHECK(header_names->empty());
  DCHECK(header_values->empty());
  net::HttpRequestHeaders::Iterator headers_iterator(headers);
  while (headers_iterator.GetNext()) {
    header_names->push_back(headers_iterator.name());
    header_values->push_back(headers_iterator.value());
  }
}

}  // namespace

XWalkWebResourceRequest::XWalkWebResourceRequest(const network::ResourceRequest& request)
    : url(request.url.spec()),
      method(request.method),
      is_main_frame(request.resource_type == static_cast<int>(content::ResourceType::kMainFrame)),
      has_user_gesture(request.has_user_gesture),
      is_renderer_initiated(
          ui::PageTransitionIsWebTriggerable(static_cast<ui::PageTransition>(request.transition_type))) {
  ConvertRequestHeadersToVectors(request.headers, &header_names, &header_values);
}

XWalkWebResourceRequest::XWalkWebResourceRequest() {
}

XWalkWebResourceRequest::XWalkWebResourceRequest(XWalkWebResourceRequest&& other) = default;
XWalkWebResourceRequest& XWalkWebResourceRequest::operator=(XWalkWebResourceRequest&& other) = default;
XWalkWebResourceRequest::~XWalkWebResourceRequest() = default;

XWalkWebResourceRequest::JavaWebResourceRequest::JavaWebResourceRequest() = default;
XWalkWebResourceRequest::JavaWebResourceRequest::~JavaWebResourceRequest() = default;

// static
void XWalkWebResourceRequest::ConvertToJava(JNIEnv* env, const XWalkWebResourceRequest& request,
                                            JavaWebResourceRequest* jRequest) {
  jRequest->jurl = ConvertUTF8ToJavaString(env, request.url);
  jRequest->jmethod = ConvertUTF8ToJavaString(env, request.method);
  jRequest->jheader_names = ToJavaArrayOfStrings(env, request.header_names);
  jRequest->jheader_values = ToJavaArrayOfStrings(env, request.header_values);
}

} /* namespace xwalk */

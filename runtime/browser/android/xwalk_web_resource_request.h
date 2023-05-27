/*
 * xwalk_web_resource_request.h
 *
 *  Created on: Feb 22, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_ANDROID_XWALK_WEB_RESOURCE_REQUEST_H_
#define XWALK_RUNTIME_BROWSER_ANDROID_XWALK_WEB_RESOURCE_REQUEST_H_

#include <string>
#include <vector>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/optional.h"

namespace network {
struct ResourceRequest;
}

namespace xwalk {

struct XWalkWebResourceRequest {
 public:
  explicit XWalkWebResourceRequest(const network::ResourceRequest& request);

  // Add default copy/move/assign operators. Adding explicit destructor
  // prevents generating move operator.
  XWalkWebResourceRequest(XWalkWebResourceRequest&& other);
  XWalkWebResourceRequest& operator=(XWalkWebResourceRequest&& other);

  XWalkWebResourceRequest();
  virtual ~XWalkWebResourceRequest();

  // The java equivalent
  struct JavaWebResourceRequest {
    JavaWebResourceRequest();
    ~JavaWebResourceRequest();

    base::android::ScopedJavaLocalRef<jstring> jurl;
    base::android::ScopedJavaLocalRef<jstring> jmethod;
    base::android::ScopedJavaLocalRef<jobjectArray> jheader_names;
    base::android::ScopedJavaLocalRef<jobjectArray> jheader_values;
  };

  // Convenience method to convert AwWebResourceRequest to Java equivalent.
  static void ConvertToJava(JNIEnv* env,
                            const XWalkWebResourceRequest& request,
                            JavaWebResourceRequest* jRequest);

  std::string url;
  std::string method;
  bool is_main_frame;
  bool has_user_gesture;
  std::vector<std::string> header_names;
  std::vector<std::string> header_values;
  base::Optional<bool> is_renderer_initiated;
};

} /* namespace xwalk */

#endif /* XWALK_RUNTIME_BROWSER_ANDROID_XWALK_WEB_RESOURCE_REQUEST_H_ */

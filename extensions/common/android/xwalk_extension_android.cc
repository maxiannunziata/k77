// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/extensions/common/android/xwalk_extension_android.h"

#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/bind.h"
#include "base/logging.h"
#include "jni/XWalkExtensionAndroid_jni.h"
#include "xwalk/extensions/common/xwalk_extension.h"
#include "xwalk/runtime/browser/xwalk_browser_main_parts_android.h"
#include "xwalk/runtime/browser/xwalk_content_browser_client.h"

using base::android::ScopedJavaLocalRef;
using base::android::JavaParamRef;

namespace xwalk {
namespace extensions {

XWalkExtensionAndroid::XWalkExtensionAndroid(JNIEnv* env, jobject obj,
                                             jstring name, jstring js_api,
                                             jobjectArray js_entry_points)
    : XWalkExtension(),
      java_ref_(env, obj),
      next_instance_id_(1) {
  const char *str = env->GetStringUTFChars(name, 0);
  set_name(str);
  env->ReleaseStringUTFChars(name, str);

  str = env->GetStringUTFChars(js_api, 0);
  set_javascript_api(str);
  env->ReleaseStringUTFChars(js_api, str);

  std::vector<std::string> entry_points;
  base::android::AppendJavaStringArrayToStringVector(
      env, js_entry_points, &entry_points);

  if (!entry_points.size())
    return;

  set_entry_points(entry_points);
}

XWalkExtensionAndroid::~XWalkExtensionAndroid() {
  // The |instances_| holds references to the instances created from this
  // extension, but it doesn't own the object. The instance deletion is
  // transferred to XWalkExtensionServer.
  instances_.clear();
}

bool XWalkExtensionAndroid::is_valid() {
  if (instances_.empty() || javascript_api().empty()) {
    return false;
  }

  return true;
}

void XWalkExtensionAndroid::PostMessage(JNIEnv* env, jobject obj,
                                       jint instance, jstring msg) {
  if (!is_valid()) return;

  InstanceMap::iterator it = instances_.find(instance);
  if (it == instances_.end()) {
    LOG(WARNING) << "Instance(" << instance << ") not found ";
    return;
  }

  const char* str = env->GetStringUTFChars(msg, 0);
  it->second->PostMessageWrapper(str);
  env->ReleaseStringUTFChars(msg, str);
}

void XWalkExtensionAndroid::PostBinaryMessage(JNIEnv* env, jobject obj,
                                              jint instance, jbyteArray msg) {
  if (!is_valid()) return;

  InstanceMap::iterator it = instances_.find(instance);
  if (it == instances_.end()) {
    LOG(WARNING) << "Instance(" << instance << ") not found ";
    return;
  }

  jbyte* msg_ptr = env->GetByteArrayElements(msg, NULL);
  jsize msg_size = env->GetArrayLength(msg);
  it->second->PostBinaryMessageWrapper((const char*)msg_ptr, msg_size);
  env->ReleaseByteArrayElements(msg, msg_ptr, JNI_ABORT);
}

void XWalkExtensionAndroid::BroadcastMessage(JNIEnv* env, jobject obj,
                                             jstring msg) {
  if (!is_valid()) return;

  const char* str = env->GetStringUTFChars(msg, 0);
  for (InstanceMap::iterator it = instances_.begin();
       it != instances_.end(); ++it) {
    it->second->PostMessageWrapper(str);
  }
  env->ReleaseStringUTFChars(msg, str);
}

void XWalkExtensionAndroid::DestroyExtension(JNIEnv* env, jobject obj) {
  // Since XWalkExtensionServer owns this native object, and it won't be deleted
  // at this point even if the corresponding Java-side object is destroyed.
  // Instead, we only reset the java reference to the Java-side object and id
  // counter. See comments in xwalk_extension_android.h.
  java_ref_.reset();
  next_instance_id_ = 1;
}

XWalkExtensionInstance* XWalkExtensionAndroid::CreateInstance() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null()) {
    return NULL;
  }

  XWalkExtensionAndroidInstance* instance =
      new XWalkExtensionAndroidInstance(this, java_ref_, next_instance_id_);
  instances_[next_instance_id_] = instance;

  Java_XWalkExtensionAndroid_onInstanceCreated(
      env, obj, next_instance_id_);
  next_instance_id_++;

  // Here we return the raw pointer to its caller XWalkExtensionServer. Since
  // XWalkExtensionServer has a map to maintain all instances created, the
  // ownership is also transferred to XWalkExtensionServer so that the instance
  // should be deleted by XWalkExtensionServer.
  return instance;
}

void XWalkExtensionAndroid::RemoveInstance(int instance) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null()) {
    return;
  }

  InstanceMap::iterator it = instances_.find(instance);
  if (it == instances_.end()) {
    LOG(WARNING) << "Instance(" << instance << ") not found ";
    return;
  }

  instances_.erase(instance);
}

void XWalkExtensionAndroid::BindToJavaObject(JNIEnv* env, jobject obj) {
  JavaObjectWeakGlobalRef ref(env, obj);
  java_ref_ = ref;
}

XWalkExtensionAndroidInstance::XWalkExtensionAndroidInstance(
    XWalkExtensionAndroid* extension,
    const JavaObjectWeakGlobalRef& java_ref,
    int id)
    : extension_(extension),
      java_ref_(java_ref),
      id_(id) {
}

XWalkExtensionAndroidInstance::~XWalkExtensionAndroidInstance() {
  extension_->RemoveInstance(id_);

  // Try to notice Java side on instance removed.
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null()) {
    LOG(ERROR) << "No valid Java object to notice instance destroyed.";
    return;
  }
  Java_XWalkExtensionAndroid_onInstanceDestroyed(
      env, obj, id_);
}

void XWalkExtensionAndroidInstance::HandleMessage(
    std::unique_ptr<base::Value> msg) {
  std::string value;

  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null()) {
    LOG(ERROR) << "No valid Java object is referenced for message routing";
    return;
  }

  if (msg->GetAsString(&value)) {
    ScopedJavaLocalRef<jstring> buffer(env, env->NewStringUTF(value.c_str()));
    Java_XWalkExtensionAndroid_onMessage(env, obj, getID(), buffer);
  } else if (msg->is_blob()) {
    const base::Value::BlobStorage& blob_value = msg->GetBlob();
    ScopedJavaLocalRef<jbyteArray> buffer(
        env, env->NewByteArray(blob_value.size()));
    env->SetByteArrayRegion(
        buffer.obj(), 0, blob_value.size(),
        reinterpret_cast<jbyte*>(const_cast<char*>(blob_value.data())));
    Java_XWalkExtensionAndroid_onBinaryMessage(
        env, obj, getID(), buffer);
  } else {
    NOTREACHED() << "Failed to decode message as either string or binary blob";
  }
}

void XWalkExtensionAndroidInstance::HandleSyncMessage(
    std::unique_ptr<base::Value> msg) {
  std::unique_ptr<base::Value> ret_val(new base::Value(""));

  std::string value;
  if (!msg->GetAsString(&value)) {
    SendSyncReplyToJS(std::move(ret_val));
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null()) {
    LOG(ERROR) << "No valid Java object is referenced for sync message routing";
    SendSyncReplyToJS(std::move(ret_val));
    return;
  }

  ScopedJavaLocalRef<jstring> buffer(env, env->NewStringUTF(value.c_str()));
  ScopedJavaLocalRef<jstring> ret =
      Java_XWalkExtensionAndroid_onSyncMessage(
              env, obj, getID(), buffer);

  const char *str = env->GetStringUTFChars(ret.obj(), 0);
  ret_val.reset(new base::Value(str));
  env->ReleaseStringUTFChars(ret.obj(), str);

  SendSyncReplyToJS(std::move(ret_val));
}

static jlong JNI_XWalkExtensionAndroid_GetOrCreateExtension(JNIEnv* env, const JavaParamRef<jobject>& obj,
                                                            const JavaParamRef<jstring>& name,
                                                            const JavaParamRef<jstring>& js_api,
                                                            const JavaParamRef<jobjectArray>& js_entry_points) {
  xwalk::XWalkBrowserMainPartsAndroid* main_parts = ToAndroidMainParts(XWalkContentBrowserClient::Get()->main_parts());

  const char *str = env->GetStringUTFChars(name, 0);
  XWalkExtension* extension = main_parts->LookupExtension(str);
  env->ReleaseStringUTFChars(name, str);

  // Create a new extension object if no existing one is found.
  if (!extension) {
    extension = new XWalkExtensionAndroid(env, obj, name, js_api, js_entry_points);
    main_parts->RegisterExtension(std::unique_ptr < XWalkExtension > (extension));
  } else {
    static_cast<XWalkExtensionAndroid*>(extension)->BindToJavaObject(env, obj);
  }

  return reinterpret_cast<intptr_t>(extension);
}

bool RegisterXWalkExtensionAndroid(JNIEnv* env) {
//  return RegisterNativesImpl(env);
  return true;
}

}  // namespace extensions
}  // namespace xwalk

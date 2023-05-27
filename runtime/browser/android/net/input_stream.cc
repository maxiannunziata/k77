// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xwalk/runtime/browser/android/net/input_stream.h"

#include <algorithm>

#include "base/android/jni_android.h"
// Disable "Warnings treated as errors" for input_stream_jni as it's a Java
// system class and we have to generate C++ hooks for all methods in the class
// even if they're unused.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "xwalk/runtime/android/core_refactor/xwalk_core_jar_jni/InputStream_jni.h"
#pragma GCC diagnostic pop
#include "net/base/io_buffer.h"

using base::android::AttachCurrentThread;
using base::android::ClearException;
using base::android::JavaRef;
using JNI_InputStream::Java_InputStream_available;
using JNI_InputStream::Java_InputStream_close;
using JNI_InputStream::Java_InputStream_skip;
using JNI_InputStream::Java_InputStream_readI_AB_I_I;

namespace xwalk {

bool RegisterInputStream(JNIEnv* env) {
  // TODO remove registration too
//  return JNI_InputStream::RegisterNativesImpl(env);
  return true;
}

// Maximum number of bytes to be read in a single read.
const int InputStream::kBufferSize = 4096;

// static
const InputStream* InputStream::FromInputStream(
        const InputStream* input_stream) {
    return static_cast<const InputStream*>(input_stream);
}

// TODO(shouqun): Use unsafe version for all Java_InputStream methods in this
// file once BUG 157880 is fixed and implement graceful exception handling.

InputStream::InputStream() {
}

InputStream::InputStream(const JavaRef<jobject>& stream)
    : jobject_(stream) {
  DCHECK(!stream.is_null());
}

InputStream::~InputStream() {
  JNIEnv* env = AttachCurrentThread();
  Java_InputStream_close(env, jobject_);
}

bool InputStream::BytesAvailable(int* bytes_available) const {
  JNIEnv* env = AttachCurrentThread();
  int bytes = Java_InputStream_available(env, jobject_);
  if (ClearException(env))
    return false;
  *bytes_available = bytes;
  return true;
}

bool InputStream::Skip(int64_t n, int64_t* bytes_skipped) {
  JNIEnv* env = AttachCurrentThread();
  int bytes = Java_InputStream_skip(env, jobject_, n);
  if (ClearException(env))
    return false;
  if (bytes > n)
    return false;
  *bytes_skipped = bytes;
  return true;
}

bool InputStream::Read(net::IOBuffer* dest, int length, int* bytes_read) {
  JNIEnv* env = AttachCurrentThread();
  if (!buffer_.obj()) {
    // Allocate transfer buffer.
    buffer_.Reset(env, env->NewByteArray(kBufferSize));
    if (ClearException(env))
      return false;
  }

  *bytes_read = 0;

  const int read_size = std::min(length, kBufferSize);
  int32_t byte_count;
  do {
    // Unfortunately it is valid for the Java InputStream to read 0 bytes some
    // number of times before returning any more data. Because this method
    // signals EOF by setting |bytes_read| to 0 and returning true necessary to
    // call the Java-side read method until it returns something other than 0.
    byte_count = Java_InputStream_readI_AB_I_I(
        env, jobject_, buffer_, 0, read_size);
    if (ClearException(env))
      return false;
  } while (byte_count == 0);

  // We've reached the end of the stream.
  if (byte_count < 0)
    return true;

#ifndef NDEBUG
  int32_t buffer_length = env->GetArrayLength(buffer_.obj());
  DCHECK_GE(read_size, byte_count);
  DCHECK_GE(buffer_length, byte_count);
#endif  // NDEBUG

  // The DCHECKs are in place to help Chromium developers in case of bugs,
  // this check is to prevent a malicious InputStream implementation from
  // overrunning the |dest| buffer.
  if (byte_count > read_size)
    return false;

  // Copy the data over to the provided C++ side buffer.
  DCHECK_GE(length, byte_count);
  env->GetByteArrayRegion(buffer_.obj(), 0, byte_count,
      reinterpret_cast<jbyte*>(dest->data() + *bytes_read));
  if (ClearException(env))
    return false;

  *bytes_read = byte_count;
  return true;
}

}  // namespace xwalk

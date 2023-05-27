/*
 * xwalk_descriptors.h
 *
 *  Created on: Jan 17, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_COMMON_ANDROID_XWALK_DESCRIPTORS_H_
#define XWALK_RUNTIME_COMMON_ANDROID_XWALK_DESCRIPTORS_H_

#include "content/public/common/content_descriptors.h"

enum {
  kXWalkLocalePakDescriptor = kContentIPCDescriptorMax + 1,
  kXWalkMainPakDescriptor,
  kXWalk100PakDescriptor
};


#endif /* XWALK_RUNTIME_COMMON_ANDROID_XWALK_DESCRIPTORS_H_ */

/*
 * xwalk_content_gpu_client.h
 *
 *  Created on: Jan 16, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_GPU_XWALK_CONTENT_GPU_CLIENT_H_
#define XWALK_RUNTIME_GPU_XWALK_CONTENT_GPU_CLIENT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "content/public/gpu/content_gpu_client.h"

namespace xwalk {

class XWalkContentGpuClient : public content::ContentGpuClient {
 public:
  using GetSyncPointManagerCallback = base::RepeatingCallback<gpu::SyncPointManager*()>;
  using GetSharedImageManagerCallback = base::RepeatingCallback<gpu::SharedImageManager*()>;

  XWalkContentGpuClient(const GetSyncPointManagerCallback& sync_point_manager_callback,
                        const GetSharedImageManagerCallback& shared_image_manager_callback);
  XWalkContentGpuClient();
  ~XWalkContentGpuClient() override;

  // content::ContentGpuClient implementation.
  gpu::SyncPointManager* GetSyncPointManager() override;
  gpu::SharedImageManager* GetSharedImageManager() override;

 private:
  GetSyncPointManagerCallback sync_point_manager_callback_;
  GetSharedImageManagerCallback shared_image_manager_callback_;
};

} /* namespace xwalk */

#endif /* XWALK_RUNTIME_GPU_XWALK_CONTENT_GPU_CLIENT_H_ */

/*
 * xwalk_stream_reader_url_loader.h
 *
 *  Created on: Feb 24, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_STREAM_READER_URL_LOADER_H_
#define XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_STREAM_READER_URL_LOADER_H_

#include <jni.h>

#include "services/network/public/cpp/net_adapters.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "net/http/http_byte_range.h"

namespace xwalk {
class InputStream;
class InputStreamReaderWrapper;

class XWalkStreamReaderUrlLoader : public network::mojom::URLLoader {
 public:
  // Delegate abstraction for obtaining input streams.
  class ResponseDelegate {
   public:
    virtual ~ResponseDelegate() {
    }

    // This method is called from a worker thread, not from the IO thread.
    virtual std::unique_ptr<InputStream> OpenInputStream(JNIEnv* env) = 0;

    // This method is called on the URLLoader thread (IO thread) if the
    // result of calling OpenInputStream was null.
    // Returns true if the request was restarted with a new loader or
    // was completed, false otherwise.
    virtual bool OnInputStreamOpenFailed() = 0;

    virtual bool GetMimeType(JNIEnv* env, const GURL& url, InputStream* stream, std::string* mime_type) = 0;

    virtual bool GetCharset(JNIEnv* env, const GURL& url, InputStream* stream, std::string* charset) = 0;

    virtual void AppendResponseHeaders(JNIEnv* env, net::HttpResponseHeaders* headers) = 0;
  };

  XWalkStreamReaderUrlLoader(const network::ResourceRequest& resource_request,
                             network::mojom::URLLoaderClientPtr client,
                             const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
                             std::unique_ptr<ResponseDelegate> response_delegate);
  ~XWalkStreamReaderUrlLoader() override;

  void Start();

  // network::mojom::URLLoader overrides:
  void FollowRedirect(const std::vector<std::string>& removed_headers, const net::HttpRequestHeaders& modified_headers,
                      const base::Optional<GURL>& new_url) override;
  void ProceedWithResponse() override;
  void SetPriority(net::RequestPriority priority, int intra_priority_value) override;
  void PauseReadingBodyFromNet() override;
  void ResumeReadingBodyFromNet() override;

 private:
  bool ParseRange(const net::HttpRequestHeaders& headers);
  void OnInputStreamOpened(std::unique_ptr<XWalkStreamReaderUrlLoader::ResponseDelegate> returned_delegate,
                           std::unique_ptr<InputStream> input_stream);
  void OnReaderSeekCompleted(int result);
  void HeadersComplete(int status_code, const std::string& status_text);
  void RequestComplete(int status_code);
  void SendBody();

  void OnDataPipeWritable(MojoResult result);
  void CleanUp();
  void DidRead(int result);
  void ReadMore();

  // Expected content size
  int64_t expected_content_size_ = -1;

  net::HttpByteRange byte_range_;
  network::ResourceRequest resource_request_;
  network::mojom::URLLoaderClientPtr client_;
  const net::MutableNetworkTrafficAnnotationTag traffic_annotation_;
  std::unique_ptr<ResponseDelegate> response_delegate_;
  scoped_refptr<InputStreamReaderWrapper> input_stream_reader_wrapper_;

  mojo::ScopedDataPipeProducerHandle producer_handle_;
  scoped_refptr<network::NetToMojoPendingBuffer> pending_buffer_;
  mojo::SimpleWatcher writable_handle_watcher_;
  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<XWalkStreamReaderUrlLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(XWalkStreamReaderUrlLoader)
  ;
};

} /* namespace xwalk */

#endif /* XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_STREAM_READER_URL_LOADER_H_ */

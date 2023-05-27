// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XWALK_RUNTIME_BROWSER_GEOLOCATION_XWALK_ACCESS_TOKEN_STORE_H_
#define XWALK_RUNTIME_BROWSER_GEOLOCATION_XWALK_ACCESS_TOKEN_STORE_H_

//TODO(iotto) : replaced by
/*
  // Allows the embedder to provide a URLRequestContextGetter to use for network
  // geolocation queries.
  // * May be called from any thread. A URLRequestContextGetter is then provided
  //   by invoking |callback| on the calling thread.
  // * Default implementation provides nullptr URLRequestContextGetter.
  virtual void GetGeolocationRequestContext(
      base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
          callback);
 */

class XWalkAccessTokenStore : public device::AccessTokenStore {
 public:
  explicit XWalkAccessTokenStore(net::URLRequestContextGetter* request_context);

 private:
  ~XWalkAccessTokenStore() override;

  // AccessTokenStore
  void LoadAccessTokens(
      const LoadAccessTokensCallback& callback) override;

  void SaveAccessToken(
      const GURL& server_url, const base::string16& access_token) override;

  static void DidLoadAccessTokens(
      net::URLRequestContextGetter* request_context,
      const LoadAccessTokensCallback& callback);

  net::URLRequestContextGetter* request_context_;

  DISALLOW_COPY_AND_ASSIGN(XWalkAccessTokenStore);
};

#endif  // XWALK_RUNTIME_BROWSER_GEOLOCATION_XWALK_ACCESS_TOKEN_STORE_H_

/*
 * xwalk_net_helpers.h
 *
 *  Created on: Feb 16, 2020
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_NET_HELPERS_H_
#define XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_NET_HELPERS_H_

class GURL;

namespace xwalk {

class XWalkContentsIoThreadClient;

// Returns the updated request's |load_flags| based on the settings.
int UpdateLoadFlags(int load_flags, XWalkContentsIoThreadClient* client);

// Returns true if the given URL should be aborted with
// net::ERR_ACCESS_DENIED.
bool ShouldBlockURL(const GURL& url, XWalkContentsIoThreadClient* client);


} // xwalk

#endif /* XWALK_RUNTIME_BROWSER_NETWORK_SERVICES_XWALK_NET_HELPERS_H_ */

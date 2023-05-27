/*
 * network_change_notifier_factory_tenta.h
 *
 *  Created on: Dec 5, 2016
 *      Author: iotto
 */

#ifndef XWALK_RUNTIME_NET_TENTA_NETWORK_CHANGE_NOTIFIER_FACTORY_H_
#define XWALK_RUNTIME_NET_TENTA_NETWORK_CHANGE_NOTIFIER_FACTORY_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/android/network_change_notifier_delegate_android.h"
#include "net/android/network_change_notifier_factory_android.h"

namespace net {

class NetworkChangeNotifier;
class NetworkChangeNotifierDelegateAndroid;
}

namespace xwalk {
namespace tenta {
// NetworkChangeNotifierFactory creates Android-specific specialization of
// NetworkChangeNotifier. See network_change_notifier_android.h for more
// details.
class NetworkChangeNotifierFactoryTenta: public net::NetworkChangeNotifierFactoryAndroid {
public:
	// Must be called on the JNI thread.
	NetworkChangeNotifierFactoryTenta();

	// Must be called on the JNI thread.
	~NetworkChangeNotifierFactoryTenta() override;

//	// NetworkChangeNotifierFactory:
//	net::NetworkChangeNotifier* CreateInstance() override;
//
//private:
//	// Delegate passed to the instances created by this class.
//	net::NetworkChangeNotifierDelegateAndroid delegate_;
//
//	DISALLOW_COPY_AND_ASSIGN(NetworkChangeNotifierFactoryTenta);
};

}	// namespace tenta
}  // namespace xwalk

#endif /* XWALK_RUNTIME_NET_TENTA_NETWORK_CHANGE_NOTIFIER_FACTORY_H_ */

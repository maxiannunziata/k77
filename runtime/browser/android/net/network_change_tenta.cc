/*
 * network_change_tenta.cc
 *
 *  Created on: Feb 2, 2017
 *      Author: iotto
 */

#include <xwalk/runtime/browser/android/net/network_change_tenta.h>

namespace xwalk {
namespace tenta {

base::LazyInstance<NetworkChangeTenta>::Leaky g_lazy_instance;

//static
NetworkChangeTenta* NetworkChangeTenta::GetInstance() {
  return g_lazy_instance.Pointer();
}

NetworkChangeTenta::NetworkChangeTenta() {
  // TODO Auto-generated constructor stub
  net::NetworkChangeNotifier::AddIPAddressObserver(this);
  net::NetworkChangeNotifier::AddConnectionTypeObserver(this);
  net::NetworkChangeNotifier::AddDNSObserver(this);

}

NetworkChangeTenta::~NetworkChangeTenta() {
  // TODO Auto-generated destructor stub
  net::NetworkChangeNotifier::RemoveIPAddressObserver(this);
  net::NetworkChangeNotifier::RemoveConnectionTypeObserver(this);
  net::NetworkChangeNotifier::RemoveDNSObserver(this);
}

/**
 * Called by system, when network ip changed
 */
void NetworkChangeTenta::OnIPAddressChanged() {
  // TODO
}

void NetworkChangeTenta::OnConnectionTypeChanged(
    net::NetworkChangeNotifier::ConnectionType type) {

}

void NetworkChangeTenta::OnDNSChanged() {

}

void NetworkChangeTenta::OnInitialDNSConfigRead() {

}
} /* namespace tenta */
} /* namespace xwalk */

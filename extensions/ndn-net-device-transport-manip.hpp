#ifndef NDN_NET_DEVICE_TRANSPORT_M_HPP
#define NDN_NET_DEVICE_TRANSPORT_M_HPP

#include <vector>

#include "ndn-packet-filter.hpp"
#include "ndn-qdisc-item.hpp"
#include "ns3/callback.h"
#include "ns3/channel.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/log.h"
#include "ns3/ndnSIM/NFD/daemon/face/transport.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/watchdog.h"
#include "prio-queue-disc-manip.h"
//#include "storage-queue.hpp"

namespace ns3 {
namespace ndn {

class NetDeviceTransportManip;

void monitorFunc(NetDeviceTransportManip* c);

/**
 * \ingroup ndn-face
 * \brief ndnSIM-specific transport
 */
class NetDeviceTransportManip : public nfd::face::Transport {
  public:
    NetDeviceTransportManip(
        Ptr<Node> node, const Ptr<NetDevice>& netDevice,
        const std::string& localUri, const std::string& remoteUri,
        ::ndn::nfd::FaceScope scope = ::ndn::nfd::FACE_SCOPE_NON_LOCAL,
        ::ndn::nfd::FacePersistency persistency =
            ::ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
        ::ndn::nfd::LinkType linkType = ::ndn::nfd::LINK_TYPE_POINT_TO_POINT);

    ~NetDeviceTransportManip();

    Ptr<NetDevice> GetNetDevice() const;

    virtual ssize_t getSendQueueLength() final;

    void QdiscRun();

  private:
    virtual void doClose() override;

    virtual void doSend(const Block& packet,
                        const nfd::EndpointId& endpoint) override;

    void receiveFromNetDevice(Ptr<NetDevice> device, Ptr<const ns3::Packet> p,
                              uint16_t protocol, const Address& from,
                              const Address& to,
                              NetDevice::PacketType packetType);

    Ptr<NetDevice> m_netDevice;  ///< \brief Smart pointer to NetDevice
    Ptr<Node> m_node;
    Ptr<NDNPacketFilter> m_pf;
    Ptr<Queue<Packet>> m_txQueue;
    PrioQueueDiscManip m_prioq;
    std::function<void(Ptr<const Packet>)> cbfunc;
    std::function<void(Ptr<const QueueDiscItem> item, const char* reason)>
        dropcbfunc;
    DropTailQueue<QueueDiscItem> storageq;

    std::list<uint64_t> qsize4;
    Watchdog queueMonitor;
    friend void monitorFunc(NetDeviceTransportManip*);
    // ns3::Callback<void> monitorFunc;
};

}  // namespace ndn
}  // namespace ns3

#endif

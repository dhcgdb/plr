#include "ndn-net-device-transport-manip.hpp"

#include "boost/chrono/chrono.hpp"
#include "ns3/callback.h"
#include "ns3/core-module.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/model/ndn-block-header.hpp"
#include "ns3/ndnSIM/ndn-cxx/data.hpp"
#include "ns3/ndnSIM/ndn-cxx/encoding/block.hpp"
#include "ns3/ndnSIM/ndn-cxx/interest.hpp"
#include "ns3/ndnSIM/ndn-cxx/lp/packet.hpp"
#include "ns3/ndnSIM/utils/ndn-ns3-packet-tag.hpp"
#include "ns3/ptr.h"
#include "ns3/queue.h"
#include "predefine.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.NetDeviceTransportManip");

namespace ns3 {
namespace ndn {

NetDeviceTransportManip::NetDeviceTransportManip(
    Ptr<Node> node, const Ptr<NetDevice>& netDevice,
    const std::string& localUri, const std::string& remoteUri,
    ::ndn::nfd::FaceScope scope, ::ndn::nfd::FacePersistency persistency,
    ::ndn::nfd::LinkType linkType)
    : m_netDevice(netDevice), m_node(node) {
    this->setLocalUri(FaceUri(localUri));
    this->setRemoteUri(FaceUri(remoteUri));
    this->setScope(scope);
    this->setPersistency(persistency);
    this->setLinkType(linkType);
    this->setMtu(m_netDevice->GetMtu());  // Use the MTU of the netDevice

    /*
    // Get send queue capacity for congestion marking
    PointerValue txQueueAttribute;
    if (m_netDevice->GetAttributeFailSafe("TxQueue", txQueueAttribute)) {
        Ptr<ns3::QueueBase> txQueue = txQueueAttribute.Get<ns3::QueueBase>();
        m_txQueue = DynamicCast<Queue<Packet>>(txQueue);
        // must be put into bytes mode queue

        auto size = txQueue->GetMaxSize();
        NS_LOG_DEBUG("NetDevice QueueSize:" << size);
        if (size.GetUnit() == BYTES) {
            this->setSendQueueCapacity(size.GetValue());
        } else {
            // don't know the exact size in bytes, guessing based on "standard"
            // packet size
            this->setSendQueueCapacity(size.GetValue() * 1500);
        }
    }
    */
    PointerValue txQueueAttribute;
    if (m_netDevice->GetAttributeFailSafe("TxQueue", txQueueAttribute)) {
        Ptr<ns3::QueueBase> txQueue = txQueueAttribute.Get<ns3::QueueBase>();
        m_txQueue = DynamicCast<Queue<Packet>>(txQueue);
    }
    setSendQueueCapacity(QLENGTH * 1500);

    NS_LOG_FUNCTION(
        this << "Creating an ndnSIM transport instance for netDevice with URI"
             << this->getLocalUri());

    NS_ASSERT_MSG(m_netDevice != 0,
                  "NetDeviceFace needs to be assigned a valid NetDevice");

    m_node->RegisterProtocolHandler(
        MakeCallback(&NetDeviceTransportManip::receiveFromNetDevice, this),
        L3Protocol::ETHERNET_FRAME_TYPE, m_netDevice,
        true /*promiscuous mode*/);

    cbfunc = [this](Ptr<const Packet> pkt) { return QdiscRun(); };
    if (!m_netDevice->TraceConnectWithoutContext(
            "PhyTxBegin",
            MakeCallback(&std::function<void(Ptr<const Packet>)>::operator(),
                         &cbfunc)))
        NS_LOG_DEBUG("PhyTxEnd Set Error");

    m_prioq.SetMaxSize(QueueSize(std::to_string(QLENGTH) + "p"));
    m_prioq.Initialize();
    m_pf = Create<NDNPacketFilter>();
    m_prioq.AddPacketFilter(m_pf);
    for (int i = 0; i < m_prioq.GetNQueueDiscClasses(); i++) {
        NDN_LOG_DEBUG(i << ":"
                        << m_prioq.GetQueueDiscClass(i)
                               ->GetQueueDisc()
                               ->GetInternalQueue(0)
                               ->GetMaxSize());
    }
    dropcbfunc = [this](Ptr<const QueueDiscItem> item, const char* reason) {
        auto itemnew = ConstCast<QueueDiscItem>(item);
        auto itemndn = DynamicCast<NDNQueueDiscItem>(itemnew);
        Name name;
        BlockHeader header;
        itemndn->GetPacket()->Copy()->PeekHeader(header);
        Block blk = header.getBlock();
        lp::Packet p(blk);
        ::ndn::Buffer::const_iterator first, last;
        std::tie(first, last) = p.get<lp::FragmentField>(0);
        Block fragmentBlock(&*first, std::distance(first, last));
        if (fragmentBlock.type() == ::ndn::tlv::Data) {
            Data d(fragmentBlock);
            name = d.getName();
        } else
            return;
        if (storageq.Enqueue(itemndn)) {
            std::cout << "storageq Enqueue dropped item successs"
                      << storageq.GetTotalReceivedPackets() << std::endl;
            Interest tmpint(name);
            tmpint.setInterestLifetime(boost::chrono::milliseconds(2));
            tmpint.setCanBePrefix(false);
            lp::Nack nack(tmpint);
            nack.setReason(lp::NackReason::CONGESTION);
            lp::Packet lppkt(nack.getInterest().wireEncode());
            lppkt.add<lp::NackField>(nack.getHeader());
            auto nackblk = lppkt.wireEncode();
            BlockHeader nackblkheader(nackblk);
            Ptr<ns3::Packet> ns3nackPacket = Create<ns3::Packet>();
            ns3nackPacket->AddHeader(nackblkheader);
            Ptr<NDNQueueDiscItem> qditem = Create<NDNQueueDiscItem>(
                ns3nackPacket, m_netDevice->GetBroadcast(),
                L3Protocol::ETHERNET_FRAME_TYPE);
            if (m_prioq.Enqueue(qditem))
                std::cout << "m_prioq Enqueue nack successs" << std::endl;
            QdiscRun();
        }
    };
    if (!m_prioq.GetQueueDiscClass(1)
             ->GetQueueDisc()
             ->TraceConnectWithoutContext(
                 "DropBeforeEnqueue",
                 MakeCallback(
                     &std::function<void(Ptr<const QueueDiscItem> item,
                                         const char* reason)>::operator(),
                     &dropcbfunc)))
        NS_LOG_DEBUG("DropBeforeEnqueue Set Error");

    storageq.SetMaxSize(QueueSize(std::to_string(SQLENGTH) + "p"));
    storageq.Initialize();
    NS_LOG_DEBUG("storageq:" << storageq.GetMaxSize());
    queueMonitor.SetFunction(monitorFunc);
    queueMonitor.SetArguments(this);
    if (HAVERETX) {
        NS_LOG_DEBUG("have retx");
        queueMonitor.Ping(MilliSeconds(INTERVALMS));
    }
}

void monitorFunc(NetDeviceTransportManip* c) {
    auto curqlen =
        c->m_prioq.GetQueueDiscClass(1)->GetQueueDisc()->GetNPackets();
    std::cout << "call monitorFunc " << curqlen << std::endl;
    // std::cout << "call monitorFunc " << c->qsize4.back() << std::endl;
    // if (c->qsize4.size() < SQLENGTH)
    //     c->qsize4.push_back(
    //         c->m_prioq.GetQueueDiscClass(1)->GetQueueDisc()->GetNPackets());
    // else {
    //     c->qsize4.pop_front();
    //     c->qsize4.push_back(
    //         c->m_prioq.GetQueueDiscClass(1)->GetQueueDisc()->GetNPackets());
    // }
    if (curqlen < RECLENGTH) {
        auto item = c->storageq.Dequeue();
        if (item != nullptr) {
            std::cout << "dequeue success, current length:"
                      << c->storageq.GetNPackets() << std::endl;
            c->m_prioq.Enqueue(item);
            c->QdiscRun();
        }
    }
    c->queueMonitor.Ping(MilliSeconds(INTERVALMS));
}

NetDeviceTransportManip::~NetDeviceTransportManip() {
    NS_LOG_FUNCTION_NOARGS();
}

ssize_t NetDeviceTransportManip::getSendQueueLength() {
    // PointerValue txQueueAttribute;
    // if (m_netDevice->GetAttributeFailSafe("TxQueue", txQueueAttribute)) {
    //     Ptr<ns3::QueueBase> txQueue = txQueueAttribute.Get<ns3::QueueBase>();
    //     return txQueue->GetNBytes();
    // } else {
    //     return nfd::face::QUEUE_UNSUPPORTED;
    // }
    return m_prioq.GetQueueDiscClass(1)->GetQueueDisc()->GetNBytes();
}

void NetDeviceTransportManip::doClose() {
    NS_LOG_FUNCTION(this << "Closing transport for netDevice with URI"
                         << this->getLocalUri());

    // set the state of the transport to "CLOSED"
    this->setState(nfd::face::TransportState::CLOSED);
}

void NetDeviceTransportManip::doSend(const Block& packet,
                                     const nfd::EndpointId& endpoint) {
    NS_LOG_FUNCTION(this << "Sending packet from netDevice with URI"
                         << this->getLocalUri());

    // convert NFD packet to NS3 packet
    BlockHeader header(packet);

    Ptr<ns3::Packet> ns3Packet = Create<ns3::Packet>();
    ns3Packet->AddHeader(header);

    Ptr<NDNQueueDiscItem> qditem =
        Create<NDNQueueDiscItem>(ns3Packet, m_netDevice->GetBroadcast(),
                                 L3Protocol::ETHERNET_FRAME_TYPE);
    m_prioq.Enqueue(qditem);
    QdiscRun();
    // send the NS3 packet
    // m_netDevice->Send(ns3Packet, m_netDevice->GetBroadcast(),
    //                  L3Protocol::ETHERNET_FRAME_TYPE);
}

// callback
void NetDeviceTransportManip::receiveFromNetDevice(
    Ptr<NetDevice> device, Ptr<const ns3::Packet> p, uint16_t protocol,
    const Address& from, const Address& to, NetDevice::PacketType packetType) {
    NS_LOG_FUNCTION(device << p << protocol << from << to << packetType);

    // Convert NS3 packet to NFD packet
    Ptr<ns3::Packet> packet = p->Copy();

    BlockHeader header;
    packet->RemoveHeader(header);

    this->receive(std::move(header.getBlock()));
}

Ptr<NetDevice> NetDeviceTransportManip::GetNetDevice() const {
    return m_netDevice;
}

void NetDeviceTransportManip::QdiscRun() {
    // NS_LOG_DEBUG(
    //     "Entering QdiscRun: " << m_txQueue->GetCurrentSize().GetValue());
    if (m_txQueue->GetCurrentSize() < m_txQueue->GetMaxSize() &&
        m_prioq.GetCurrentSize().GetValue() > 0) {
        auto qdi = m_prioq.Dequeue();
        if (qdi) {
            NS_LOG_DEBUG("Send " << qdi << " to Dev");
            m_netDevice->Send(qdi->GetPacket(), qdi->GetAddress(),
                              qdi->GetProtocol());
        }
    }
    // NS_LOG_DEBUG(
    //     "Levaing QdiscRun: " << m_txQueue->GetCurrentSize().GetValue());
}

}  // namespace ndn
}  // namespace ns3

#include "ndn-packet-filter.hpp"

#include "ndn-qdisc-item.hpp"
#include "ns3/log.h"
#include "ns3/ndnSIM/model/ndn-block-header.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/ndn-cxx/encoding/tlv.hpp"
#include "ns3/ndnSIM/ndn-cxx/lp/packet.hpp"
#include "ns3/ptr.h"

namespace ns3 {
namespace ndn {

NS_LOG_COMPONENT_DEFINE("ndn.NDNPacketFilter");

NS_OBJECT_ENSURE_REGISTERED(NDNPacketFilter);

TypeId NDNPacketFilter::GetTypeId() {
    static TypeId tid = TypeId("ns3::ndn::NDNPacketFilter")
                            .SetGroupName("ndn")
                            .SetParent<PacketFilter>()
                            .AddConstructor<NDNPacketFilter>();
    return tid;
}

NDNPacketFilter::NDNPacketFilter() { NS_LOG_FUNCTION(this); };

NDNPacketFilter::~NDNPacketFilter() { NS_LOG_FUNCTION(this); }

bool NDNPacketFilter::CheckProtocol(Ptr<QueueDiscItem> item) const {
    Ptr<NDNQueueDiscItem> item_rtype = DynamicCast<NDNQueueDiscItem>(item);
    return true;
}

int32_t NDNPacketFilter::DoClassify(Ptr<QueueDiscItem> item) const {
    BlockHeader header;
    Ptr<NDNQueueDiscItem> item_rtype = DynamicCast<NDNQueueDiscItem>(item);
    item_rtype->GetPacket()->Copy()->PeekHeader(header);
    Block blk = header.getBlock();

    std::function<int32_t(const Block&)> decodeType =
        [&decodeType](const Block& block) {
            namespace tlv = ::ndn::tlv;
            namespace lp = ::ndn::lp;
            if (block.type() == lp::tlv::LpPacket) {
                lp::Packet p(block);
                if (p.has<lp::NackField>()) {
                    NS_LOG_DEBUG("decodeType is nack");
                    return 0;
                }
            }
            NS_LOG_DEBUG("decodeType is not nack");
            return 1;
            /*
            switch (block.type()) {
            case tlv::Interest:
            case tlv::Data:
                NS_LOG_DEBUG("decodeType is: " << block.type());
                return 1;
            case lp::tlv::LpPacket: {
                lp::Packet p(block);
                if (p.has<lp::FragCountField>() &&
                    p.get<lp::FragCountField>() != 1) {
                    NS_LOG_DEBUG("decodeType is fragment");
                    return 1;
                } else {
                    if (p.has<lp::NackField>()) {
                        NS_LOG_DEBUG("decodeType is nack");
                        return 0;
                    }
                    ::ndn::Buffer::const_iterator first, last;
                    std::tie(first, last) = p.get<lp::FragmentField>(0);
                    try {
                        Block fragmentBlock(&*first, std::distance(first,
            last)); NS_LOG_DEBUG("decodeType is to recurse"); return
            decodeType(fragmentBlock); } catch (const tlv::Error& error) {
                        NS_LOG_DEBUG("decodeType meets non-tlv");
                        return 2;
                    }
                }
            }
            default:
                NS_LOG_DEBUG("decodeType remains unrecognized");
                return 2;
            };*/
        };
    return decodeType(blk);
}
}  // namespace ndn
}  // namespace ns3
#include "ndn-qdisc-item.hpp"

#include "ns3/ndnSIM/model/ndn-block-header.hpp"
namespace ns3 {
namespace ndn {
NDNQueueDiscItem::NDNQueueDiscItem(Ptr<Packet> p, const Address &addr,
                                   uint16_t protocol)
    : QueueDiscItem(p, addr, protocol) {}

NDNQueueDiscItem::~NDNQueueDiscItem() {}

uint32_t NDNQueueDiscItem::GetSize() const {
    Ptr<Packet> ptrp = GetPacket();
    NS_ASSERT(ptrp != 0);
    uint32_t ret = ptrp->GetSize();
    return ret;
}

void NDNQueueDiscItem::AddHeader() { return; }

bool NDNQueueDiscItem::Mark(void) { return false; }
}  // namespace ndn
}  // namespace ns3
#ifndef NDN_PACKET_FILTER_H
#define NDN_PACKET_FILTER_H

#include "ns3/ndnSIM/ndn-cxx/encoding/block.hpp"
#include "ns3/object.h"
#include "ns3/packet-filter.h"

namespace ns3 {
namespace ndn {
class NDNPacketFilter : public PacketFilter {
  public:
    static TypeId GetTypeId(void);
    NDNPacketFilter();
    virtual ~NDNPacketFilter();

  private:
    virtual bool CheckProtocol(Ptr<QueueDiscItem> item) const;
    virtual int32_t DoClassify(Ptr<QueueDiscItem> item) const;
};
}  // namespace ndn
}  // namespace ns3

#endif
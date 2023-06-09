#ifndef NDN_QDISC_ITEM_H
#define NDN_QDISC_ITEM_H

#include "ns3/packet.h"
#include "ns3/queue-item.h"

namespace ns3 {
namespace ndn {
class NDNQueueDiscItem : public QueueDiscItem {
  public:
    NDNQueueDiscItem(Ptr<Packet> p, const Address &addr, uint16_t protocol);

    virtual ~NDNQueueDiscItem();

    virtual uint32_t GetSize(void) const;
    virtual void AddHeader(void);
    virtual bool Mark(void);

  private:
    NDNQueueDiscItem();

    NDNQueueDiscItem(const NDNQueueDiscItem &);

    NDNQueueDiscItem &operator=(const NDNQueueDiscItem &);
};
}  // namespace ndn
}  // namespace ns3

#endif
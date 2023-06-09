#ifndef TRANSPORT_PRIOQ_H
#define TRANSPORT_PRIOQ_H
#include "ns3/drop-tail-queue.h"

namespace ns3 {
namespace ndn {
class StorageQueue : DropTailQueue<Packet> {
  public:
    static TypeId GetTypeId(void);
    StorageQueue();
    ~StorageQueue();

  private:
};
}  // namespace ndn
}  // namespace ns3

#endif

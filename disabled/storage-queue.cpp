#include "storage-queue.hpp"
namespace ns3 {
namespace ndn {
    
TypeId StorageQueue::GetTypeId() {
    static TypeId tid = TypeId("ns3::ndn::StorageQueue")
                            .SetParent<DropTailQueue<Packet>>()
                            .SetGroupName("ndn")
                            .AddConstructor<StorageQueue>();
    return tid;
}

StorageQueue::StorageQueue() {}

StorageQueue::~StorageQueue() {}

}  // namespace ndn
}  // namespace ns3
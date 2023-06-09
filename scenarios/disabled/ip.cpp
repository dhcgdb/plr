#include "ns3/core-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/utils/tracers/l2-rate-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-cs-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.hpp"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/udp-echo-helper.h"
#include "openssl/evp.h"
#include "sodium.h"

#define MESSAGE ((const unsigned char*)"test")
#define MESSAGE_LEN 4

namespace ns3 {

int main(int argc, char* argv[]) {
    Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                       StringValue("100Mbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
    Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20p"));

    CommandLine cmd;
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    auto devices = p2p.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    TrafficControlHelper tc;
    tc.SetRootQueueDisc("ns3::PfifoFastQueueDisc");
    tc.Install(devices);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    UdpEchoServerHelper echoServer(9);
    echoServer.Install(nodes.Get(1));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    echoClient.Install(nodes.Get(0));

    Simulator::Stop(Seconds(3));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

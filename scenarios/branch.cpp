#include "ndn-stack-helper-manip.hpp"
#include "ns3/core-module.h"
#include "ns3/ndn-all.hpp"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/apps/ndn-consumer-pcon.hpp"
#include "ns3/ndnSIM/utils/topology/annotated-topology-reader.hpp"
#include "ns3/ndnSIM/utils/tracers/l2-rate-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-cs-tracer.hpp"
#include "ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.hpp"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "predefine.hpp"

#define MESSAGE ((const unsigned char*)"test")
#define MESSAGE_LEN 4

namespace ns3 {

int main(int argc, char* argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // NodeContainer nodes;
    // nodes.Create(3);
    // PointToPointHelper p2p;
    // p2p.SetChannelAttribute("Delay", StringValue("2.5ms"));
    // p2p.SetQueueAttribute("MaxSize", StringValue("1p"));
    // p2p.SetDeviceAttribute("DataRate", StringValue("20Mbps"));
    // auto ndevc1 = p2p.Install(nodes.Get(0), nodes.Get(1));
    // p2p.SetDeviceAttribute("DataRate", StringValue("25Mbps"));
    // auto ndevc2 = p2p.Install(nodes.Get(1), nodes.Get(2));

    AnnotatedTopologyReader toporeader;
    toporeader.SetFileName("/home/user/retx_env/retx/scenarios/topo_branch");
    toporeader.Read();
    auto nodes = toporeader.GetNodes();
    auto links = toporeader.GetLinks();
    auto c0 = nodes[0];
    auto r0 = nodes[1];
    auto r10 = nodes[2];
    auto r11 = nodes[3];
    auto r2 = nodes[4];
    auto p0 = nodes[5];
    // auto p0 = nodes[4];

    ndn::StackHelperManip ndnHelper;
    ndnHelper.SetDefaultRoutes(false);
    ndnHelper.InstallAll();

    ndn::FibHelper::AddRoute(c0, "/prefix", 257, 1);
    ndn::FibHelper::AddRoute(r0, "/prefix", 258, 1);
    ndn::FibHelper::AddRoute(r0, "/prefix", 259, 1);
    ndn::FibHelper::AddRoute(r10, "/prefix", 258, 1);
    ndn::FibHelper::AddRoute(r11, "/prefix", 258, 1);
    ndn::FibHelper::AddRoute(r2, "/prefix", 259, 1);

    ndn::StrategyChoiceHelper::InstallAll(
        "/prefix", "/localhost/nfd/strategy/best-route-manip/%FD%05");

    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPconManip");
    consumerHelper.SetPrefix("/prefix");
    consumerHelper.SetAttribute("MaxSeq", UintegerValue(50000));
    consumerHelper.SetAttribute("ReactToCongestionMarks", BooleanValue(true));
    consumerHelper.SetAttribute("CcAlgorithm",
                                EnumValue(ndn::CcAlgorithm::CUBIC));
    consumerHelper.SetAttribute("ReactToNack", BooleanValue(false));
    // consumerHelper.SetAttribute("Frequency", StringValue("1000"));
    // consumerHelper.SetAttribute("NumberOfContents", UintegerValue(100));
    auto apps = consumerHelper.Install(c0);

    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix("/prefix");
    producerHelper.SetAttribute("PayloadSize",
                                StringValue(std::to_string(ndn::PKGSIZE)));
    producerHelper.Install(p0);

    // ndn::L3RateTracer::InstallAll("log/l3-rtrace.txt", Seconds(1.0));
    ndn::AppDelayTracer::InstallAll("log/branch-pcon(cubic)-retx.txt");

    Simulator::Stop(Seconds(200.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

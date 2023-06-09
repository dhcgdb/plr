#ifndef PRE_DEF_H
#define PRE_DEF_H
namespace ns3 {
namespace ndn {
constexpr unsigned int PKGSIZE = 1040;
constexpr unsigned int QLENGTH = 64;
constexpr double SQFACTOR = 0.5;
constexpr double RECFACTOR = 0.75;
constexpr double MARKFACTOR = 0.4;
constexpr bool HAVERETX = true;
constexpr unsigned int INTERVALMS = 1;

// don't change below
constexpr unsigned int SQLENGTH = SQFACTOR * QLENGTH;
constexpr unsigned int RECLENGTH = RECFACTOR * QLENGTH;
constexpr unsigned int MARKTHOLD = MARKFACTOR * QLENGTH * 1500;
}  // namespace ndn
}  // namespace ns3
#endif
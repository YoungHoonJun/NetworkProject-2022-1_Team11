#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace ns3;
//using namespace std;

//#define NS3_LOG_ENABLE

/**
 * @brief The test cases include:
 * 1. P2P network with 1 server and 1 client
 * 2. P2P network with 1 server and 2 clients
 * 3. Wireless network with 1 server and 3 mobile clients
 * 4. Wireless network with 3 servers and 3 mobile clients
 * 5. New system(wi-fi)
 * 6. New system(p2p)
 */
#define CASE 6

NS_LOG_COMPONENT_DEFINE ("VideoStreamTest");

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  std::ifstream fin("./scratch/videoStreamer/input.txt");
  std::string line;
  std::vector<std::vector<int>> route = {};

  route.push_back({0, 1});

  getline(fin, line);
  uint32_t nodeNum = line[0] - '0';
  uint32_t bridgeNum = line[2] - '0';
  uint32_t count = bridgeNum;
  while (count>0) {
    getline(fin, line);

    std::vector<int> v_temp = {};

    uint32_t temp1 = line[0] - '0';
    uint32_t temp2 = line[2] - '0';
    temp1 += 1;
    temp2 += 1;
    v_temp.push_back(temp1);
    v_temp.push_back(temp2);

    route.push_back(v_temp);
    count -= 1;
    if (count==0) {
      std::vector<int> v_temp = {};

      const uint32_t temp1 = temp2;
      const uint32_t temp2 = nodeNum + 1;

      v_temp.push_back(temp1);
      v_temp.push_back(temp2);

      route.push_back(v_temp);

    }
  }

  Time::SetResolution (Time::NS);
  LogComponentEnable ("VideoStreamClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("VideoStreamServerApplication", LOG_LEVEL_INFO);

  
  if (CASE == 6) {
    NodeContainer nodes;
    nodes.Create (nodeNum + 2);
    std::string delaytime;
    bridgeNum += 2;

    std::vector<NodeContainer> linkvector(bridgeNum);
    for(uint i=0; i<bridgeNum; i++){
        linkvector[i] = NodeContainer(nodes.Get(route[i][0]),nodes.Get(route[i][1]));
    }

    //std::vector<PointToPointHelper> p2pvector(bridgeNum);
    //for(uint i=0; i<bridgeNum; i++){
    //    p2pvector[i].SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    //    delaytime = std::to_string(route[i][2])+"ms";
    //    p2pvector[i].SetChannelAttribute("Delay", StringValue(delaytime));
    //}
    
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    std::vector<NetDeviceContainer> netvector(bridgeNum);
    for(uint i=0; i<bridgeNum; i++){
        netvector[i] = pointToPoint.Install(linkvector[i]);
    }

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    std::string address_value = "10.1.1.0";
    for(uint i=0; i<bridgeNum; i++){
        int num = route[i][1];
        address_value = "10.1."+std::to_string(num)+".0";
        address.SetBase(Ipv4Address(address_value.c_str()), "255.255.255.0");
        address.Assign(netvector[i]);
    }

    std::vector<Ipv4InterfaceContainer> interfacevector(bridgeNum);
    for(uint i=0; i<bridgeNum; i++){
        interfacevector[i] = address.Assign(netvector[i]);
    }
    
    for(uint k=0; k<bridgeNum; k++){
        VideoStreamClientHelper videoClient (interfacevector[k].GetAddress (0), 5000);
        ApplicationContainer clientApps = videoClient.Install (nodes.Get (route[k][0]));
        clientApps.Start (Seconds (0.0));
        clientApps.Stop (Seconds (100.0));
    }

    VideoStreamServerHelper videoServer (5000);
    videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));
    videoServer.SetAttribute ("FrameFile", StringValue ("./scratch/videoStreamer/small.txt"));
    // videoServer.SetAttribute ("FrameSize", UintegerValue (4096));
    for(uint k=0; k<nodeNum+2; k++){
        ApplicationContainer serverApps = videoServer.Install(nodes.Get(k));
        serverApps.Start(Seconds(0.0));
        serverApps.Stop(Seconds(100.0));
    }
    
    pointToPoint.EnablePcap("videoStream", netvector[0].Get(0),false);
    
    
    Simulator::Run ();
    Simulator::Destroy ();
  }

  fin.close();
  return 0;
}

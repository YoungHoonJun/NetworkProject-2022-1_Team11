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
using namespace std;
#define INF 1e9

uint32_t n, m, start;

vector<pair<uint32_t, uint32_t>> graph[100001];

uint32_t d[100001];
uint32_t from[100001];
uint32_t nodenum, bridgenum;

void dijkstra(uint32_t start) {
    priority_queue<pair<uint32_t, uint32_t>> pq; 

    pq.push({ 0, start });
    d[start] = 0;

    while (!pq.empty()) {
        uint32_t dist = -pq.top().first;
        uint32_t now = pq.top().second;
        pq.pop();

        if (dist > d[now]) continue;

        for (uint32_t i = 0; i < graph[now].size(); i++){
            uint32_t next = graph[now][i].first;
            uint32_t cost = dist + graph[now][i].second;

            if (cost < d[next]) {
                d[next] = cost;
                pq.push(make_pair(-cost, next));

                from[next] = now;
            }
        }
    } 
}

//#define NS3_LOG_ENABLE

/**
 * @brief The test cases include:
 * 1. New system(wi-fi)
 * 2. New system(p2p)
**/

#define CASE 2

NS_LOG_COMPONENT_DEFINE ("VideoStreamTest");

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  ifstream fin("./scratch/videoStreamer/input.txt");
  string line;
  vector<vector<int>> route = {};

  route.push_back({0, 1, 1});

  getline(fin, line);
  const uint32_t nodeNum = line[0] - '0';
  const uint32_t bridgeNum = line[2] - '0';

  while (!fin.eof()) {
    getline(fin, line);

    std::vector<int> v_temp = {};

    const uint32_t temp1 = line[0] - '0';
    const uint32_t temp2 = line[2] - '0';
    const uint32_t temp3 = line[4] - '0';

    v_temp.push_back(temp1);
    v_temp.push_back(temp2);
    v_temp.push_back(temp3);

    route.push_back(v_temp);
    
    if (fin.eof()) {
      std::vector<int> v_temp = {};

      const uint32_t temp1 = nodeNum;
      const uint32_t temp2 = nodeNum + 1;
      const uint32_t temp3 = 1;

      v_temp.push_back(temp1);
      v_temp.push_back(temp2);
      v_temp.push_back(temp3);

      route.push_back(v_temp);
    }
  }

  Time::SetResolution (Time::NS);
  LogComponentEnable ("VideoStreamClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("VideoStreamServerApplication", LOG_LEVEL_INFO);

  if (CASE == 1)
  {
    const uint32_t nWifi = nodeNum + 2, nAp = nodeNum + 2;
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);
    NodeContainer wifiApNode;
    wifiApNode.Create(nAp);
    
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());
  
    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  
    WifiMacHelper mac; 
    Ssid ssid = Ssid ("ns-3-aqiao");  
    mac.SetType ("ns3::StaWifiMac",    
                "Ssid", SsidValue (ssid),   
                "ActiveProbing", BooleanValue (false));  
  
    NetDeviceContainer staDevices;
    staDevices = wifi.Install (phy, mac, wifiStaNodes);  
  
    mac.SetType ("ns3::ApWifiMac",   
                "Ssid", SsidValue (ssid));   
  
    NetDeviceContainer apDevices;
    apDevices = wifi.Install (phy, mac, wifiApNode);   
    MobilityHelper mobility; 
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (0.0),
                                  "MinY", DoubleValue (0.0),
                                  "DeltaX", DoubleValue (50.0),
                                  "DeltaY", DoubleValue (30.0),
                                  "GridWidth", UintegerValue (3),
                                  "LayoutType", StringValue ("RowFirst"));
  
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");  
    mobility.Install (wifiApNode);

    mobility.Install (wifiStaNodes);
  
    InternetStackHelper stack;
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);   
  
    Ipv4AddressHelper address;
  
    address.SetBase ("10.1.3.0", "255.255.255.0");
    
    Ipv4InterfaceContainer apInterfaces;
    apInterfaces = address.Assign (apDevices); 
    Ipv4InterfaceContainer wifiInterfaces;
    wifiInterfaces=address.Assign (staDevices);
                  
    //UdpEchoServerHelper echoServer (9);
    VideoStreamServerHelper videoServer (5000);
    videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));
    videoServer.SetAttribute ("FrameFile", StringValue ("./scratch/videoStreamer/small.txt"));
    for(uint m=0; m<nAp; m++)
    {
      ApplicationContainer serverApps = videoServer.Install (wifiApNode.Get (m));
      serverApps.Start (Seconds (0.0));
      serverApps.Stop (Seconds (100.0));
    }
  
    for(uint k=0; k<bridgeNum+2; k++)
    {
      VideoStreamClientHelper videoClient (apInterfaces.GetAddress (route[k][1]), 5000);
      ApplicationContainer clientApps =
      videoClient.Install (wifiStaNodes.Get (route[k][0]));
      clientApps.Start (Seconds (0.5));
      clientApps.Stop (Seconds (100.0));
    }
  
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
    Simulator::Stop (Seconds (10.0));
  
    phy.EnablePcap ("wifi-videoStream", apDevices.Get (nWifi - 1));
    AnimationInterface anim("wifi-1-3.xml");
    Simulator::Run ();
    Simulator::Destroy ();
  }

  else if (CASE == 2) {
    NodeContainer nodes;
    nodes.Create (nodeNum + 2);
    std::string delaytime;
    const uint32_t bridgenum = bridgeNum + 2;

    std::vector<NodeContainer> linkvector(bridgenum);
    for(uint i=0; i<bridgenum; i++){
        linkvector[i] = NodeContainer(nodes.Get(route[i][0]),nodes.Get(route[i][1]));
    }

    // dif delay
    std::vector<PointToPointHelper> p2pvector(bridgeNum);
    for(uint i=0; i<bridgeNum; i++){
       p2pvector[i].SetDeviceAttribute("DataRate", StringValue("1Mbps"));
       delaytime = std::to_string(route[i][2])+"ms";
       p2pvector[i].SetChannelAttribute("Delay", StringValue(delaytime));
    }
    
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    std::vector<NetDeviceContainer> netvector(bridgenum);
    for(uint i=0; i<bridgenum; i++){
        netvector[i] = pointToPoint.Install(linkvector[i]);
    }

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    std::string address_value = "10.1.1.0";
    for(uint i=0; i<bridgenum; i++){
        int num = route[i][1];
        address_value = "10.1."+std::to_string(num)+".0";
        address.SetBase(Ipv4Address(address_value.c_str()), "255.255.255.0");
        address.Assign(netvector[i]);
    }

    std::vector<Ipv4InterfaceContainer> interfacevector(bridgenum);
    for(uint i=0; i<bridgenum; i++){
        interfacevector[i] = address.Assign(netvector[i]);
    }
    
    for(uint k=0; k<bridgenum; k++){
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
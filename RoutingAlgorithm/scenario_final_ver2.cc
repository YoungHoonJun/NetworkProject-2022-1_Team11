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
uint32_t cost[1001][1001];

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

#define CASE 1

NS_LOG_COMPONENT_DEFINE ("VideoStreamTest");

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  ifstream fin("./scratch/videoStreamer/input.txt");
  string line;
  getline(fin, line);
  uint32_t nodeNum = line[0] - '0';
  uint32_t bridgeNum = line[2] - '0';


  for(uint32_t i = 0; i <bridgeNum; i++){
      getline(fin, line);
      uint32_t temp1 = line[0] - '0';
      uint32_t temp2 = line[2] - '0';
      uint32_t temp3 = line[4] - '0';
      // node a -> node b, cost c
      graph[temp1].push_back({temp2,temp3});
      graph[temp2].push_back({temp1,temp3});
      cost[temp1][temp2] = cost[temp2][temp1] = temp3;
  }
  fill(d, d+100001, INF);
  start = 1;
  dijkstra(start);
  vector<uint32_t> routes;
  uint32_t temp = nodeNum;

  // routes.push_back(nodeNum);
  while(temp){
      routes.push_back(temp);
      temp = from[temp];
  }
  uint32_t len = routes.size()-1;
  vector<vector<uint32_t>> route = {};
  bridgeNum = len;
  printf("%u %u %u\n", bridgeNum, nodeNum, len);


    // server(0->1)
  uint32_t temp1 = 0;
  uint32_t temp2 = 1;
  uint32_t temp3 = 1;
  vector<uint32_t> v_temp1 = {};
  v_temp1.push_back(temp1);
  v_temp1.push_back(temp2);
  v_temp1.push_back(temp3);
  route.push_back(v_temp1);
  printf("%u %u %u\n", temp1, temp2, temp3);

  for(uint32_t i = len; i >= 1; i--){
      vector<uint32_t> v_temp = {};
      const uint32_t temp1 = routes[i];
      const uint32_t temp2 = routes[i-1];
      const uint32_t temp3 = cost[temp1][temp2];

      v_temp.push_back(temp1);
      v_temp.push_back(temp2);
      v_temp.push_back(temp3);
      route.push_back(v_temp);
      printf("%u %u %u\n", temp1, temp2, temp3);
  }
  vector<uint32_t> v_temp = {};
  temp1 = nodeNum;
  temp2 = nodeNum+1;
  temp3 = 1;

    // client(n->n+1)
  v_temp.push_back(temp1);
  v_temp.push_back(temp2);
  v_temp.push_back(temp3);
  route.push_back(v_temp);
  printf("%u %u %u---\n", temp1, temp2, temp3);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("VideoStreamClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("VideoStreamServerApplication", LOG_LEVEL_INFO);
  if (CASE == 1)
  {
    printf("\n\nnodeNUm = %u\n", nodeNum+2);
      
    const uint32_t nWifi = nodeNum + 2, nAp = nodeNum + 2;
    printf("%u : %u : %u", bridgeNum, nAp, nAp);
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);
    NodeContainer wifiApNode;
    wifiApNode.Create(nAp);
    printf("%u : %u : %u", bridgeNum, nAp, nAp);
    
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
    
    printf("%u : %u : %u", bridgeNum, nAp, nAp);
    for(uint m=0; m<nAp; m++)
    {
      ApplicationContainer serverApps = videoServer.Install (wifiApNode.Get (m));
      serverApps.Start (Seconds (0.0));
      serverApps.Stop (Seconds (100.0));
    }
    printf("%u : %u : %u", bridgeNum, nAp, nAp);
    for(uint k=0; k<bridgeNum; k++)
    {
      printf("%u : %u : %u", route[k][0], route[k][0], route[k][0]);
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
    printf("%u", bridgenum);
    for(uint i=0; i<bridgenum; i++){
        printf("%u %u\n", route[i][0], route[i][1]);
        linkvector[i] = NodeContainer(nodes.Get(route[i][0]),nodes.Get(route[i][1]));
    }

    // dif delay
    std::vector<PointToPointHelper> p2pvector(bridgeNum);
    for(uint i=0; i<bridgeNum; i++){
        printf("%u\n", route[i][2]);
       p2pvector[i].SetDeviceAttribute("DataRate", StringValue("1Mbps"));
       delaytime = std::to_string(route[i][2])+"ms";
       p2pvector[i].SetChannelAttribute("Delay", StringValue(delaytime));
    }
    
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    std::vector<NetDeviceContainer> netvector(bridgenum);
    for(uint i=0; i<bridgenum; i++){
        printf("%u\n", i);
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

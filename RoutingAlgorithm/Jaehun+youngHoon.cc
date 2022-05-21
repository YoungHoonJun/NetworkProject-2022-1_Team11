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
#include <queue>

using namespace ns3;
using namespace std;
#define INF 1e9 // 무한을 의미하는 값으로 10억을 설정

// 노드 개수: n, 간선 개수: m, 시작 노드 번호: start
// 노드의 개수는 최대 100,000개라고 가정
uint32_t n, m, start;

// 각 노드에 연결되어 있는 노드에 대한 정보를 담는 배열
// (인접 노드 번호, 가중치)
std::vector<std::pair<uint32_t, uint32_t>> graph[100001];

// 최단 거리 테이블 만들기
uint32_t d[100001];
uint32_t from[100001];
uint32_t nodenum, bridgenum;

void dijkstra(uint32_t start) {
    std::priority_queue<std::pair<uint32_t, uint32_t>> pq; 
    // 기본적으로 최대 힙이기 때문에
    // 거리가 가장 짧은 노드부터 먼저 꺼내는 '최소 힙'으로 구현하려면
    // 원소를 삽입, 삭제할 때 마이너스 부호를 붙여줘야 한다.

    // 시작 노드로 가기 위한 최단 경로는 0으로 설정하여, 큐에 삽입
    // (거리, 노드 번호)
    pq.push({ 0, start });
    d[start] = 0;

    while (!pq.empty()) {
        // 최단 거리가 가장 짧은 노드에 대한 정보 꺼내기
        uint32_t dist = -pq.top().first; // 시작 노드에서 현재 노드까지의 거리
        uint32_t now = pq.top().second; // 현재 노드 번호
        pq.pop();

        // 현재 노드가 이미 처리된 적이 있는 노드라면 무시
        if (dist > d[now]) continue;

        // 현재 노드와 연결된 다른 인접 노드들을 확인
        for (uint32_t i = 0; i < graph[now].size(); i++){
            uint32_t next = graph[now][i].first;
            uint32_t cost = dist + graph[now][i].second;

            // 현재 노드들을 거쳐서 다른 노드로 이동하는 거리가 더 짧은 경우
            if (cost < d[next]) {
                d[next] = cost;
                pq.push(std::make_pair(-cost, next));
                // 경로 저장(next->now)
                from[next] = now;
            }
        }
    } 
}

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
#define CASE 5

NS_LOG_COMPONENT_DEFINE ("VideoStreamTest");

int main (int argc, char *argv[]) {
  CommandLine cmd;
  cmd.Parse (argc, argv);

  const uint32_t start = 0;

  ifstream fin("ns-3.29/scratch/input.txt");
  string line;
  getline(fin, line);
  nodenum = line[0] - '0';
  bridgenum = line[2] - '0';


  for(uint32_t i = 0; i <bridgenum; i++){
      getline(fin, line);
      const uint32_t temp1 = line[0] - '0';
      const uint32_t temp2 = line[2] - '0';
      const uint32_t temp3 = line[4] - '0';
      // node a -> node b, cost c
      graph[temp1].push_back({temp2,temp3});
      graph[temp2].push_back({temp1,temp3});
  }

  fill(d, d+100001, INF);

  dijkstra(start);
  std::vector<uint32_t> routes;
  uint32_t temp = nodenum - 1;

  routes.push_back(nodenum);
  while(temp){
      routes.push_back(temp);
      temp = from[temp];
  }
  routes.push_back(0);
  uint32_t len = routes.size();
  std::vector<std::vector<int>> route = {};

  for(uint32_t i = len-1; i >= 1; i--){
      std::vector<int> v_temp = {};
      const uint32_t temp1 = routes[i];
      const uint32_t temp2 = routes[i-1];

      v_temp.push_back(temp1);
      v_temp.push_back(temp2);
      route.push_back(v_temp);
      printf("%d %d\n", temp1, temp2);
  }
  return 0;

  Time::SetResolution (Time::NS);
  LogComponentEnable ("VideoStreamClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("VideoStreamServerApplication", LOG_LEVEL_INFO);

  if (CASE == 1)
  {
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    VideoStreamClientHelper videoClient (interfaces.GetAddress (0), 5000);
    ApplicationContainer clientApp = videoClient.Install (nodes.Get (1));
    clientApp.Start (Seconds (0.5));
    clientApp.Stop (Seconds (100.0));

    VideoStreamServerHelper videoServer (5000);
    videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));
    videoServer.SetAttribute ("FrameFile", StringValue ("./scratch/videoStreamer/frameList.txt"));
    // videoServer.SetAttribute ("FrameSize", UintegerValue (4096));

    ApplicationContainer serverApp = videoServer.Install (nodes.Get (0));
    serverApp.Start (Seconds (0.0));
    serverApp.Stop (Seconds (100.0));

    pointToPoint.EnablePcap ("videoStream", devices.Get (1), false);
    Simulator::Run ();
    Simulator::Destroy ();
  }
  else if (CASE == 2)
  {
    NodeContainer nodes;
    nodes.Create (3);
    NodeContainer n0n1= NodeContainer (nodes.Get(0), nodes.Get(1));
    NodeContainer n0n2= NodeContainer (nodes.Get(0), nodes.Get(2));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer d0d1= pointToPoint.Install (n0n1);
    NetDeviceContainer d0d2= pointToPoint.Install (n0n2);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    address.Assign (d0d1);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    address.Assign (d0d2);

    //Ipv4InterfaceContainer interfaces = address.Assign (devices);
    Ipv4InterfaceContainer i0i1 = address.Assign (d0d1);
    Ipv4InterfaceContainer i0i2 = address.Assign (d0d2);

    VideoStreamClientHelper videoClient1 (i0i1.GetAddress (0), 5000);
    ApplicationContainer clientApp1 = videoClient1.Install (nodes.Get (1));
    clientApp1.Start (Seconds (1.0));
    clientApp1.Stop (Seconds (100.0));

    VideoStreamClientHelper videoClient2 (i0i2.GetAddress (0), 5000);
    ApplicationContainer clientApp2 = videoClient2.Install (nodes.Get (2));
    clientApp2.Start (Seconds (0.5));
    clientApp2.Stop (Seconds (100.0));

    VideoStreamServerHelper videoServer (5000);
    videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));
    videoServer.SetAttribute ("FrameFile", StringValue ("./scratch/videoStreamer/small.txt"));
    // videoServer.SetAttribute ("FrameSize", UintegerValue (4096));

    ApplicationContainer serverApp = videoServer.Install (nodes.Get (0));
    serverApp.Start (Seconds (0.0));
    serverApp.Stop (Seconds (100.0));

    pointToPoint.EnablePcap ("videoStream", d0d1.Get (1), false);
    pointToPoint.EnablePcap ("videoStream", d0d2.Get (1), false);
    Simulator::Run ();
    Simulator::Destroy ();
  }
  else if (CASE == 3)
  {
    const uint32_t nWifi = 3, nAp = 1;
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
                                "DeltaX", DoubleValue (30.0),
                                "DeltaY", DoubleValue (30.0),
                                "GridWidth", UintegerValue (2),
                                "LayoutType", StringValue ("RowFirst"));

    
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));   
    mobility.Install (wifiStaNodes);
  
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");  
    mobility.Install (wifiApNode);
  
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
  
    for(uint k=0; k<nWifi; k++)
    {
      VideoStreamClientHelper videoClient (apInterfaces.GetAddress (0), 5000);
      ApplicationContainer clientApps =
      videoClient.Install (wifiStaNodes.Get (k));
      clientApps.Start (Seconds (0.5));
      clientApps.Stop (Seconds (100.0));
    }
  
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
    Simulator::Stop (Seconds (10.0));
  
    phy.EnablePcap ("wifi-videoStream", apDevices.Get (0));
    AnimationInterface anim("wifi-1-3.xml");
    Simulator::Run ();
    Simulator::Destroy ();
  }
  else if (CASE == 4)
  {
    const uint32_t nWifi = 3, nAp = 3;
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
      
    //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",   
    //                           "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));   
    //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");  
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
  
    for(uint k=0; k<nWifi; k++)
    {
      VideoStreamClientHelper videoClient (apInterfaces.GetAddress (k), 5000);
      ApplicationContainer clientApps =
      videoClient.Install (wifiStaNodes.Get (k));
      clientApps.Start (Seconds (0.5));
      clientApps.Stop (Seconds (100.0));
    }
  
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
    Simulator::Stop (Seconds (10.0));
  
    phy.EnablePcap ("wifi-videoStream", apDevices.Get (0));
    AnimationInterface anim("wifi-1-3.xml");
    Simulator::Run ();
    Simulator::Destroy ();
  }
  else if (CASE == 5)
  {
    const uint32_t nWifi = nodenum + 2, nAp = len;
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
      
    //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",   
    //                           "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));   
    //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");  
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
  
    for(uint k=0; k<bridgenum+2; k++)
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
  else if (CASE == 6) {
      //seung++
  }

  fin.close();
  return 0;
}

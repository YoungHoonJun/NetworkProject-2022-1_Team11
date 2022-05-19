/*****************************************************
*
* File:  videoStreamTest.cc
*
* Explanation:  This script modifies the tutorial first.cc
*               to test the video stream application.
*
*****************************************************/
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
#include <queue>
#include <vector>

using namespace ns3;
#define INF 1e9 // 무한을 의미하는 값으로 10억을 설정
using namespace std;

// 노드 개수: n, 간선 개수: m, 시작 노드 번호: start
// 노드의 개수는 최대 100,000개라고 가정
int n, m, start;

// 각 노드에 연결되어 있는 노드에 대한 정보를 담는 배열
// (인접 노드 번호, 가중치)
vector<pair<int, int>> graph[100'001];

// 최단 거리 테이블 만들기
int d[100'001];
int route[100'001];

void dijkstra(int start) {
    priority_queue<pair<int, int>> pq; 
    // 기본적으로 최대 힙이기 때문에
    // 거리가 가장 짧은 노드부터 먼저 꺼내는 '최소 힙'으로 구현하려면
    // 원소를 삽입, 삭제할 때 마이너스 부호를 붙여줘야 한다.

    // 시작 노드로 가기 위한 최단 경로는 0으로 설정하여, 큐에 삽입
    // (거리, 노드 번호)
    pq.push({ 0, start });
    d[start] = 0;

    while (!pq.empty()) {
        // 최단 거리가 가장 짧은 노드에 대한 정보 꺼내기
        int dist = -pq.top().first; // 시작 노드에서 현재 노드까지의 거리
        int now = pq.top().second; // 현재 노드 번호
        pq.pop();

        // 현재 노드가 이미 처리된 적이 있는 노드라면 무시
        if (dist > d[now]) continue;

        // 현재 노드와 연결된 다른 인접 노드들을 확인
        for (int i = 0; i < graph[now].size(); i++){
            int next = graph[now][i].first
            int cost = dist + graph[now][i].second;

            // 현재 노드들을 거쳐서 다른 노드로 이동하는 거리가 더 짧은 경우
            if (cost < d[graph[next]) {
                d[next] = cost;
                pq.push(make_pair(-cost, next));
                // 경로 저장(next->now)
                route[next] = now;
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
 */
#define CASE 0

NS_LOG_COMPONENT_DEFINE ("VideoStreamTest");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("VideoStreamClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("VideoStreamServerApplication", LOG_LEVEL_INFO);

  if (CASE == 1)
  {
    NodeContainer nodes;
    nodes.Create (3);
	NodeContainer n0n1 = NodeContainer(nodes.Get(0), nodes.Get(1));
	NodeContainer n0n2 = NodeContainer(nodes.Get(0), nodes.Get(2));


    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));


    NetDeviceContainer devices_0;
	NetDeviceContainer devices_1;
    devices_0 = pointToPoint.Install (n0n1);
	devices_1 = pointToPoint.Install (n0n2);


    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces_0 = address.Assign (devices_0);

	address.SetBase ("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaces_1 = address.Assign (devices_1);


    VideoStreamClientHelper videoClient (interfaces_0.GetAddress (1), 5000);
    ApplicationContainer clientApp_1 = videoClient.Install (nodes.Get (0));
    clientApp_1.Start (Seconds (0.5));
    clientApp_1.Stop (Seconds (2.0));

	videoClient.SetAttribute ("RemoteAddress", AddressValue(interfaces_1.GetAddress(1)) );
	videoClient.SetAttribute ("RemotePort", UintegerValue(5000));
	ApplicationContainer clientApp_2 = videoClient.Install (nodes.Get (0));
	clientApp_2.Start (Seconds (8.0));
	clientApp_2.Stop (Seconds (10.0));



    VideoStreamServerHelper videoServer (5000);
    videoServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));
    videoServer.SetAttribute ("FrameFile", StringValue ("./scratch/videoStreamer/frameList.txt"));
    // videoServer.SetAttribute ("FrameSize", UintegerValue (4096));

    ApplicationContainer serverApp_1 = videoServer.Install (nodes.Get (1));
	
    serverApp_1.Start (Seconds (0.0));
    serverApp_1.Stop (Seconds (100.0));

	ApplicationContainer serverApp_2 = videoServer.Install (nodes.Get (2));

	serverApp_2.Start (Seconds (0.0));
	serverApp_2.Stop (Seconds (100.0));

    pointToPoint.EnablePcap ("videoStream", devices_0.Get(1), false);
	pointToPoint.EnablePcap ("videoStream", devices_1.Get(1), false);

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

  else if (CASE == 0)
  {
    FILE *fp; // 파일 입출력
    fp = fopen("input.txt","r");
    fp2 = fopen("route.txt","w");
    fscanf(fp,"%d %d", &n, &m)
    uint32_t start = 0

    // ifstream fileStream("input.txt");
    // getline(fileStream, line)

    for (int i = 0; i < m; i++) {
        int a, b, c;
        fscanf(fp, "%d %d %d", &a, &b, &c)

        // a번 노드에서 b번 노드로 가는 비용이 c라는 의미
        graph[a].push_back({ b, c });
        graph[b].push_back({ a, c }); 
        // 양방향이므로 거꾸로도 추가해줌
    }

    // 최단 거리 테이블을 모두 무한으로 초기화
    fill(d, d + 100'001, INF);

    // 다익스트라 알고리즘을 수행
    dijkstra(start);

    vector<int> routes;
    int temp = n-1;
    while(temp) {
      routes.push_back(temp)
      temp = route[temp];
    }

    // 경로 길이 및 path 출력
    int len = routes.size();
    fprinf(fp2,"%d", len);
    
    for(int i=len-1;i>=0;i--)
      fprintf(fp2,"%d ",route[i]);

    

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

  return 0;
}

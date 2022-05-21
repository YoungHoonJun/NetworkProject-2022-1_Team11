/*****************************************************
*
* File:  mosTest.cc
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

using namespace ns3;

//#define NS3_LOG_ENABLE

/**
 * @brief The test cases include:
 * 1. P2P network with 1 server and 1 client
 * 2. P2P network with 1 server and 2 clients
 * 3. Wireless network with 1 server and 3 mobile clients
 * 4. Wireless network with 3 servers and 3 mobile clients
 */
#define CASE 1

NS_LOG_COMPONENT_DEFINE ("mosTest");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("MosClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("MosServerApplication", LOG_LEVEL_INFO);

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


    MosClientHelper mosClient (interfaces_0.GetAddress (1), 5000);
	mosClient.SetAttribute ("TextFile", StringValue("./scratch/mos/text1.txt"));
    ApplicationContainer clientApp_1 = mosClient.Install (nodes.Get (0));
    clientApp_1.Start (Seconds (0.5));
    clientApp_1.Stop (Seconds (1000.0));

	/*
	mosClient.SetAttribute ("RemoteAddress", AddressValue(interfaces_1.GetAddress(1)) );
	mosClient.SetAttribute ("RemotePort", UintegerValue(5000));
	mosClient.SetAttribute ("TextFile", StringValue("./scratch/mos/text2.txt"));
	ApplicationContainer clientApp_2 = mosClient.Install (nodes.Get (0));
	clientApp_2.Start (Seconds (0.5));
	clientApp_2.Stop (Seconds (0.6));
	*/

    MosServerHelper mosServer (5000);
    mosServer.SetAttribute ("MaxPacketSize", UintegerValue (1400));

    ApplicationContainer serverApp_1 = mosServer.Install (nodes.Get (1));
	
    serverApp_1.Start (Seconds (0.0));
    serverApp_1.Stop (Seconds (1000.0));

	ApplicationContainer serverApp_2 = mosServer.Install (nodes.Get (2));

	serverApp_2.Start (Seconds (0.0));
	serverApp_2.Stop (Seconds (1000.0));

    pointToPoint.EnablePcap ("videoStream", devices_0.Get(1), false);
	pointToPoint.EnablePcap ("videoStream", devices_1.Get(1), false);
    Simulator::Run ();
    Simulator::Destroy ();
  }

  return 0;
}

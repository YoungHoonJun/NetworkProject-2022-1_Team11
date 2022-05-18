/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/mos-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MosServerApplication");

NS_OBJECT_ENSURE_REGISTERED (MosServer);

TypeId
MosServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MosServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<MosServer> ()
    .AddAttribute ("Interval", "The time to wait between packets",
                    TimeValue (Seconds (0.01)),
                    MakeTimeAccessor (&MosServer::m_interval),
                    MakeTimeChecker ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                    UintegerValue (5000),
                    MakeUintegerAccessor (&MosServer::m_port),
                    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MaxPacketSize", "The maximum size of a packet",
                    UintegerValue (1400),
                    MakeUintegerAccessor (&MosServer::m_maxPacketSize),
                    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("FrameFile", "The file that contains the video frame sizes",
                    StringValue (""),
                    MakeStringAccessor (&MosServer::SetFrameFile, &MosServer::GetFrameFile),
                    MakeStringChecker ())
    ;
    return tid;
}

MosServer::MosServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_frameSizeList = std::vector<uint32_t>();
}

MosServer::~MosServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void 
MosServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
MosServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
    if (m_socket->Bind (local) == -1)
    {
      NS_FATAL_ERROR ("Failed to bind socket");
    }
    if (addressUtils::IsMulticast (m_local))
    {
      Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
      if (udpSocket)
      {
        udpSocket->MulticastJoinGroup (0, m_local);
      }
      else
      {
        NS_FATAL_ERROR ("Error: Failed to join multicast group");
      }
    }
  }

  m_socket->SetAllowBroadcast (true);
  m_socket->SetRecvCallback (MakeCallback (&MosServer::HandleRead, this));
}

void
MosServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);


  if (m_socket != 0)
  {
    m_socket->Close();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    m_socket = 0;
  }

  for (auto iter = m_clients.begin (); iter != m_clients.end (); iter++)
  {
    Simulator::Cancel (iter->second->m_sendEvent);
  }
  
}

void
MosServer::SetFrameFile (std::string frameFile)
{
  NS_LOG_FUNCTION (this << frameFile);
  m_frameFile = frameFile;
  if (frameFile != "")
  {
    std::string line;
    std::ifstream fileStream(frameFile);
    while (std::getline (fileStream, line))
    {
      int result = std::stoi(line);
      m_frameSizeList.push_back (result);
    }
  }
  NS_LOG_INFO ("Frame list size: " << m_frameSizeList.size());
}

std::string
MosServer::GetFrameFile (void) const
{
  NS_LOG_FUNCTION (this);
  return m_frameFile;
}

void
MosServer::SetMaxPacketSize (uint32_t maxPacketSize)
{
  m_maxPacketSize = maxPacketSize;
}

uint32_t
MosServer::GetMaxPacketSize (void) const
{
  return m_maxPacketSize;
}

void 
MosServer::Send (uint32_t ipAddress)
{
  NS_LOG_FUNCTION (this);

  uint32_t totalFrames;
  ClientInfo *clientInfo = m_clients.at (ipAddress);

  //NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server about to send");
  if (m_lastTime.find(ipAddress) != m_lastTime.end()){ //that ip has time log
	  double lastTime = m_lastTime.at (ipAddress);
	  if (Simulator::Now().GetSeconds() - lastTime > 3.0){
		  Simulator::Cancel(clientInfo->m_sendEvent);
          return;
	  }
  }
  



  NS_ASSERT (clientInfo->m_sendEvent.IsExpired ());
  totalFrames = m_frameSizeList.size ();
  
  // long line packet dividing function would be needed
  SendPacket (clientInfo, m_maxPacketSize, (char*)"www");
  

  clientInfo->m_sent += 1;
  //if (clientInfo->m_sent < totalFrames)
  //{
    clientInfo->m_sendEvent = Simulator::Schedule (m_interval, &MosServer::Send, this, ipAddress);
  //}
}

void 
MosServer::SendPacket (ClientInfo *client, uint32_t packetSize, char* targetLine)
{
  uint8_t dataBuffer[packetSize];
  sprintf ((char*)dataBuffer, "%s", targetLine);
  Ptr<Packet> p = Create<Packet> (dataBuffer, packetSize);
  if (m_socket->SendTo (p, 0, client->m_address) < 0)
  {
    NS_LOG_INFO ("Error while sending " << packetSize << "bytes to " << InetSocketAddress::ConvertFrom (client->m_address).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (client->m_address).GetPort ());
  }
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s SEVER SEND" << dataBuffer);
}

void 
MosServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
  {
    socket->GetSockName (localAddress);
    if (InetSocketAddress::IsMatchingType (from))
    {
	  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());

	  // uint8_t recvData[packet->GetSize()];
	  // packet->CopyData (recvData, packet->GetSize());
	  // printf(recvData);
	  //// now, just server send the data "wwww"

      uint32_t ipAddr = InetSocketAddress::ConvertFrom (from).GetIpv4 ().Get ();

      // the first time we received the message from the client
      if (m_clients.find (ipAddr) == m_clients.end ())
      {
        ClientInfo *newClient = new ClientInfo();
        newClient->m_sent = 0;
        newClient->m_address = from;
        // newClient->m_sendEvent = EventId ();
        m_clients[ipAddr] = newClient;
        newClient->m_sendEvent = Simulator::Schedule (Seconds (0.0), &MosServer::Send, this, ipAddr);
		
		m_lastTime[ipAddr] = Simulator::Now ().GetSeconds ();
      }
      else
      {
		m_lastTime. at(ipAddr) = Simulator::Now ().GetSeconds ();

        //uint8_t dataBuffer[10];
        //packet->CopyData (dataBuffer, 10);

        //uint16_t videoLevel;
        //sscanf((char *) dataBuffer, "%hu", &videoLevel);
        // NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received video level " << videoLevel);
        //m_clients.at (ipAddr)->m_videoLevel = videoLevel;
      }
    }
  }
}

} // namespace ns3

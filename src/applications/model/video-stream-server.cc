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
#include "ns3/video-stream-server.h"
#include "ns3/rtp-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VideoStreamServerApplication");

NS_OBJECT_ENSURE_REGISTERED (VideoStreamServer);

TypeId
VideoStreamServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VideoStreamServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<VideoStreamServer> ()
    .AddAttribute ("Interval", "The time to wait between packets",
                    TimeValue (Seconds (0.01)),
                    MakeTimeAccessor (&VideoStreamServer::m_interval),
                    MakeTimeChecker ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                    UintegerValue (5000),
                    MakeUintegerAccessor (&VideoStreamServer::m_port),
                    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MaxPacketSize", "The maximum size of a packet",
                    UintegerValue (1400),
                    MakeUintegerAccessor (&VideoStreamServer::m_maxPacketSize),
                    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("FrameFile", "The file that contains the video frame sizes",
                    StringValue (""),
                    MakeStringAccessor (&VideoStreamServer::SetFrameFile, &VideoStreamServer::GetFrameFile),
                    MakeStringChecker ())
    .AddAttribute ("VideoLength", "The length of the video in seconds",
                    UintegerValue (60),
                    MakeUintegerAccessor (&VideoStreamServer::m_videoLength),
                    MakeUintegerChecker<uint32_t> ())
    ;
    return tid;
}

VideoStreamServer::VideoStreamServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_frameRate = 25;
  m_frameSizeList = std::vector<uint32_t>();
}

VideoStreamServer::~VideoStreamServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void 
VideoStreamServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
VideoStreamServer::StartApplication (void)
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
  m_socket->SetRecvCallback (MakeCallback (&VideoStreamServer::HandleRead, this));
}

void
VideoStreamServer::StopApplication ()
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
VideoStreamServer::SetFrameFile (std::string frameFile)
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
VideoStreamServer::GetFrameFile (void) const
{
  NS_LOG_FUNCTION (this);
  return m_frameFile;
}

void
VideoStreamServer::SetMaxPacketSize (uint32_t maxPacketSize)
{
  m_maxPacketSize = maxPacketSize;
}

uint32_t
VideoStreamServer::GetMaxPacketSize (void) const
{
  return m_maxPacketSize;
}

void 
VideoStreamServer::Send (uint32_t ipAddress)
{
  NS_LOG_FUNCTION (this);

  uint32_t frameSize, totalFrames;
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
  // If the frame sizes are not from the text file, and the list is empty
  if (m_frameSizeList.empty ())
  {
    frameSize = m_frameSizes[clientInfo->m_videoLevel];
    totalFrames = m_videoLength * m_frameRate;
  }
  else
  {
    frameSize = m_frameSizeList[clientInfo->m_sent] * clientInfo->m_videoLevel;
    totalFrames = m_frameSizeList.size ();
  }

  // the frame might require several packets to send
  if (clientInfo->m_isRTP)
  {
    clientInfo->m_FrameLastSeq += (frameSize / m_maxPacketSize) + 1;
  }

  for (uint i = 0; i < frameSize / m_maxPacketSize; i++)
  {
    SendPacket (clientInfo, m_maxPacketSize);
  }
  uint32_t remainder = frameSize % m_maxPacketSize;
  SendPacket (clientInfo, remainder);

  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent frame " << clientInfo->m_sent << " and " << frameSize << " bytes to " << InetSocketAddress::ConvertFrom (clientInfo->m_address).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (clientInfo->m_address).GetPort ());

  clientInfo->m_sent += 1;
  if (clientInfo->m_sent < totalFrames)
  {
    clientInfo->m_sendEvent = Simulator::Schedule (m_interval, &VideoStreamServer::Send, this, ipAddress);
  }
}

void 
VideoStreamServer::SendPacket (ClientInfo *client, uint32_t packetSize)
{
  uint8_t dataBuffer[packetSize];
  sprintf ((char *) dataBuffer, "%u", client->m_sent);
  Ptr<Packet> p = Create<Packet> (dataBuffer, packetSize);
  if(client->m_isRTP)
  {
    client->m_lastSeq += 1;
    RtpHeader hdr;
    hdr.SetSquence (client->m_lastSeq);
    hdr.SetLastFrameSquence (client->m_FrameLastSeq);
    p->AddHeader (hdr);
    client->m_queue->push_back (*p);
    while(client->m_queue->size () > m_maxRtpQueueLen)
    {
      client->m_queue->pop_front ();
    }
  }
  if (m_socket->SendTo (p, 0, client->m_address) < 0)
  {
    NS_LOG_INFO ("Error while sending " << packetSize << "bytes to " << InetSocketAddress::ConvertFrom (client->m_address).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (client->m_address).GetPort ());
  }
}

void 
VideoStreamServer::HandleRead (Ptr<Socket> socket)
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

      uint32_t ipAddr = InetSocketAddress::ConvertFrom (from).GetIpv4 ().Get ();

      // the first time we received the message from the client
      if (m_clients.find (ipAddr) == m_clients.end ())
      {
        ClientInfo *newClient = new ClientInfo();
        newClient->m_sent = 0;
        newClient->m_videoLevel = 3;
        newClient->m_address = from;
        newClient->m_sendEvent = EventId ();
        m_clients[ipAddr] = newClient;
        newClient->m_sendEvent = Simulator::Schedule (Seconds (0.0), &VideoStreamServer::Send, this, ipAddr);
        newClient->m_lastSeq = 0;

        // read first got packet to determine it is RTP client or not.
        uint8_t dataBuffer[10];
        packet->CopyData (dataBuffer, 10);
        uint16_t det;
        sscanf((char *) dataBuffer, "%hu", &det);
        if (det == 0)
          newClient->m_isRTP = false;
        else if (det == 1)
        {
          newClient->m_isRTP = true;
          m_maxRtpQueueLen = 1000;
          newClient->m_queue = new std::list<Packet>;
        }
		
		    m_lastTime[ipAddr] = Simulator::Now ().GetSeconds ();
      }
      else
      {
        ClientInfo *currentClient = m_clients.at(ipAddr);
		    m_lastTime[ipAddr] = Simulator::Now ().GetSeconds ();

        uint32_t lostSeq = 0;
        if(currentClient->m_isRTP)
        {
          RtpHeader hdr;
          packet->RemoveHeader(hdr);
          lostSeq = hdr.GetSquence();

          if(lostSeq > 0)
          {
            RtpHeader pivHdr;
            std::list<Packet>::iterator iter = currentClient->m_queue->begin();
            iter->PeekHeader(pivHdr);

            while (pivHdr.GetSquence() < lostSeq)
            {
              iter ++;
              iter->PeekHeader(pivHdr);
            }

            Packet retransPacket = *iter;

            NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server retransmit lost sequence " << lostSeq << " to " << ipAddr);
            if (m_socket->SendTo (&retransPacket, 0, currentClient->m_address) < 0)
            {
              NS_LOG_INFO ("Error while retransmit seq " << lostSeq << " to " << ipAddr);
            }
            return;
          }
        }
        uint8_t dataBuffer[10];
        packet->CopyData (dataBuffer, 10);

        uint16_t videoLevel;
        sscanf ((char *) dataBuffer, "%hu", &videoLevel);

        if (videoLevel == (unsigned short int) 10){		// if it is alive signal
          return;		//do nothing
        }
        if (videoLevel == (unsigned short int) 11){		// if it is death signal
          Simulator::Cancel (currentClient->m_sendEvent);		//stop sending
          m_clients.erase (ipAddr);
          delete currentClient->m_queue;
          delete currentClient;
          return;
        }
		
        NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received video level " << videoLevel);

        m_clients.at (ipAddr)->m_videoLevel = videoLevel;
      }
    }
  }
}

} // namespace ns3

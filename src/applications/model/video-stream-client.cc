/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "video-stream-client.h"
#include "ns3/rtp-header.h"
#include "ns3/boolean.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VideoStreamClientApplication");

NS_OBJECT_ENSURE_REGISTERED (VideoStreamClient);

TypeId
VideoStreamClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VideoStreamClient")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<VideoStreamClient> ()
    .AddAttribute ("RemoteAddress", "The destination address of the outbound packets",
                    AddressValue (),
                    MakeAddressAccessor (&VideoStreamClient::m_peerAddress),
                    MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                    UintegerValue (5000),
                    MakeUintegerAccessor (&VideoStreamClient::m_peerPort),
                    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("IsRTP", "true if the client wants to use RTP",
                    BooleanValue(false),
                    MakeBooleanAccessor (&VideoStreamClient::m_isRTP),
                    MakeBooleanChecker ())
  ;
  return tid;
}

VideoStreamClient::VideoStreamClient ()
{
  NS_LOG_FUNCTION (this);
  m_initialDelay = 3;
  m_lastBufferSize = 0;
  m_currentBufferSize = 0;
  m_frameSize = 0;
  m_frameRate = 25;
  m_videoLevel = 3;
  m_stopCounter = 0;
  m_lastRecvFrame = 1e6;
  m_rebufferCounter = 0;
  m_bufferEvent = EventId();
  m_sendEvent = EventId();
  m_reReqDelay = 20000;
  m_minSeq = 1;
  m_maxSeq = 0;
}

VideoStreamClient::~VideoStreamClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
VideoStreamClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void 
VideoStreamClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
VideoStreamClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
VideoStreamClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);
    if (Ipv4Address::IsMatchingType (m_peerAddress) == true)
    {
      if (m_socket->Bind () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
      m_socket->Connect (InetSocketAddress(Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
    }
    else if (Ipv6Address::IsMatchingType (m_peerAddress) == true)
    {
      if (m_socket->Bind6 () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
      m_socket->Connect (m_peerAddress);
    }
    else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
	{  
      if (m_socket->Bind () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
      m_socket->Connect (m_peerAddress);
    }
    else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
    {
      if (m_socket->Bind6 () == -1)
      {
        NS_FATAL_ERROR ("Failed to bind socket");
      }
      m_socket->Connect (m_peerAddress);
    }
    else
    {
      NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
    }
  }

  m_socket->SetRecvCallback (MakeCallback (&VideoStreamClient::HandleRead, this));
  m_sendEvent = Simulator::Schedule (MilliSeconds (1.0), &VideoStreamClient::Send, this);
  m_bufferEvent = Simulator::Schedule (Seconds (m_initialDelay), &VideoStreamClient::ReadFromBuffer, this);
}

void
VideoStreamClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
  {
    // sending 11 means client quits
    uint8_t dataBuffer[10];
    sprintf((char*) dataBuffer, "%hu", (unsigned short int) 11);
    Ptr<Packet> lastPacket = Create<Packet> (dataBuffer, 10);

    if (m_isRTP)
    {
      RtpHeader hdr;
      lastPacket->AddHeader(hdr);
    }

    m_socket->Send (lastPacket);
    NS_LOG_INFO("Stop signal sent from client");
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    m_socket = 0;
  }

  Simulator::Cancel (m_bufferEvent);
}

void
VideoStreamClient::ChangeServer (Address s_ip, uint16_t s_port)
{
	NS_LOG_FUNCTION (this);
	
	this->StopApplication();
	this->SetRemote(s_ip, s_port);
	this->StartApplication();
}

void
VideoStreamClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  uint8_t dataBuffer[10];

  uint8_t sig = 2;
  if(m_isRTP) // rtp client라는 것을 1 signal로 서버에 알림.
    sig = 1;

  sprintf((char *) dataBuffer, "%hu", (unsigned short int)sig);
  Ptr<Packet> firstPacket = Create<Packet> (dataBuffer, 10);
  m_socket->Send (firstPacket);

  if (Ipv4Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }
  else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  Inet6SocketAddress::ConvertFrom (m_peerAddress).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }
}

uint32_t 
VideoStreamClient::ReadFromBuffer (void)
{
  // NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " s, last buffer size: " << m_lastBufferSize << ", current buffer size: " << m_currentBufferSize);
  if (m_currentBufferSize < m_frameRate) 
  {

    if (m_lastBufferSize == m_currentBufferSize)
    {
      m_stopCounter++;
      // If the counter reaches 3, which means the client has been waiting for 3 sec, and no packets arrived.
      // In this case, we think the video streaming has finished, and there is no need to schedule the event.
      if (m_stopCounter < 3)
      {
        m_bufferEvent = Simulator::Schedule (Seconds (1.0), &VideoStreamClient::ReadFromBuffer, this);
      }
    }
    else
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " s: Not enough frames in the buffer, rebuffering!");
      m_stopCounter = 0;  // reset the stopCounter
      m_rebufferCounter++;
      m_bufferEvent = Simulator::Schedule (Seconds (1.0), &VideoStreamClient::ReadFromBuffer, this);
    }

    m_lastBufferSize = m_currentBufferSize;
    return (-1);
  }
  else
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " s: Play video frames from the buffer");
    if (m_stopCounter > 0) m_stopCounter = 0;    // reset the stopCounter
    if (m_rebufferCounter > 0) m_rebufferCounter = 0;   // reset the rebufferCounter
    m_currentBufferSize -= m_frameRate;

    m_bufferEvent = Simulator::Schedule (Seconds (1.0), &VideoStreamClient::ReadFromBuffer, this);
    m_lastBufferSize = m_currentBufferSize;
    return (m_currentBufferSize);
  }
}

void 
VideoStreamClient::HandleRead (Ptr<Socket> socket)
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
      uint32_t  recvSeq, recvLastSeq;
      if (m_isRTP)
      {
        RtpHeader hdr;
        packet->RemoveHeader (hdr);
        recvSeq = hdr.GetSquence ();
        recvLastSeq = hdr.GetLastFrameSquence ();
        // missing packet entered, then pop
        if (m_missingQueue.begin()->first == recvSeq)
        {
          NS_LOG_INFO("Requested retransmission seq=" << recvSeq <<" in");
          m_missingQueue.erase(recvSeq);
        }

        // if empty or never pushed, push it. (it only pushed on ascending order.)
        if (m_lastSeqQueue.empty () || m_lastSeqQueue.back () < recvLastSeq)
          m_lastSeqQueue.push(recvLastSeq);

        // if passed frame's seqence came in, do nothing.
        if (recvSeq >= m_minSeq)
          m_packetBuffer[recvSeq] = *packet;
        else
          return;

        if (m_maxSeq < recvSeq)
        {
          m_maxSeq = recvSeq;
        }

        uint32_t target = m_lastSeqQueue.front ();

        // if maximum recved seq > least final seq for frame
        if (target < m_maxSeq)
        {
          // check whether it is complete
          bool complete = true;
          for (uint32_t i = m_minSeq; i <= target; i ++)
          {
            // if missing seq found, mark incomplete, and push missing seq into missing queue
            if (m_packetBuffer.count (i) == 0)
            {
              complete = false;
              if (m_missingQueue.count (i) == 0)
                NS_LOG_INFO("Missing packet seq = " << i);
                m_missingQueue[i] = Simulator::Now ().GetMicroSeconds() - m_reReqDelay*2; // quick first retransmit, then delay
              break;
            }
          }
          // if it is complete, add up the frame.
          if (complete)
          {
            uint32_t frameNum;
            for (uint32_t i = m_minSeq; i <= target; i ++)
            {
              Packet p = m_packetBuffer.at (i);
              uint8_t recvData[p.GetSize ()];
              p.CopyData (recvData, p.GetSize ());
              sscanf ((char *) recvData, "%u", &frameNum);
              m_frameSize += p.GetSize ();
              m_packetBuffer.erase (i);
            }
            
            NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received frame " << frameNum << " and " << m_frameSize << " bytes from " <<  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
        
            m_currentBufferSize++;
            m_frameSize = 0;
            m_minSeq = target+1;
            m_lastSeqQueue.pop ();
          }
        }
      }
      else
      {
        uint8_t recvData[packet->GetSize ()];
        packet->CopyData (recvData, packet->GetSize ());
        uint32_t frameNum;
        sscanf ((char *) recvData, "%u", &frameNum);
    
        if (frameNum == m_lastRecvFrame)
        {
          m_frameSize += packet->GetSize ();
        }
        else
        {
          if (frameNum > 0)
          {
            NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received frame " << frameNum-1 << " and " << m_frameSize << " bytes from " <<  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
          }
          
          m_currentBufferSize++;
          m_lastRecvFrame = frameNum;
          m_frameSize = packet->GetSize ();
        }
      }

      // sending 10 means the client is still alive
      uint8_t dataBuffer[10];
      sprintf ((char*) dataBuffer, "%hu", (unsigned short int) 10);
      Ptr<Packet> alivePacket = Create<Packet> (dataBuffer, 10);

      // send retransmit request with alive signal
      if (m_isRTP)
      {
        RtpHeader hdr;
        uint32_t wantToRetrans = 0;
        if (!m_missingQueue.empty ())
        {
          
          std::map<uint32_t, int64_t> :: iterator iter = m_missingQueue.begin ();
          uint32_t RetransSeq = iter->first;
          NS_LOG_INFO ("Client missing some seq " << RetransSeq);

          if (Simulator::Now ().GetMicroSeconds () > iter->second + m_reReqDelay)
          {
            wantToRetrans = RetransSeq;
            hdr.SetSquence (wantToRetrans);

            m_missingQueue.erase(RetransSeq);
            m_missingQueue[RetransSeq] = Simulator::Now ().GetMicroSeconds ();
          }
        }

        alivePacket->AddHeader (hdr);
        if (wantToRetrans != 0)
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client requested to retransmit seq " << wantToRetrans << " to " <<  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
          
      }

      socket->SendTo (alivePacket, 0, from);

      // The rebuffering event has happend 3+ times, which suggest the client to lower the video quality.
      if (m_rebufferCounter >= 3)
      {
        if (m_videoLevel > 1)
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s: Lower the video quality level!");
          m_videoLevel--;
          // reflect the change to the server
          uint8_t dataBuffer[10];
          sprintf((char *) dataBuffer, "%hu", m_videoLevel);
          Ptr<Packet> levelPacket = Create<Packet> (dataBuffer, 10);

          if (m_isRTP)
          {
            RtpHeader hdr;
            levelPacket->AddHeader(hdr);
          }

          socket->SendTo (levelPacket, 0, from);
          m_rebufferCounter = 0;
        }
      }
      
      // If the current buffer size supports 5+ seconds video, we can try to increase the video quality level.
      if (m_currentBufferSize > 5 * m_frameRate)
      {
        if (m_videoLevel < MAX_VIDEO_LEVEL)
        {
          m_videoLevel++;
          // reflect the change to the server
          uint8_t dataBuffer[10];
          sprintf((char *) dataBuffer, "%hu", m_videoLevel);
          Ptr<Packet> levelPacket = Create<Packet> (dataBuffer, 10);

          if (m_isRTP)
          {
            RtpHeader hdr;
            levelPacket->AddHeader(hdr);
          }

          socket->SendTo (levelPacket, 0, from);
          m_currentBufferSize = m_frameRate;
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds() << "s: Increase the video quality level to " << m_videoLevel);
        }
      }
    }
  }
}

} // namespace ns3

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
#include "mos-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MosClientApplication");

NS_OBJECT_ENSURE_REGISTERED (MosClient);

TypeId
MosClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MosClient")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<MosClient> ()
    .AddAttribute ("RemoteAddress", "The destination address of the outbound packets",
                    AddressValue (),
                    MakeAddressAccessor (&MosClient::m_peerAddress),
                    MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                    UintegerValue (5000),
                    MakeUintegerAccessor (&MosClient::m_peerPort),
                    MakeUintegerChecker<uint16_t> ())
    
  ;
  return tid;
}

MosClient::MosClient ()
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
}

MosClient::~MosClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
MosClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void 
MosClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
MosClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
MosClient::StartApplication (void)
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

  m_socket->SetRecvCallback (MakeCallback (&MosClient::HandleRead, this));
  m_sendEvent = Simulator::Schedule (MilliSeconds (1.0), &MosClient::Send, this);
  // m_bufferEvent = Simulator::Schedule (Seconds (m_initialDelay), &MosClient::ReadFromBuffer, this);
}

void
MosClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
  {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    m_socket = 0;
  }

  Simulator::Cancel (m_bufferEvent);
}

void
MosClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  uint8_t dataBuffer[10];
  sprintf((char *) dataBuffer, "%hu", (unsigned short int)0);
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


void 
MosClient::HandleRead (Ptr<Socket> socket)
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
      uint8_t recvData[packet->GetSize()];
      packet->CopyData (recvData, packet->GetSize ());
      NS_LOG_INFO("At time " << Simulator::Now().GetSeconds () << "s client send message " << recvData);
    }
  }
}

} // namespace ns3

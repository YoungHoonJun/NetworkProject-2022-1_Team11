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
#include "ns3/address-utils.h"
#include "ns3/string.h"

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
	.AddAttribute ("MaxPacketSize", "The maximum size of a packet",
                    UintegerValue (1400),
                    MakeUintegerAccessor (&MosClient::m_maxPacketSize),
                    MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("TextFile", "The file that contains the plain text",
                    StringValue (""),
                    MakeStringAccessor (&MosClient::SetTextFile, &MosClient::GetTextFile),
                    MakeStringChecker ())
    
  ;
  return tid;
}

MosClient::MosClient ()
{
  NS_LOG_FUNCTION (this);
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
MosClient::SetMaxPacketSize (uint32_t maxPacketSize)
{
  m_maxPacketSize = maxPacketSize;
}

uint32_t
MosClient::GetMaxPacketSize (void) const
{
  return m_maxPacketSize;
}

// get all lines from the input file
void
MosClient::SetTextFile (std::string textFile)
{
  NS_LOG_FUNCTION (this << textFile);
  m_textFile = textFile;
  if (textFile != "")
  {
    std::string line;
    std::ifstream fileStream(textFile);
    while (std::getline (fileStream, line))
    {
      m_lineList.push_back (line);
    }
  }
}

std::string
MosClient::GetTextFile (void) const
{
  NS_LOG_FUNCTION (this);
  return m_textFile;
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
  // set timer and send number
  m_lastTime = -1;
  m_sendNum = 0;
  uint8_t dataBuffer[10];
  sprintf((char *) dataBuffer, "%hu", (unsigned short int)2);
  // 2: means start of the application
  Ptr<Packet> sys2Packet = Create<Packet> (dataBuffer, 10);
  m_socket->Send (sys2Packet);

  
  if (Ipv4Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client send initial packet to " <<
                  Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client send initial packet to " <<
                  Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client send initial packet to " <<
                  InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }
  else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent initial packet to " <<
                  Inet6SocketAddress::ConvertFrom (m_peerAddress).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }

  // activate when Server send some packet
  m_socket->SetRecvCallback (MakeCallback (&MosClient::HandleRead, this));
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

}

/*
  send txt file line
  if it send all txt, it would be terminated with sending 12
  else, it would send next line
*/

void
MosClient::Send ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  uint8_t dataBuffer[m_maxPacketSize];
  sprintf((char *) dataBuffer, "%hu", (unsigned short int)11);
  // sign for message sending is 11
  const char* c = m_lineList[m_sendNum].c_str();
  sprintf((char *) dataBuffer + 2, "%s", c);
  Ptr<Packet> textPacket = Create<Packet> (dataBuffer, m_maxPacketSize);
  m_socket->Send (textPacket);
  m_sendNum++;

  std::string tmp = c;

  if (Ipv4Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " client send server msg " << Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " client send server msg" << Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
  }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " client send server msg" <<
			InetSocketAddress::ConvertFrom (m_peerAddress) << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }
  else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " client send server msg" <<
			Inet6SocketAddress::ConvertFrom (m_peerAddress) << " port " << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
  }

  NS_LOG_INFO ("MSG: " << c);

  if (m_lineList.size() <= m_sendNum) //send all
  {
    	
    uint8_t dataBuffer[10];
    sprintf((char *) dataBuffer, "%hu", (unsigned short int)12);
    Ptr<Packet> textPacket = Create<Packet> (dataBuffer, 10);
    m_socket->Send (textPacket);

    if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent all txt line" << " to " << Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }
    else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client send all txt line" << " to " << Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }
    else if (InetSocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client send all txt line" <<
			  " to " << InetSocketAddress::ConvertFrom (m_peerAddress) << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
    }
    else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client send all txt line " <<
		  	  " to " << Inet6SocketAddress::ConvertFrom (m_peerAddress) << " port " << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
	}
  }
  else
  {
    m_sendEvent = Simulator::Schedule (MilliSeconds (1.0), &MosClient::Send, this);
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
	  uint16_t signal;
	  sscanf ((char*)recvData, "%hu", &signal);

	  if (signal == (unsigned short int) 2)
	  // if client get ack from the server, clinet send the txt file
	  {
	    m_sendEvent = Simulator::Schedule (MilliSeconds (1.0), &MosClient::Send, this);
	  }

	  if (signal == (unsigned short int) 11)
	  // if client send the mos data, it shows the data with the proper time interval
	  {
		if (m_lastTime == -1){
		  m_lastTime = Simulator::Now().GetSeconds();
		  NS_LOG_INFO("-----MOS START BY CLIENT SENDING MSG-----" << std::endl);
		}
		else{
		  double gap = Simulator::Now().GetSeconds() - m_lastTime;
		  m_lastTime = Simulator::Now().GetSeconds();
		  uint8_t tap = uint8_t(gap / 0.9);
		  for (uint8_t i = 0; i < tap; i++){
		    NS_LOG_INFO("");
		  }
		}

		Address addr;
		socket->GetPeerName (addr);
		InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (addr);
		char c = '\0';
		sscanf((char*)recvData + 2, "%c", &c);
		NS_LOG_INFO("AT TIME " << Simulator::Now().GetSeconds() <<" CLIENT " << iaddr.GetIpv4() << " Get " << c);	
	  }	
    }
  }
}

} // namespace ns3

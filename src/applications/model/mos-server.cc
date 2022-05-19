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
#include "ns3/string.h"
#define DIT 0.2

namespace ns3 {

/* mos translate code from https://numerok.tistory.com/71 */
class Morse {
	std::string alpahbet[26]; // 알파벳의 모스 부호 저장
	std::string digit[10]; // 숫자의 모스 부호 저장
	std::string slash, question, comma, period, plus, equal; // 특수 문자의 모스 부호 저장
public:
    Morse(); // alphabet[], digit[] 배열 및 특수 문자의 모스 부호 초기화
    void text2Morse(std::string text, std::string& morse); // 영문 텍스트를 모스 부호로 변환
};
Morse::Morse() {
    alpahbet[0] = ".-"; alpahbet[1] = "-..."; alpahbet[2] = "-.-."; alpahbet[3] = "-..";
    alpahbet[4] = "."; alpahbet[5] = "..-."; alpahbet[6] = "--."; alpahbet[7] = "....";
    alpahbet[8] = ".."; alpahbet[9] = ".---"; alpahbet[10] = "-.-"; alpahbet[11] = ".-..";
    alpahbet[12] = "--"; alpahbet[13] = "-."; alpahbet[14] = "---"; alpahbet[15] = ".--.";
    alpahbet[16] = "--.-"; alpahbet[17] = ".-."; alpahbet[18] = "..."; alpahbet[19] = "-";
    alpahbet[20] = "..-"; alpahbet[21] = "...-"; alpahbet[22] = ".--"; alpahbet[23] = "-..-";
    alpahbet[24] = "-.--"; alpahbet[25] = "--.."; digit[0] = "-----"; digit[1] = ".----";
    digit[2] = "..---"; digit[3] = "...--"; digit[4] = "....-"; digit[5] = ".....";
    digit[6] = "-...."; digit[7] = "--..."; digit[8] = "---.."; digit[9] = "----.";
    slash = "-..-."; question = "..--.."; comma = "--..--"; period = ".-.-.-";
    plus = ".-.-"; equal = "-...-";
}
void Morse::text2Morse(std::string text, std::string& morse) {
    for (int i = 0; i < text.size(); ++i) {
        char c = text.at(i);
        if (c >= 65 && c <= 90) c=tolower(c);
        if (c >= 97 && c <= 122) {
            morse = morse +alpahbet[c - 97]+" ";
        }
        else if (c >= 48 && c<=57) {
            morse = morse + digit[c - 48]+" ";
        }
        else {
            switch (c)
            {
            case '/':morse = morse + slash+" "; break;
            case '?':morse = morse + question + " "; break;
            case ',':morse = morse + comma + " "; break;
            case '.':morse = morse + period + " "; break;
            case '+':morse = morse + plus + " "; break;
            case '=':morse = morse + equal + " "; break;
            case ' ':morse += "  "; break;
            }
        }
    }
}


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

  ClientInfo *clientInfo = m_clients.at (ipAddress);

  NS_ASSERT (clientInfo->m_sendEvent.IsExpired ());
  
  // long line packet dividing function would be needed
  Morse m;
  uint8_t morseList[30000];
  uint16_t cnt = 0;
  for (auto str:clientInfo->m_textContainer)
  {
	std::string morse;
    m.text2Morse(str, morse);
	//NS_LOG_INFO("SERVER MORSE" << morse);
	for (char c : morse)
	{
	  morseList[cnt++] = c;
	}
	morseList[cnt] = '*'; //next line
  }
	
  Simulator::Schedule(Seconds(0.0), &MosServer::SendByTime, this, ipAddress, morseList, cnt);
  
  // send some outputs 

}


void 
MosServer::SendByTime (uint32_t ipAddress, uint8_t* morseList, uint16_t cnt)
{
  ClientInfo* client = m_clients[ipAddress];
  uint16_t sent = client->m_sent;
  if (cnt <= sent)
  {
	
	return;
  }

  uint8_t dataBuffer[20];
  sprintf ((char *) dataBuffer, "%u", (unsigned short int)11);
  char c = morseList[client->m_sent++];
  if (c == ' ')
  {
    Simulator::Schedule(Seconds(DIT * 3), &MosServer::SendByTime, this, ipAddress, morseList, cnt);
	return;
  }
  if (c == '*')
  {
    Simulator::Schedule(Seconds(DIT * 7), &MosServer::SendByTime, this, ipAddress, morseList, cnt);
	return;
  }
  
  sprintf ((char *) dataBuffer + 2, "%c", c);
  Ptr<Packet> p = Create<Packet> (dataBuffer, 20);
  m_socket->SendTo (p, 0, client->m_address);
  
  if (c == '.')
  {
    Simulator::Schedule(Seconds(DIT), &MosServer::SendByTime, this, ipAddress, morseList, cnt);
	return;
  }
  
  if (c == '-')
  {
    Simulator::Schedule(Seconds(DIT * 3), &MosServer::SendByTime, this, ipAddress, morseList, cnt);
	return;
  }
  
}

void
MosServer::WriteBuffer(uint8_t* textBuffer, uint32_t ipAddr){
	std::string str((const char*)textBuffer);
	//uint16_t len = strlen( (char*) textBuffer );
	//sscanf((char*) textBuffer, "%s", to + m_clients[ipAddr]->cnt);
	m_clients[ipAddr]->m_textContainer.push_back(str);
	//strcpy(to, (char*) textBuffer);
	
	//m_clients[ipAddr]->cnt = m_clients[ipAddr]->cnt + len;
	NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s SEVER HAS STR " << str);
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

      uint32_t ipAddr = InetSocketAddress::ConvertFrom (from).GetIpv4 ().Get ();

      // the first time we received the message from the client
      if (m_clients.find (ipAddr) == m_clients.end ())
      {
        ClientInfo *newClient = new ClientInfo();
        newClient->m_sent = 0;
        newClient->m_address = from;
        newClient->m_sendEvent = EventId ();
		newClient->m_recieveEvent = EventId();
        m_clients[ipAddr] = newClient;

		uint8_t dataBuffer[10];
		packet->CopyData (dataBuffer, 10);

		uint16_t signal = 0;
		sscanf((char*) dataBuffer, "%hu", &signal);
		
		if (signal == (unsigned short int)2) 
		{
		  // resend ack 2
		  Ptr<Packet> sys1Packet = Create<Packet> (dataBuffer, 10);
		  socket->SendTo (sys1Packet, 0, from);
		}
	  }

      
      else
      {
		uint8_t dataBuffer[10];
		packet->CopyData (dataBuffer, 10);
		uint16_t signal = 0;
		sscanf((char*) dataBuffer, "%hu", &signal);

		if (signal == (unsigned short int) 11)
		{
		  uint8_t textBuffer[1000];
		  packet->CopyData (textBuffer, 1000);
		  NS_LOG_INFO("INSIDE textBuffer" << textBuffer +2 ); 
		  MosServer::WriteBuffer(textBuffer + 2, ipAddr);
		}
		if (signal == (unsigned short int) 12)
		{
			Simulator::Schedule (Seconds(0.0), &MosServer::Send, this, ipAddr);
		}
      }
    }
  }
}

} // namespace ns3

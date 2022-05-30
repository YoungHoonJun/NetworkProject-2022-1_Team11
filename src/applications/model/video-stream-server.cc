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
#define MAXFRAMESIZE 10000000
//10MB

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
                    TimeValue (Seconds (0.15)),
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
	NS_LOG_INFO("IM ALIVE");
}

void
VideoStreamServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket != 0)
  {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    m_socket = 0;
  }

  for (auto iter = m_clients.begin (); iter != m_clients.end (); iter++)
  {
    Simulator::Cancel (iter->second->m_sendEvent);
  }
  
}


// save data and its size in m_frames
void VideoStreamServer::SetFrameData (std::string line, std::string path, std::string arg)
{
	std::string R = path + line + arg + ".png";
	std::cout << R <<std::endl;
	FILE *fp = fopen( R.c_str(), "rb");
	if (fp == NULL) puts("cannot open file");
	uint8_t* frame = new uint8_t[MAXFRAMESIZE]; // 10MB
	fseek(fp, 0, SEEK_END);
  uint32_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);  
	fread(frame, 1, size, fp);
	Frame* newFrame = new (Frame);
	newFrame->m_frameData = frame;
	newFrame->m_dataSize = size;
	m_frames[line + arg] = newFrame; //ex) swings3
	fclose(fp);
	NS_LOG_INFO("Size of Each frame :" << size);
}


// read the input file
void 
VideoStreamServer::SetFrameFile (std::string frameFile)
{
  NS_LOG_FUNCTION (this << frameFile);
  m_frameFile = frameFile;
  if (frameFile != "")
  {
    std::string line;
    std::ifstream fileStream (frameFile);
		std::string path = "scratch/videoStreamer/images/";
    while (std::getline (fileStream, line))
    {	
			std::stringstream ss(line);
			std::string trimmed_line;
			ss >> trimmed_line;

			m_frameNameList.push_back(trimmed_line);
			if (m_frames.find(trimmed_line + std::to_string(1)) == m_frames.end()) //do not have frame data in program
			{
				SetFrameData(trimmed_line, path, std::to_string(1));
				SetFrameData(trimmed_line, path, std::to_string(2));
				SetFrameData(trimmed_line, path, std::to_string(3));
				SetFrameData(trimmed_line, path, std::to_string(4));
				SetFrameData(trimmed_line, path, std::to_string(5));
				SetFrameData(trimmed_line, path, std::to_string(6));
			}
    }
  }
  NS_LOG_INFO ("Frame list size: " << m_frameNameList.size());
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
	std::string frameName;
  ClientInfo *clientInfo = m_clients.at (ipAddress);

  if (m_lastTime.find (ipAddress) != m_lastTime.end ()){ //that ip has time log - no response -> don't send
	  double lastTime = m_lastTime.at (ipAddress);
	  if (Simulator::Now ().GetSeconds () - lastTime > 3.0){
		  Simulator::Cancel (clientInfo->m_sendEvent);
      return;
	  }
  }


  NS_ASSERT (clientInfo->m_sendEvent.IsExpired ());
  // If the frame sizes are not from the text file, and the list is empty
  if (m_frameNameList.empty ())
  {
    frameSize = m_frameSizes[clientInfo->m_videoLevel];
    totalFrames = m_videoLength * m_frameRate;
  }
  else
  {
    frameName = m_frameNameList.at(clientInfo->m_sent) + std::to_string(clientInfo->m_videoLevel);
    totalFrames = m_frameNameList.size ();
		frameSize = m_frames[frameName]->m_dataSize;
  }

  // the frame might require several packets to send

  // if the client requires RTP, last info of last seq for the frame is needed.
  if (clientInfo->m_isRTP)
  {
    clientInfo->m_FrameLastSeq += (frameSize / m_maxPacketSize) + 1;
  }
	NS_LOG_INFO ("Server sending m_sent " << clientInfo->m_sent << " frame");
	uint32_t curFrameVideoRate = clientInfo->m_videoLevel;	//should not change current Frame's videorate
  for (uint i = 0; i <= frameSize / m_maxPacketSize; i++)
  {
    SendPacket (clientInfo, m_maxPacketSize, i, curFrameVideoRate);
  }

  clientInfo->m_sent += 1;
  if (clientInfo->m_sent < totalFrames)
  {
    clientInfo->m_sendEvent = Simulator::Schedule (m_interval, &VideoStreamServer::Send, this, ipAddress);
  }
}

// Send the packet divided by MaxPacket Size
void 
VideoStreamServer::SendPacket (ClientInfo *client, uint32_t packetSize, uint32_t i, uint32_t curFrameVideoRate)
{
	std::string frameName = m_frameNameList.at(client->m_sent) + std::to_string(curFrameVideoRate);
	auto t = m_frames.find(frameName);
	if (t == m_frames.end()) printf("wrong in getting the data\n\n\n\"");
	Frame* targetFrame = t->second;
	uint32_t frameSize = targetFrame->m_dataSize;
	uint32_t sendSize = packetSize;
	if ( (i + 1) * packetSize > frameSize) 
	{
		sendSize = frameSize % packetSize;
	}
	uint8_t* dataBuffer = targetFrame->m_frameData;
	Ptr<Packet> p = Create<Packet> (dataBuffer + (i * packetSize), sendSize);
		
  if (client->m_isRTP)
  {
    client->m_lastSeq += 1;
    RtpHeader hdr;
    hdr.SetSquence (client->m_lastSeq);
    hdr.SetLastFrameSquence (client->m_FrameLastSeq);
    p->AddHeader (hdr);
    client->m_queue->push_back (p->Copy());
    while (client->m_queue->size () > m_maxRtpQueueLen)
    {
      client->m_queue->pop_front ();
    }
		NS_LOG_INFO("server sends packet:  SeqNum: " << client->m_lastSeq );
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

	NS_LOG_INFO("SIGNAL CAME");
  while ((packet = socket->RecvFrom (from)))
  {
    socket->GetSockName (localAddress);
    if (InetSocketAddress::IsMatchingType (from))
    {
      uint32_t ipAddr = InetSocketAddress::ConvertFrom (from).GetIpv4 ().Get ();

      // the first time we received the message from the client
      if (m_clients.find (ipAddr) == m_clients.end ())
      {
        ClientInfo *newClient = new ClientInfo ();
        newClient->m_sent = 0;
        newClient->m_videoLevel = 3;
        newClient->m_address = from;
        newClient->m_sendEvent = EventId ();

        // read first got packet to determine it is RTP client or not.
        uint8_t dataBuffer[10];
        packet->CopyData (dataBuffer, 10);
        uint16_t det;
        sscanf ((char *) dataBuffer, "%hu", &det);
        if (det == 2)
          newClient->m_isRTP = false;
        else if (det == 1)
        {
          newClient->m_isRTP = true;
          m_maxRtpQueueLen = 1000000;
          newClient->m_queue = new std::list<Ptr<Packet>>;
        }
        
        m_clients[ipAddr] = newClient;
        newClient->m_sendEvent = Simulator::Schedule (Seconds (0.0), &VideoStreamServer::Send, this, ipAddr);
        newClient->m_lastSeq = 0;
		
		    m_lastTime[ipAddr] = Simulator::Now ().GetSeconds ();
      }
      else
      {
        ClientInfo *currentClient = m_clients.at (ipAddr);
		    m_lastTime[ipAddr] = Simulator::Now ().GetSeconds ();

        uint32_t lostSeq = 0;
        if (currentClient->m_isRTP)
        {
          RtpHeader hdr;
          packet->RemoveHeader (hdr);
          lostSeq = hdr.GetSquence ();
          // if lostSeq, give the Seq to client
					if(lostSeq > 0)
          {
            
            NS_LOG_INFO ("SERVER CHECKED Client " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " lost seq " << lostSeq );
            RtpHeader pivHdr;
            std::list<Ptr<Packet>>::iterator iter = currentClient->m_queue->begin ();
            (*iter)->PeekHeader (pivHdr);
            uint32_t pivSeq = pivHdr.GetSquence ();

            while (pivSeq < lostSeq)
            {
              iter ++;
							(*iter)->PeekHeader (pivHdr);
              pivSeq = pivHdr.GetSquence ();
            }

            Ptr<Packet> retransPacket = Ptr<Packet>(*iter);

            NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server retransmit lost sequence " << pivSeq << " to " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
            if (m_socket->SendTo (retransPacket, 0, currentClient->m_address) < 0)
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
          NS_LOG_INFO("Stop signal from client " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
          Simulator::Cancel (currentClient->m_sendEvent);		//stop sending
          m_clients.erase (ipAddr);
          m_lastTime.erase (ipAddr);
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

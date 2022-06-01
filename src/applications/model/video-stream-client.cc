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
#include "ns3/output-stream-wrapper.h"
#include <sys/stat.h>
#define MAXFRAMESIZE 10000000

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
                    BooleanValue(true),
                    MakeBooleanAccessor (&VideoStreamClient::m_isRTP),
                    MakeBooleanChecker ())
  ;
  return tid;
}

VideoStreamClient::VideoStreamClient ()
{
  NS_LOG_FUNCTION (this);
  m_initialDelay = 2;
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
	m_sendMissingSignalEvent = EventId();
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
	m_frameSec = 0;	
	NS_LOG_INFO("START____________!!!");
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
	std::string filePath = "./scratch/videoStreamer/videos";
	mkdir(filePath.c_str(), 0666);
	// Set handle read, signalEvent, buffer Event. (Send Event is only called once)
	m_sendMissingSignalEvent = Simulator::Schedule(MicroSeconds(m_reReqDelay * 10), &VideoStreamClient::SendMissingSignal, this);	
  m_socket->SetRecvCallback (MakeCallback (&VideoStreamClient::HandleRead, this));
  m_sendEvent = Simulator::Schedule (MilliSeconds (1), &VideoStreamClient::Send, this);
  m_bufferEvent = Simulator::Schedule (Seconds (m_initialDelay), &VideoStreamClient::ReadFromBuffer, this);
}

//managing Events for stopping the app
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

    RtpHeader hdr;
    lastPacket->AddHeader(hdr);
    m_socket->Send (lastPacket);
    NS_LOG_INFO("Stop signal sent from client");
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    m_socket = 0;
  }
	Simulator::Cancel (m_sendMissingSignalEvent);
  Simulator::Cancel (m_bufferEvent);
}

//only called when the client starts
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

//sent per m_reReqDelay * 10
void
VideoStreamClient::SendMissingSignal(void)
{
	NS_LOG_FUNCTION (this);
	NS_ASSERT (m_sendMissingSignalEvent.IsExpired ());

	if (m_missingQueue.empty())
	{	
		m_sendMissingSignalEvent = Simulator::Schedule(MicroSeconds(m_reReqDelay * 10), &VideoStreamClient::SendMissingSignal, this);	
		NS_LOG_INFO("QUEUE is EMPTY NOTHING HAPPENS");
		return;
	}

  uint8_t dataBuffer[10];
  sprintf ((char*) dataBuffer, "%hu", (unsigned short int) 10);
  Ptr<Packet> alivePacket = Create<Packet> (dataBuffer, 10);
	//10 -> alive signal

	RtpHeader hdr;
	uint32_t wantToRetrans = 0;

  std::map<uint32_t, int64_t> :: iterator iter = m_missingQueue.begin ();
	uint32_t RetransSeq = iter->first;

  wantToRetrans = RetransSeq;
  hdr.SetSquence (wantToRetrans);

	alivePacket->AddHeader(hdr);
	m_socket->Send(alivePacket);
	NS_LOG_INFO(Simulator::Now().GetSeconds() << "Client send regular missing packet signal");

	m_sendMissingSignalEvent = Simulator::Schedule(MicroSeconds(m_reReqDelay * 5), &VideoStreamClient::SendMissingSignal, this);	

	//ask missing packet signal
}

uint32_t 
VideoStreamClient::ReadFromBuffer (void)
{
  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " s, last buffer size: " << m_lastBufferSize << ", current buffer size: " << m_currentBufferSize.load() << "reading from buffer");
	NS_ASSERT(m_bufferEvent.IsExpired());
  if (m_currentBufferSize.load() < m_frameRate) 
  {
    if (m_lastBufferSize == m_currentBufferSize.load())
    {
      m_stopCounter++;
      // If the counter reaches 3, which means the client has been waiting for 3 sec, and no packets arrived.
      // In this case, we use TCP, so m_stopCounter is not used.
      //if (m_stopCounter < 3)
      m_bufferEvent = Simulator::Schedule (Seconds (1.0), &VideoStreamClient::ReadFromBuffer, this);
    }
    else
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " s: Not enough frames in the buffer, rebuffering!");
      m_stopCounter = 0;  // reset the stopCounter
      m_rebufferCounter++;	// Buffering counter for manipultaing video level
      m_bufferEvent = Simulator::Schedule (Seconds (1.0), &VideoStreamClient::ReadFromBuffer, this); // read again after 1sec
    }

    m_lastBufferSize = m_currentBufferSize.load();
    return (-1);
  }
  else //if frameRate is lower than currentBufferSize -> can play the video for 1sec
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " s: Play video frames from the buffer");
    if (m_stopCounter > 0) m_stopCounter = 0;    // reset the stopCounter
    if (m_rebufferCounter > 0) m_rebufferCounter = 0;   // reset the rebufferCounter
    m_currentBufferSize.fetch_sub(m_frameRate); // m_curBufferSize -= m_frameRate
			
		// saving 1 sec video. which is 25(frameRate) frame.
		for (uint8_t frameSeq = 0; frameSeq < m_frameRate; frameSeq++)
		{
			uint8_t* targetFrame = m_frameBuffer.front();
			uint32_t targetFrameSize = m_frameBufferSize.front();
		
			NS_LOG_INFO ("Frame playing.. frameSeq:" << frameSeq << " targetFrameSize" << targetFrameSize);
			std::string filePath = "./scratch/videoStreamer/videos/";
			std::string curPath = filePath + std::to_string(m_frameSec)+ "." + std::to_string(frameSeq) + ".png";
			
			FILE* fp;
			fp = fopen(curPath.c_str(), "w");
			if (fp==NULL) printf("Cannot open the file");
			fwrite(targetFrame, targetFrameSize, 1, fp);
			
			fclose(fp);
			

			m_frameBuffer.pop();
			m_frameBufferSize.pop();
			
			delete[] targetFrame;
		}
		m_frameSec++;
	
    
		m_bufferEvent = Simulator::Schedule (Seconds (1.0), &VideoStreamClient::ReadFromBuffer, this);
    m_lastBufferSize = m_currentBufferSize.load();
    return (m_currentBufferSize.load());
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
      RtpHeader hdr;
      packet->RemoveHeader (hdr);
      recvSeq = hdr.GetSquence ();
      recvLastSeq = hdr.GetLastFrameSquence ();

      // missing packet entered, then pop.
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
			{	
        return;
			}
      if (m_maxSeq < recvSeq)
      {
        m_maxSeq = recvSeq;
      }

			// target is the final frame Seq that program want to wrap up
      uint32_t target = m_lastSeqQueue.front ();
			NS_LOG_INFO("CLIENTs recieved pakcet. target: " << target << "  m_maxSeq:" << m_maxSeq << " recved seq:" << recvSeq);
				
      // if maximum recved seq > least final seq for frame
      if (target <= m_maxSeq)
      {
        // check whether it is complete
        bool complete = true;
        for (uint32_t i = m_minSeq; i <= target; i ++)
        {
          // if missing seq found, mark incomplete, and push missing seq into missing queue
          if (m_packetBuffer.count (i) == 0)
          {
						NS_LOG_INFO ("MISSING PACKET SEQ FOUND = " << i);
            complete = false;
            if (m_missingQueue.count (i) == 0)
						{
							m_missingQueue[i] = Simulator::Now ().GetMicroSeconds() - m_reReqDelay*2; // quick first retransmit, then delay
						}
            break;
          }
        }

        // if it is complete, add up the frame.
				// And, we should also check the next missing packet
        if (complete)
        {
					// checking the next frame's missing packet
					for (uint32_t i = target + 1; i <= m_maxSeq; i++)
					{
						if (m_packetBuffer.count(i) == 0)
						{
							NS_LOG_INFO ("COMPLETED current target but found next missing seq = " << i);
								m_missingQueue[i] = Simulator::Now ().GetMicroSeconds() - m_reReqDelay*2;
								break;
						}
					}

					// adding up the frame
					uint8_t* frame = new uint8_t[MAXFRAMESIZE];
					uint32_t size = 0;
          for (uint32_t i = m_minSeq; i <= target; i ++)
          {
            Packet p = m_packetBuffer.at (i);
            p.CopyData (frame + size, p.GetSize ());
            size += p.GetSize ();
            m_packetBuffer.erase (i);

          }
					NS_LOG_INFO ("COMPLETED OneFrame!"); 
						
					m_frameBuffer.push(frame);
					m_frameBufferSize.push(size);
					
          m_currentBufferSize++;
          m_frameSize = 0;
          m_minSeq = target+1;
          m_lastSeqQueue.pop ();
        }
      }
			// below, in case of server dropping all later contents 
			
			/*
			if (target > m_maxSeq)
			{
				if (m_missingQueue.empty())
				{
					for (uint32_t i = m_maxSeq; i <= target; i ++)
					{
						if (m_packetBuffer.count (i) == 0)
						{
							NS_LOG_INFO ("MISSING PACKET SEQ FOUND = " << i);
							m_missingQueue[i] = Simulator::Now ().GetMicroSeconds() - m_reReqDelay*2; // quick first retransmit, then delay
							break;
						}
          }
        }
			}
			*/

      // sending 10 means the client is still alive
      uint8_t dataBuffer[10];
      sprintf ((char*) dataBuffer, "%hu", (unsigned short int) 10);
      Ptr<Packet> alivePacket = Create<Packet> (dataBuffer, 10);

      // send retransmit request with alive signal
			RtpHeader hdr_client;
			uint32_t wantToRetrans = 0;
			if (!m_missingQueue.empty ())
			{
				NS_LOG_INFO("MISSING QUEUE SIZE: " << m_missingQueue.size());
				std::map<uint32_t, int64_t> :: iterator iter = m_missingQueue.begin ();
				uint32_t RetransSeq = iter->first;
				NS_LOG_INFO ("Client missing some seq " << RetransSeq);

				if (Simulator::Now ().GetMicroSeconds () > iter->second + m_reReqDelay)
				{
					wantToRetrans = RetransSeq;
					hdr_client.SetSquence (wantToRetrans);

					m_missingQueue.erase(RetransSeq);
					m_missingQueue[RetransSeq] = Simulator::Now ().GetMicroSeconds ();
				}
			}

			alivePacket->AddHeader (hdr_client);
			if (wantToRetrans != 0)
				NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client requested to retransmit seq " << wantToRetrans << " to " <<  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());    
      

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
      if (m_currentBufferSize.load() > 5 * m_frameRate)
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
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds() << "s: Increase the video quality level to " << m_videoLevel);
        }
      }
    }
  }
}

} // namespace ns3

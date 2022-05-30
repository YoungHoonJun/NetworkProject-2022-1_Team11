/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef VIDEO_STREAM_CLIENT_H
#define VIDEO_STREAM_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"

#include <map>
#include <queue>
#include <atomic>

#define MAX_VIDEO_LEVEL 6

namespace ns3 {

class Socket;
class Packet;

/**
 * @brief A Video Stream Client
 */
class VideoStreamClient : public Application
{
public:
/**
 * @brief Get the type ID.
 * 
 * @return the object TypeId
 */
  static TypeId GetTypeId (void);
  VideoStreamClient ();
  virtual ~VideoStreamClient ();

  /**
   * @brief Set the server address and port.
   * 
   * @param ip server IP address
   * @param port server port
   */
  void SetRemote (Address ip, uint16_t port);
  /**
   * @brief Set the server address.
   * 
   * @param addr server address
   */
  void SetRemote (Address addr);

protected:
  virtual void DoDispose (void);

private: 
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  /**
   * @brief Send the packet to the remote server.
   */
  void Send (void);
	/**
	 * @regular missing packet send to Server
	 */
	void SendMissingSignal(void);
  /**
   * @brief Read data from the frame buffer. If the buffer does not have 
   * enough frames, it will reschedule the reading event next second.
   * 
   * @return the updated buffer size (-1 if the buffer size is smaller than the fps)
   */
  uint32_t ReadFromBuffer (void);

  /**
   * @brief Handle a packet reception.
   * 
   * This function is called by lower layers.
   * 
   * @param socket the socket the packet was received to
   */
  void HandleRead (Ptr<Socket> socket);

  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port

  uint16_t m_initialDelay; //!< Seconds to wait before displaying the content
  uint16_t m_stopCounter; //!< Counter to decide if the video streaming finishes
  uint16_t m_rebufferCounter; //!< Counter of the rebuffering event
  uint16_t m_videoLevel; //!< The quality of the video from the server
  uint32_t m_frameRate; //!< Number of frames per second to be played
  uint32_t m_frameSize; //!< Total size of packets from one frame
  uint32_t m_lastRecvFrame; //!< Last received frame number
  uint32_t m_lastBufferSize; //!< Last size of the buffer
	std::atomic <uint32_t> m_currentBufferSize; //!< Size of the frame buffer

  EventId m_bufferEvent; //!< Event to read from the buffer
  EventId m_sendEvent; //!< Event to send data to the server
	EventId m_sendMissingSignalEvent; //!< Event to regualrly sending the missing packet Seq
  // below are for RTP implementation
  int64_t m_reReqDelay; // !< re-Request delay for retransmition (us)
  uint32_t m_minSeq; //!< min seq on packetBuffer
  uint32_t m_maxSeq; //!< max seq on packetBuffer
  std::queue<uint32_t> m_lastSeqQueue; //!< last seq num of each frame
  std::map<uint32_t, int64_t> m_missingQueue; //!< Queue (but the length is 1) for reasking the missing packet
  std::map<uint32_t, Packet> m_packetBuffer; //!< Packet buffer for RTP (key = seq num.)

	std::queue<uint8_t*> m_frameBuffer; //!< frame buffer. each frame is up to 10MB
	std::queue<uint32_t> m_frameBufferSize; //!< Size of each frame

	uint32_t m_frameSec; //!< indicating when the frame appears in video
  bool m_isRTP; //!< True if Client is using Real-time protocol

};

} // namespace ns3

#endif /* VIDEO_STREAM_CLIENT_H */

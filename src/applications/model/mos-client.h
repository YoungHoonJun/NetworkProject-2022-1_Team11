/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MOS_CLIENT_H
#define MOS_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include <fstream>



namespace ns3 {

class Socket;
class Packet;

/**
 * @brief A Video Stream Client
 */
class MosClient : public Application
{
public:
/**
 * @brief Get the type ID.
 * 
 * @return the object TypeId
 */
  static TypeId GetTypeId (void);
  MosClient ();
  virtual ~MosClient ();

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
  void SetMaxPacketSize (uint32_t maxPacketSize);
  uint32_t GetMaxPacketSize (void) const;
  void SetTextFile(std::string textFile);
  std::string GetTextFile(void) const;
  


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
	
  uint16_t m_sendNum; //!< how many lines you send
  std::string m_textFile;
  std::vector<std::string> m_lineList; //!< array of lines from the text file
  uint32_t m_maxPacketSize; //!< Maximum size of the packet to be sent
  EventId m_bufferEvent; //!< Event to read from the buffer
  EventId m_sendEvent; //!< Event to send data to the server
};

} // namespace ns3

#endif /* MOS_CLIENT_H */

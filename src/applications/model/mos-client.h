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

class MosClient : public Application
{
public:
  static TypeId GetTypeId (void);
  MosClient ();
  virtual ~MosClient ();

  // set remote ip, port
  void SetRemote (Address ip, uint16_t port);
  void SetRemote (Address addr);

protected:
  virtual void DoDispose (void);

private: 
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  
  // send all txt contents that client has
  void Send (void);

  void SetMaxPacketSize (uint32_t maxPacketSize);
  uint32_t GetMaxPacketSize (void) const;

  // get ready to use txt file
  void SetTextFile(std::string textFile);
  std::string GetTextFile(void) const;
  


  // handle how to process signal
  void HandleRead (Ptr<Socket> socket);

  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  
  double m_lastTime; //!< Timer for mos-language
  uint16_t m_sendNum; //!< how many lines you send
  std::string m_textFile; //!< file location of txt
  std::vector<std::string> m_lineList; //!< array of lines from the text file
  uint32_t m_maxPacketSize; //!< Maximum size of the packet to be sent
  EventId m_sendEvent; //!< Event to send data to the server
};

} // namespace ns3

#endif /* MOS_CLIENT_H */

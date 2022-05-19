/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MOS_SERVER_H
#define MOS_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"

#include <fstream>
#include <unordered_map>
namespace ns3 {

class Socket;
class Packet;

  /**
   * @brief A Video Stream Server
   */
  class MosServer : public Application
  {
  public:
    /**
     * @brief Get the type ID.
     * 
     * @return the object TypeId
     */
    static TypeId GetTypeId (void);

    MosServer ();

    virtual ~MosServer ();

    /**
     * @brief Set the maximum packet size.
     * 
     * @param maxPacketSize the largest number of bytes a packet can be
     */
    void SetMaxPacketSize (uint32_t maxPacketSize);

    /**
     * @brief Get the maximum packet size.
     * 
     * @return uint32_t the largest number of bytes a packet can be
     */
    uint32_t GetMaxPacketSize (void) const;

  protected:
    virtual void DoDispose (void);

  private:

    virtual void StartApplication (void);
    virtual void StopApplication (void);

    /**
     * @brief The information required for each client.
     */
    typedef struct ClientInfo
    {
      Address m_address; //!< Address
      uint32_t m_sent; //!< Counter for sent frames
      EventId m_sendEvent; //! Send event used by the client
	  EventId m_recieveEvent;
      std::vector <std::string> m_textContainer;
    } ClientInfo; //! To be compatible with C language

    
    void SendByTime (uint32_t ipAddress, uint8_t* morseList, uint16_t cnt);
    
    /**
     * @brief Send the video frame to the given ipv4 address.
     * 
     * @param ipAddress ipv4 address
     */
    void Send (uint32_t ipAddress);

	void WriteBuffer(uint8_t* textBuffer, uint32_t ipAddr);
    void HandleRead (Ptr<Socket> socket);

    Time m_interval; //!< Packet inter-send time
    uint32_t m_maxPacketSize; //!< Maximum size of the packet to be sent
    Ptr<Socket> m_socket; //!< Socket

    uint16_t m_port; //!< The port 
    Address m_local; //!< Local multicast address

    std::vector<uint32_t> m_frameSizeList; //!< List of video frame sizes

    std::unordered_map<uint32_t, ClientInfo*> m_clients; //!< Information saved for each client
  };

} // namespace ns3


#endif /* MOS_SERVER_H */

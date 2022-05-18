/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef RTP_HEADER_H
#define RTP_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/tcp-option.h"
#include "ns3/buffer.h"
#include "ns3/sequence-number.h"

namespace ns3 {

/**
 * @brief A RTP(Real-time protocol)-like streaming Header
 * 
 * This class is simple RTP-like header
 * only checking sequence of sent/received each packet
 * 
 */
class RtpHeader : public Header
{
public:
  RtpHeader ();
  virtual ~RtpHeader();

  /**
   * @brief Get the type ID.
   * 
   * @return the object TypeId
   */
  static ns3::TypeId GetTypeId (void);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize(Buffer::Iterator start);

  /**
   * \brief Set the sequence Number
   * \param sequence the sequence number for this RtpHeader
   */
  void SetSquence (uint32_t sequence);
  /**
   * \brief Get the sequence number
   * \return the sequence number for this RtpHeader
   */
  uint32_t GetSquence (void) const;

private:
  uint32_t m_sequence; //!< Sequence number
};

} // namespace ns3

#endif /* RTP_HEADER */
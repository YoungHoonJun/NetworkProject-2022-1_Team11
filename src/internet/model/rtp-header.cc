/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <stdint.h>
#include <iostream>
#include "rtp-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RtpHeader);

RtpHeader::RtpHeader ()
  : m_sequence (0) // default value is 0 : no retransmition required.
{
}

RtpHeader::~RtpHeader ()
{
}

TypeId
RtpHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RtpHeader")
    .SetParent<Header> ()
    .AddConstructor<RtpHeader> ()
    ;
  return tid;
}

TypeId
RtpHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
RtpHeader::Print (std::ostream &os) const
{
  os << " Seq=" << m_sequence << "\n";
}

uint32_t
RtpHeader::GetSerializedSize (void) const
{
  return 4;
}

void
RtpHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_sequence);
}

uint32_t
RtpHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_sequence = i.ReadNtohU32 ();
  return i.GetDistanceFrom (start);
}

void
RtpHeader::SetSquence (uint32_t sequence)
{
  m_sequence = sequence;
}

uint32_t
RtpHeader::GetSquence (void) const
{
  return m_sequence;
}

} // namespace ns3
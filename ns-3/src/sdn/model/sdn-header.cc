/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Haoliang Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Haoliang Chen  <chl41993@gmail.com>
 */

#include <cmath>

#include "ns3/assert.h"
#include "ns3/log.h"

#include "sdn-header.h"

#define IPV4_ADDRESS_SIZE 4
#define SDN_PKT_HEADER_SIZE 8
#define SDN_MSG_HEADER_SIZE 8
#define SDN_HELLO_HEADER_SIZE 28
#define SDN_RM_HEADER_SIZE 16
#define SDN_RM_TUPLE_SIZE 3
#define SDN_APPOINTMENT_HEADER_SIZE 8
#define SDN_AODVRM_HEADER_SIZE 24

NS_LOG_COMPONENT_DEFINE ("SdnHeader");

namespace ns3 {
namespace sdn {

float
rIEEE754 (uint32_t emf)
{
  union{
    float f;
    uint32_t b;
  } u;
  u.b = emf;
  return (u.f);
}

uint32_t
IEEE754 (float dec)
{
  union{
    float f;
    uint32_t b;
  } u;
  u.f = dec;
  return (u.b);
}

// ---------------- SDN Packet -------------------------------
NS_OBJECT_ENSURE_REGISTERED (PacketHeader);



PacketHeader::PacketHeader () :
    m_packetLength (0),
    m_packetSequenceNumber (0)
{
}

PacketHeader::~PacketHeader ()
{
}

TypeId
PacketHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::sdn::PacketHeader")
    .SetParent<Header> ()
    .AddConstructor<PacketHeader> ()
  ;
  return (tid);
}
TypeId
PacketHeader::GetInstanceTypeId (void) const
{
  return (GetTypeId ());
}

uint32_t 
PacketHeader::GetSerializedSize (void) const
{
  return (SDN_PKT_HEADER_SIZE);
}

void 
PacketHeader::Print (std::ostream &os) const
{
  /// \todo
}

void
PacketHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (this->originator.Get());
  i.WriteHtonU16 (m_packetLength);
  i.WriteHtonU16 (m_packetSequenceNumber);
}

uint32_t
PacketHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t add_temp = i.ReadNtohU32();
  this->originator.Set(add_temp);
  m_packetLength  = i.ReadNtohU16 ();
  m_packetSequenceNumber = i.ReadNtohU16 ();
  return (GetSerializedSize ());
}

// ---------------- SDN Message -------------------------------

NS_OBJECT_ENSURE_REGISTERED (MessageHeader);

MessageHeader::MessageHeader ()
  : m_messageType (MessageHeader::MessageType (0)),
    m_vTime (0),
    m_timeToLive (0),
    m_messageSequenceNumber (0),
    m_messageSize (0)
{
}

MessageHeader::~MessageHeader ()
{
}

TypeId
MessageHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::sdn::MessageHeader")
    .SetParent<Header> ()
    .AddConstructor<MessageHeader> ()
  ;
  return (tid);
}
TypeId
MessageHeader::GetInstanceTypeId (void) const
{
  return (GetTypeId ());
}

uint32_t
MessageHeader::GetSerializedSize (void) const
{
  uint32_t size = SDN_MSG_HEADER_SIZE;
  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      NS_LOG_DEBUG ("Hello Message Size: " << size << " + " 
            << m_message.hello.GetSerializedSize ());
      size += m_message.hello.GetSerializedSize ();
      break;
    case ROUTING_MESSAGE:
      size += m_message.rm.GetSerializedSize ();
      break;
    case APPOINTMENT_MESSAGE:
      size += m_message.appointment.GetSerializedSize ();
      break;
    case AODV_ROUTING_MESSAGE:
      size +=m_message.aodvrm.GetSerializedSize();
      break;
    case AODV_REVERSE_MESSAGE:
      size +=m_message.aodv_r_rm.GetSerializedSize();
      break;
    default:
      NS_ASSERT (false);
    }
  return (size);
}

void 
MessageHeader::Print (std::ostream &os) const
{
  /// \todo
}

void
MessageHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_messageType);
  i.WriteU8 (m_vTime);
  i.WriteHtonU16 (GetSerializedSize ());
  i.WriteHtonU16 (m_timeToLive);
  i.WriteHtonU16 (m_messageSequenceNumber);

  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      m_message.hello.Serialize (i);
      break;
    case ROUTING_MESSAGE:
      m_message.rm.Serialize (i);
      break;
    case APPOINTMENT_MESSAGE:
      m_message.appointment.Serialize (i);
      break;
    case AODV_ROUTING_MESSAGE:
      m_message.aodvrm.Serialize(i);
    break;
    case AODV_REVERSE_MESSAGE:
      m_message.aodv_r_rm.Serialize(i);
      break;
    default:
      NS_ASSERT (false);
    }

}

uint32_t
MessageHeader::Deserialize (Buffer::Iterator start)
{
  uint32_t size;
  Buffer::Iterator i = start;
  m_messageType  = (MessageType) i.ReadU8 ();
  NS_ASSERT (m_messageType >= HELLO_MESSAGE && m_messageType <= APPOINTMENT_MESSAGE);
  m_vTime  = i.ReadU8 ();
  m_messageSize  = i.ReadNtohU16 ();
  m_timeToLive  = i.ReadNtohU16 ();
  m_messageSequenceNumber = i.ReadNtohU16 ();
  size = SDN_MSG_HEADER_SIZE;
  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      size += 
        m_message.hello.Deserialize (i, m_messageSize - SDN_MSG_HEADER_SIZE);
      break;
    case ROUTING_MESSAGE:
      size += 
        m_message.rm.Deserialize (i, m_messageSize - SDN_MSG_HEADER_SIZE);
      break;
    case APPOINTMENT_MESSAGE:
      size +=
        m_message.appointment.Deserialize (i, m_messageSize - SDN_MSG_HEADER_SIZE);
      break;
    case AODV_ROUTING_MESSAGE:
      size +=
        m_message.aodvrm.Deserialize (i, m_messageSize - SDN_MSG_HEADER_SIZE);
      break;
    case AODV_REVERSE_MESSAGE:
        size +=
          m_message.aodv_r_rm.Deserialize (i, m_messageSize - SDN_MSG_HEADER_SIZE);
        break;
    default:
      NS_ASSERT (false);
    }
  return (size);
}


// ---------------- SDN HELLO Message -------------------------------

uint32_t 
MessageHeader::Hello::GetSerializedSize (void) const
{
  return (SDN_HELLO_HEADER_SIZE);
}

void 
MessageHeader::Hello::Print (std::ostream &os) const
{
  /// \todo
}

void
MessageHeader::Hello::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU32 (this->ID.Get());
  i.WriteHtonU32 (this->position.X);
  i.WriteHtonU32 (this->position.Y);
  i.WriteHtonU32 (this->position.Z);
  i.WriteHtonU32 (this->velocity.X);
  i.WriteHtonU32 (this->velocity.Y);
  i.WriteHtonU32 (this->velocity.Z);

}

uint32_t
MessageHeader::Hello::Deserialize (Buffer::Iterator start, 
  uint32_t messageSize)
{
  Buffer::Iterator i = start;

  NS_ASSERT (messageSize == SDN_HELLO_HEADER_SIZE);

  uint32_t add_temp = i.ReadNtohU32();
  this->ID.Set(add_temp);
  this->position.X = i.ReadNtohU32();
  this->position.Y = i.ReadNtohU32();
  this->position.Z = i.ReadNtohU32();
  this->velocity.X = i.ReadNtohU32();
  this->velocity.Y = i.ReadNtohU32();
  this->velocity.Z = i.ReadNtohU32();

  return (messageSize);
}



// ---------------- SDN Routing Message -------------------------------

uint32_t 
MessageHeader::Rm::GetSerializedSize (void) const
{
  return (SDN_RM_HEADER_SIZE +
    this->routingTables.size () * IPV4_ADDRESS_SIZE * SDN_RM_TUPLE_SIZE);
}

void 
MessageHeader::Rm::Print (std::ostream &os) const
{
  /// \todo
}

void
MessageHeader::Rm::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU32 (this->routingMessageSize);
  i.WriteHtonU32 (this->ID.Get());

  for (std::vector<Routing_Tuple>::const_iterator iter = 
    this->routingTables.begin (); 
    iter != this->routingTables.end (); 
    iter++)
    {
      i.WriteHtonU32 (iter->destAddress.Get());
      i.WriteHtonU32 (iter->mask.Get());
      i.WriteHtonU32 (iter->nextHop.Get());
    }
}

uint32_t
MessageHeader::Rm::Deserialize (Buffer::Iterator start, 
  uint32_t messageSize)
{
  Buffer::Iterator i = start;

  this->routingTables.clear ();
  NS_ASSERT (messageSize >= SDN_RM_HEADER_SIZE);

  this->routingMessageSize = i.ReadNtohU32 ();
  uint32_t add_temp = i.ReadNtohU32();
  this->ID.Set(add_temp);

  NS_ASSERT ((messageSize - SDN_RM_HEADER_SIZE) % 
    (IPV4_ADDRESS_SIZE * SDN_RM_TUPLE_SIZE) == 0);
    
  int numTuples = (messageSize - SDN_RM_HEADER_SIZE) 
    / (IPV4_ADDRESS_SIZE * SDN_RM_TUPLE_SIZE);
  for (int n = 0; n < numTuples; ++n)
  {
    Routing_Tuple temp_tuple;
    uint32_t temp_dest = i.ReadNtohU32();
    uint32_t temp_mask = i.ReadNtohU32();
    uint32_t temp_next = i.ReadNtohU32();
    temp_tuple.destAddress.Set(temp_dest);
    temp_tuple.mask.Set(temp_mask);
    temp_tuple.nextHop.Set(temp_next);
    this->routingTables.push_back (temp_tuple);
   }
    
  return (messageSize);
}

// ---------------- SDN Aodv Routing Message -------------------------------

uint32_t
MessageHeader::AodvRm::GetSerializedSize (void) const
{
  return (SDN_AODVRM_HEADER_SIZE +this->forwarding_table.size()*4);
}



void
MessageHeader::AodvRm::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU32 (this->routingMessageSize);
  i.WriteHtonU32 (this->ID.Get());
  i.WriteHtonU32 (this->DesId.Get());
  i.WriteHtonU32 (this->mask);
  i.WriteHtonU32 (this->jump_nums);
  i.WriteHtonU32 (this->stability);

for(auto iter=forwarding_table.begin();iter!=forwarding_table.end();iter++){
	  Ipv4Address temp=*iter;
      i.WriteHtonU32 (temp.Get());
	}
}

uint32_t
MessageHeader::AodvRm::Deserialize (Buffer::Iterator start,
  uint32_t messageSize)
{
  Buffer::Iterator i = start;

  //this->routingTables.clear ();
  NS_ASSERT (messageSize >= SDN_AODVRM_HEADER_SIZE);

  this->routingMessageSize = i.ReadNtohU32 ();
  uint32_t add_temp = i.ReadNtohU32();
  this->ID.Set(add_temp);
  this->DesId.Set(i.ReadNtohU32());
  this->mask=i.ReadNtohU32();
  this->jump_nums=i.ReadNtohU32();
  this->stability=i.ReadNtohU32();

  //NS_ASSERT ((messageSize - SDN_RM_HEADER_SIZE) %
    //(IPV4_ADDRESS_SIZE * SDN_RM_TUPLE_SIZE) == 0);

  int sizevector = (messageSize - SDN_AODVRM_HEADER_SIZE)/4;
   // / (IPV4_ADDRESS_SIZE * SDN_RM_TUPLE_SIZE);
  this->forwarding_table.clear();
  for (int n = 0; n < sizevector; ++n)
  {
	  Ipv4Address temp(i.ReadNtohU32());
    this->forwarding_table.push_back (temp);
   }

  return (messageSize);
}



// ---------------- SDN Aodv Reverse Routing Message -------------------------------

uint32_t
MessageHeader::Aodv_R_Rm::GetSerializedSize (void) const
{
  return (SDN_AODVRM_HEADER_SIZE +this->forwarding_table.size()*4);
}



void
MessageHeader::Aodv_R_Rm::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU32 (this->routingMessageSize);
  i.WriteHtonU32 (this->ID.Get());
  i.WriteHtonU32 (this->DesId.Get());
  i.WriteHtonU32 (this->mask);
  i.WriteHtonU32 (this->jump_nums);
  i.WriteHtonU32 (this->stability);

for(auto iter=forwarding_table.begin();iter!=forwarding_table.end();iter++){
	  Ipv4Address temp=*iter;
      i.WriteHtonU32 (temp.Get());
	}
}

uint32_t
MessageHeader::Aodv_R_Rm::Deserialize (Buffer::Iterator start,
  uint32_t messageSize)
{
  Buffer::Iterator i = start;

  //this->routingTables.clear ();
  NS_ASSERT (messageSize >= SDN_AODVRM_HEADER_SIZE);

  this->routingMessageSize = i.ReadNtohU32 ();
  uint32_t add_temp = i.ReadNtohU32();
  this->ID.Set(add_temp);
  this->DesId.Set(i.ReadNtohU32());
  this->mask=i.ReadNtohU32();
  this->jump_nums=i.ReadNtohU32();
  this->stability=i.ReadNtohU32();

  //NS_ASSERT ((messageSize - SDN_RM_HEADER_SIZE) %
    //(IPV4_ADDRESS_SIZE * SDN_RM_TUPLE_SIZE) == 0);

  int sizevector = (messageSize - SDN_AODVRM_HEADER_SIZE)/4;
   // / (IPV4_ADDRESS_SIZE * SDN_RM_TUPLE_SIZE);
  this->forwarding_table.clear();
  for (int n = 0; n < sizevector; ++n)
  {

	  Ipv4Address temp(i.ReadNtohU32());
    this->forwarding_table.push_back (temp);
   }

  return (messageSize);
}


// ---------------- SDN Appointment Message -------------------------------

void
MessageHeader::Appointment::Print (std::ostream &os) const
{
  //TODO
}

uint32_t
MessageHeader::Appointment::GetSerializedSize () const
{
  return SDN_APPOINTMENT_HEADER_SIZE;
}

void
MessageHeader::Appointment::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU32 (this->ID.Get());
  uint32_t at = 0;
  if (ATField == FORWARDER)
    at = 0xFFFF;
  i.WriteHtonU32 (at);
}

uint32_t
MessageHeader::Appointment::Deserialize (Buffer::Iterator start, uint32_t messageSize)
{
  Buffer::Iterator i = start;

  uint32_t ip_temp = i.ReadNtohU32();
  this->ID.Set (ip_temp);
  uint32_t at = i.ReadNtohU32();
  if (at)
    this->ATField = FORWARDER;
  else
    this->ATField = NORMAL;

  return (messageSize);
}

}
}  // namespace sdn, ns3


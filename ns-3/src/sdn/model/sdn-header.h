/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Haoliang Chen
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

#ifndef SDN_HEADER_H
#define SDN_HEADER_H

#include <stdint.h>
#include <vector>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace olsr {

double EmfToSeconds (uint8_t emf);
uint8_t SecondsToEmf (double seconds);

// Packet Format
//
//    The basic layout of any packet in SDN is as follows (omitting IP and
//    UDP headers):
//
//        0                   1                   2                   3
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |         Packet Length         |    Packet Sequence Number     |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |  Message Type |     Vtime     |         Message Size          |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |          Time To Live         |    Message Sequence Number    |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                                                               |
//       :                            MESSAGE                            :
//       |                                                               |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |  Message Type |     Vtime     |         Message Size          |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |          Time To Live         |    Message Sequence Number    |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                                                               |
//       :                            MESSAGE                            :
//       |                                                               |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       :                                                               :
//                (etc.)
class PacketHeader : public Header
{
public:
  PacketHeader ();
  virtual ~PacketHeader ();

  void SetPacketLength (uint16_t length)
  {
    m_packetLength = length;
  }
  uint16_t GetPacketLength () const
  {
    return m_packetLength;
  }

  void SetPacketSequenceNumber (uint16_t seqnum)
  {
    m_packetSequenceNumber = seqnum;
  }
  uint16_t GetPacketSequenceNumber () const
  {
    return m_packetSequenceNumber;
  }

private:
  uint16_t m_packetLength;
  uint16_t m_packetSequenceNumber;

public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
};


//        0                   1                   2                   3
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |  Message Type |     Vtime     |         Message Size          |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |          Time To Live         |    Message Sequence Number    |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |                                                               |
//       :                            MESSAGE                            :
//       |                                                               |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class MessageHeader : public Header
{
public:

  enum MessageType {
    HELLO_MESSAGE    = 1,
    ROUTING_MESSAGE  = 2,
  };

  MessageHeader ();
  virtual ~MessageHeader ();

  void SetMessageType (MessageType messageType)
  {
    m_messageType = messageType;
  }
  MessageType GetMessageType () const
  {
    return m_messageType;
  }

  void SetVTime (Time time)
  {
    m_vTime = SecondsToEmf (time.GetSeconds ());
  }
  Time GetVTime () const
  {
    return Seconds (EmfToSeconds (m_vTime));
  }

  void SetTimeToLive (uint16_t timeToLive)
  {
    m_timeToLive = timeToLive;
  }
  uint16_t GetTimeToLive () const
  {
    return m_timeToLive;
  }

  void SetMessageSequenceNumber (uint16_t messageSequenceNumber)
  {
    m_messageSequenceNumber = messageSequenceNumber;
  }
  uint16_t GetMessageSequenceNumber () const
  {
    return m_messageSequenceNumber;
  }

   void SetMessageSize (uint16_t messageSize)
   {
     m_messageSize = messageSize;
   }
   uint16_t GetMessageSize () const
   {
     return m_messageSize;
   }

private:
  MessageType m_messageType;
  uint8_t m_vTime;
  uint16_t m_timeToLive;
  uint16_t m_messageSequenceNumber;
  uint16_t m_messageSize;

public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
//Todo
  // HELLO Message Format
  //
  //        0                   1                   2                   3
  //        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |          Reserved             |     Htime     |  Willingness  |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |   Link Code   |   Reserved    |       Link Message Size       |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                  Neighbor Interface Address                   |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                  Neighbor Interface Address                   |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       :                             .  .  .                           :
  //       :                                                               :
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |   Link Code   |   Reserved    |       Link Message Size       |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                  Neighbor Interface Address                   |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                  Neighbor Interface Address                   |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       :                                                               :
  //       :                                       :
  //    (etc.)
  struct Hello
  {
    struct LinkMessage {
      uint8_t linkCode;
      std::vector<Ipv4Address> neighborInterfaceAddresses;
    };

    uint8_t hTime;
    void SetHTime (Time time)
    {
      this->hTime = SecondsToEmf (time.GetSeconds ());
    }
    Time GetHTime () const
    {
      return Seconds (EmfToSeconds (this->hTime));
    }

    uint8_t willingness;
    std::vector<LinkMessage> linkMessages;

    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
  };

  // 9.1.  TC Message Format
  //
  //    The proposed format of a TC message is as follows:
  //
  //        0                   1                   2                   3
  //        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |              ANSN             |           Reserved            |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |               Advertised Neighbor Main Address                |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |               Advertised Neighbor Main Address                |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                              ...                              |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  struct Tc
  {
    std::vector<Ipv4Address> neighborAddresses;
    uint16_t ansn;

    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
  };


  // 12.1.  HNA Message Format
  //
  //    The proposed format of an HNA-message is:
  //
  //        0                   1                   2                   3
  //        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                         Network Address                       |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                             Netmask                           |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                         Network Address                       |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                             Netmask                           |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                              ...                              |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  // Note: HNA stands for Host Network Association
  struct Hna
  {
    struct Association
    {
      Ipv4Address address;
      Ipv4Mask mask;
    };
    std::vector<Association> associations;

    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
  };

private:
  struct
  {
    Mid mid;
    Hello hello;
    Tc tc;
    Hna hna;
  } m_message; // union not allowed

public:

  Mid& GetMid ()
  {
    if (m_messageType == 0)
      {
        m_messageType = MID_MESSAGE;
      }
    else
      {
        NS_ASSERT (m_messageType == MID_MESSAGE);
      }
    return m_message.mid;
  }

  Hello& GetHello ()
  {
    if (m_messageType == 0)
      {
        m_messageType = HELLO_MESSAGE;
      }
    else
      {
        NS_ASSERT (m_messageType == HELLO_MESSAGE);
      }
    return m_message.hello;
  }

  Tc& GetTc ()
  {
    if (m_messageType == 0)
      {
        m_messageType = TC_MESSAGE;
      }
    else
      {
        NS_ASSERT (m_messageType == TC_MESSAGE);
      }
    return m_message.tc;
  }

  Hna& GetHna ()
  {
    if (m_messageType == 0)
      {
        m_messageType = HNA_MESSAGE;
      }
    else
      {
        NS_ASSERT (m_messageType == HNA_MESSAGE);
      }
    return m_message.hna;
  }


  const Mid& GetMid () const
  {
    NS_ASSERT (m_messageType == MID_MESSAGE);
    return m_message.mid;
  }

  const Hello& GetHello () const
  {
    NS_ASSERT (m_messageType == HELLO_MESSAGE);
    return m_message.hello;
  }

  const Tc& GetTc () const
  {
    NS_ASSERT (m_messageType == TC_MESSAGE);
    return m_message.tc;
  }

  const Hna& GetHna () const
  {
    NS_ASSERT (m_messageType == HNA_MESSAGE);
    return m_message.hna;
  }

};




}//namespace sdn
}//namespace ns3


#endif /* SDN_HEADER_H */
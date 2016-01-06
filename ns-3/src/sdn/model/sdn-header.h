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

float     IEEE754 (uint32_t emf);
uint32_t  IEEE754 (float dec);

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

  //  HELLO Message Format
  //
  //        0                   1                   2                   3
  //        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                        ID (IP Address)                        |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                           Position X                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                           Position Y                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                           Position Z                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                           Velocity X                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                           Velocity Y                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                           Velocity Z                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       :                                                               :
  //       :                                       :
  //    (etc.)
  struct Hello
  {
    Ipv4Address ID;
    
    struct Position{
      uint32_t X, Y, Z;
    };
    
    struct Velocity{
      uint32_t X, Y, Z;
    };
    
    Position position;
    void SetPosition(double x, double y, double z)
    {
      this->position.X = IEEE754(x);
      this->position.Y = IEEE754(y);
      this->position.Z = IEEE754(z);
    }
    
    void GetPosition(double &x, double &y, double &z) const
    {
      x = IEEE754(this->position.X);
      y = IEEE754(this->position.Y);
      z = IEEE754(this->position.Z);
    }
    
    Velocity velocity;
    void SetVelocity(double x, double y, double z)
    {
      this->velocity.X = IEEE754(x);
      this->velocity.Y = IEEE754(y);
      this->velocity.Z = IEEE754(z);
    }
    
    void GetVelocity(double &x, double &y, double &z) const
    {
      x = IEEE754(this->velocity.X);
      y = IEEE754(this->velocity.Y);
      z = IEEE754(this->velocity.Z);
    }
    
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
  };
//TODO
  //  Routing Message Format
  //
  //    The proposed format of a routing message is as follows:
  //
  //        0                   1                   2                   3
  //        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                     Routing Message Size                      |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                          destAddress                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                             Mask                              |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                            nextHop                            |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                          destAddress                          |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                             Mask                              |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       |                            nextHop                            |
  //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //       :                                                               :
  //       :                               :

  struct Rm
  {
    struct Routing_Tuble{
      Ipv4Address destAddress, mask, nextHop;
    }
    
    uint32_t routingMessageSize;
    void SetRoutingMessageSize(uint32_t rms)
    {
      this->routingMessageSize = rms;
    }
    uint32_t GetRoutingMessageSize() const
    {
      return this->routingMessageSize;
    }
    
    
    std::vector<Routing_Tuble> routingTables;
    

    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start, uint32_t messageSize);
  };


private:
  struct
  {
    Hello hello;
    Rm rm;
  } m_message; // union not allowed

public:

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

  Rm& GetRm ()
  {
    if (m_messageType == 0)
      {
        m_messageType = ROUTING_MESSAGE;
      }
    else
      {
        NS_ASSERT (m_messageType == ROUTING_MESSAGE);
      }
    return m_message.rm;
  }


  const Hello& GetHello () const
  {
    NS_ASSERT (m_messageType == HELLO_MESSAGE);
    return m_message.hello;
  }

  const Rm& GetRm () const
  {
    NS_ASSERT (m_messageType == ROUTING_MESSAGE);
    return m_message.rm;
  }

};




}//namespace sdn
}//namespace ns3


#endif /* SDN_HEADER_H */
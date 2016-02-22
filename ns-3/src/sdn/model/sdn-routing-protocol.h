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
 * Authors: Haoliang Chen <chl41993@gmail.com>
 */

#ifndef SDN_IMPL_H
#define SDN_IMPL_H

#include "sdn-header.h"

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/event-garbage-collector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/mobility-module.h"

#include <vector>
#include <map>


namespace ns3 {
namespace sdn {


enum NodeType {CAR, CONTROLLER};

/// An SDN's routing table entry.
struct RoutingTableEntry
{
  RoutingTableEntry () : // default values
                           destAddr (), nextHop (),
                           mask(), interface (0) {};

  Ipv4Address destAddr; ///< Address of the destination subnet.
  Ipv4Address nextHop; ///< Address of the next hop.
  Ipv4Address mask; ///< mask of the destination subnet.
  uint32_t interface; ///< Interface index.
};

class RoutingProtocol;

/// \brief SDN routing protocol (Car side) for IPv4
///
class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  static TypeId GetTypeId (void);//implemented

  RoutingProtocol ();//implemented
  virtual ~RoutingProtocol ();//implemented

  ///
  /// \brief Set the SDN main address to the first address on the indicated
  ///        interface
  /// \param interface IPv4 interface index
  ///
  void SetMainInterface (uint32_t interface);//implemented

  ///
  /// Dump the routing table
  /// to logging output (NS_LOG_DEBUG log level).  If logging is disabled,
  /// this function does nothing.
  ///
  void Dump (void);//implemented

  /**
   * Return the list of routing table entries discovered by SDN
   **/
  std::vector<RoutingTableEntry> GetRoutingTableEntries () const;//implemented

 /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);//implemented

private:
  std::set<uint32_t> m_interfaceExclusions;

public:
  std::set<uint32_t> GetInterfaceExclusions () const
  {
    return (m_interfaceExclusions);
  }
  void SetInterfaceExclusions (std::set<uint32_t> exceptions);//implemented

protected:
  virtual void DoInitialize (void);//implemented
private:
  std::map<Ipv4Address, RoutingTableEntry> m_table; ///< Data structure for the routing table.

  EventGarbageCollector m_events;
	
  /// Packets sequence number counter.
  uint16_t m_packetSequenceNumber;
  /// Messages sequence number counter.
  uint16_t m_messageSequenceNumber;

  /// HELLO messages' emission interval.
  Time m_helloInterval;
  /// Routing messages' emission interval.
  Time m_rmInterval;

  Ptr<Ipv4> m_ipv4;

  void Clear ();//implemented
  uint32_t GetSize () const { return (m_table.size ()); }
  void RemoveEntry (const Ipv4Address &dest);//implemented
  void AddEntry (const Ipv4Address &dest,
                 const Ipv4Address &mask,
                 const Ipv4Address &next,
                 uint32_t interface);//implemented
  void AddEntry (const Ipv4Address &dest,
                 const Ipv4Address &mask,
                 const Ipv4Address &next,
                 const Ipv4Address &interfaceAddress);//implemented
  bool Lookup (const Ipv4Address &dest,
               RoutingTableEntry &outEntry) const;//implemented

  // From Ipv4RoutingProtocol
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
                                      const Ipv4Header &header,
                                      Ptr<NetDevice> oif,
                                      Socket::SocketErrno &sockerr);//implemented
  virtual bool RouteInput (Ptr<const Packet> p,
                           const Ipv4Header &header,
                           Ptr<const NetDevice> idev,
                           UnicastForwardCallback ucb,
                           MulticastForwardCallback mcb,
                           LocalDeliverCallback lcb,
                           ErrorCallback ecb);//implemented
  virtual void NotifyInterfaceUp (uint32_t interface);//implemented
  virtual void NotifyInterfaceDown (uint32_t interface);//implemented
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);//implemented
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);//implemented
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);//implemented
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;//implemented

  void DoDispose ();//implemented

  void SendPacket (Ptr<Packet> packet, const MessageList &containedMessages);//implemented

  /// Increments packet sequence number and returns the new value.
  inline uint16_t GetPacketSequenceNumber ();//implemented
  /// Increments message sequence number and returns the new value.
  inline uint16_t GetMessageSequenceNumber ();//implemented

  void RecvSDN (Ptr<Socket> socket);//implemented

  //Ipv4Address GetMainAddress (Ipv4Address iface_addr) const;

  // Timer handlers
  Timer m_helloTimer;
  void HelloTimerExpire ();//implemented

  /// A list of pending messages which are buffered awaiting for being sent.
  sdn::MessageList m_queuedMessages;
  Timer m_queuedMessagesTimer; // timer for throttling outgoing messages

  void QueueMessage (const sdn::MessageHeader &message, Time delay);//implemented
  void SendQueuedMessages ();//implemented
  void SendHello ();//implemented
  void SendRoutingMessage (); //Fullfilled

  void ProcessRm (const sdn::MessageHeader &msg);//implemented
  void ProcessHM (const sdn::MessageHeader &msg); //Fullfilled

  /// Check that address is one of my interfaces
  bool IsMyOwnAddress (const Ipv4Address & a) const;//implemented

  Ipv4Address m_mainAddress;

  // One socket per interface, each bound to that interface's address
  // (reason: for VANET-SDN we need to distinguish CCH and SCH interfaces)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;

  TracedCallback <const PacketHeader &,
                  const MessageList &> m_rxPacketTrace;
  TracedCallback <const PacketHeader &,
                  const MessageList &> m_txPacketTrace;
  TracedCallback <uint32_t> m_routingTableChanged;

  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;  

  // Mobility module for Vanet
  Ptr<MobilityModel> m_mobility;

public:
  void SetMobility (Ptr<MobilityModel> mobility);//implemented

private:
  bool m_isCar;

public:
  void SetType(NodeType nt); //implemented
  bool IsCar() const; //implemented
  bool IsController() const; //implemented


};


}
}  // namespace ns3

#endif /* SDN_IMPL_H */

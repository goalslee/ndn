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


///
/// \brief Implementation of SDN agent on car side 
/// and related classes.
///
/// This is the main file of this software because SDN's behaviour is
/// implemented here.
///

#define NS_LOG_APPEND_CONTEXT                                   \
  if (GetObject<Node> ()) { std::clog << "[node " << GetObject<Node> ()->GetId () << "] "; }


#include "sdn-routing-protocol.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-route.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-header.h"

/********** Useful macros **********/

///
/// \brief Gets the delay between a given time and the current time.
///
/// If given time is previous to the current one, then this macro returns
/// a number close to 0. This is used for scheduling events at a certain moment.
///
#define DELAY(time) (((time) < (Simulator::Now ())) ? Seconds (0.000001) : \
                     (time - Simulator::Now () + Seconds (0.000001)))






/********** Miscellaneous constants **********/

/// Maximum allowed jitter.
#define SDN_MAXJITTER          (m_helloInterval.GetSeconds () / 4)
/// Random number between [0-SDN_MAXJITTER] used to jitter SDN packet transmission.
#define JITTER (Seconds (m_uniformRandomVariable->GetValue (0, SDN_MAXJITTER)))


#define SDN_MAX_SEQ_NUM        65535


#define SDN_PORT_NUMBER 419
/// Maximum number of messages per packet.
#define SDN_MAX_MSGS    64

namespace ns3 {
namespace sdn {

NS_LOG_COMPONENT_DEFINE ("SdnRoutingProtocol");


/********** SDN controller class **********/

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

TypeId
RoutingProtocol::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::sdn::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<RoutingProtocol> ();
  return tid;
}


RoutingProtocol::RoutingProtocol ()
  :
    m_packetSequenceNumber (SDN_MAX_SEQ_NUM),
    m_messageSequenceNumber (SDN_MAX_SEQ_NUM),
    m_helloInterval (Seconds(1)),
    m_ipv4 (0),
    m_helloTimer (Timer::CANCEL_ON_DESTROY),
    m_queuedMessagesTimer (Timer::CANCEL_ON_DESTROY),
    m_nodetype (OTHERS)
{
  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
}

RoutingProtocol::~RoutingProtocol ()
{
  
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  NS_LOG_DEBUG ("Created sdn::RoutingProtocol");
  m_helloTimer.SetFunction 
    (&RoutingProtocol::HelloTimerExpire, this);
  m_queuedMessagesTimer.SetFunction 
    (&RoutingProtocol::SendQueuedMessages, this);

  m_packetSequenceNumber = SDN_MAX_SEQ_NUM;
  m_messageSequenceNumber = SDN_MAX_SEQ_NUM;


  m_ipv4 = ipv4;
}

void RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;

  for (std::map< Ptr<Socket>, Ipv4InterfaceAddress >::iterator iter = 
       m_socketAddresses.begin ();
       iter != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();
  m_table.clear();

  Ipv4RoutingProtocol::DoDispose ();
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  std::ostream* os = stream->GetStream ();
  *os << "Destination\t\tMask\t\tNextHop\t\tInterface\tDistance\n";

  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = 
       m_table.begin ();
       iter != m_table.end (); iter++)
    {
      *os << iter->first << "\t\t";
      *os << iter->second.mask << "\t\t";
      *os << iter->second.nextHop << "\t\t";
      if (Names::FindName (m_ipv4->GetNetDevice (iter->second.interface)) != "")
        {
          *os << 
          Names::FindName (m_ipv4->GetNetDevice (iter->second.interface)) << 
          "\t\t";
        }
      else
        {
          *os << iter->second.interface << "\t\t";
        }
      *os << "\n";
    }
}

void 
RoutingProtocol::DoInitialize ()
{
  if (m_mainAddress == Ipv4Address ())
    {
      Ipv4Address loopback ("127.0.0.1");
      uint32_t count = 0;
      //std::cout<<m_ipv4->GetNInterfaces ()<<std::endl;
      for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
        {
          // CAR Use first address as ID
          // LC Use secend address as ID
          Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
          /*
          uint32_t ip = addr.Get ();
          uint32_t a,b,c,d;
          a = ip % 256;
          ip = ip / 256;
          b = ip % 256;
          ip = ip / 256;
          c = ip % 256;
          d = ip / 256;
          std::cout<<"ITF:"<<d<<"."<<c<<"."<<b<<"."<<a<<std::endl;

          switch (m_nodetype)
          {
            case CAR:
              std::cout<<"This is a CAR"<<std::endl;
              break;
            case LOCAL_CONTROLLER:
              std::cout<<"This is a LC"<<std::endl;
              break;
            case OTHERS:
              std::cout<<"This is a OTHERS"<<std::endl;
          }
          */
          if (addr != loopback)
            {
              if (m_nodetype == CAR)
                {
                  m_mainAddress = addr;
                  break;
                }
              else
                if (m_nodetype == LOCAL_CONTROLLER)
                  {
                    if (count == 1)
                      {
                        m_mainAddress = addr;
                        break;
                      }
                    count++;
                  }
            }
        }

      NS_ASSERT (m_mainAddress != Ipv4Address ());
    }
  /*
  uint32_t ip = m_mainAddress.Get ();
  uint32_t a,b,c,d;
  a = ip % 256;
  ip = ip / 256;
  b = ip % 256;
  ip = ip / 256;
  c = ip % 256;
  d = ip / 256;
  std::cout<<"doinit:"<<d<<"."<<c<<"."<<b<<"."<<a<<std::endl;
  */

  NS_LOG_DEBUG ("Starting SDN on node " << m_mainAddress);

  Ipv4Address loopback ("127.0.0.1");

  bool canRunSdn = false;
  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
    {
      Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
      if (addr == loopback)
        continue;

      //Dont Know  
      /*
      if (addr != m_mainAddress)
        {
          // Create never expiring interface association tuple entries for our
          // own network interfaces, so that GetMainAddress () works to
          // translate the node's own interface addresses into the main address.
          IfaceAssocTuple tuple;
          tuple.ifaceAddr = addr;
          tuple.mainAddr = m_mainAddress;
          AddIfaceAssocTuple (tuple);
          NS_ASSERT (GetMainAddress (addr) == m_mainAddress);
        }
      */
      
      // Obvious
      if(m_interfaceExclusions.find (i) != m_interfaceExclusions.end ())
        continue;

      // Create a socket to listen only on this interface
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (), 
                                                 UdpSocketFactory::GetTypeId ());
      // FALSE
      socket->SetAllowBroadcast (false);
      InetSocketAddress 
        inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), SDN_PORT_NUMBER);
      socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSDN,  this));
      if (socket->Bind (inetAddr))
        {
          NS_FATAL_ERROR ("Failed to bind() OLSR socket");
        }
      socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
      m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);

      canRunSdn = true;
    }

  if(canRunSdn)
    {
      HelloTimerExpire ();
      NS_LOG_DEBUG ("SDN on node (Car) " << m_mainAddress << " started");
    }
}

void 
RoutingProtocol::SetMainInterface (uint32_t interface)
{
  std::cout<<"SetmainInterface"<<std::endl;
  m_mainAddress = m_ipv4->GetAddress (interface, 0).GetLocal ();
}

void 
RoutingProtocol::SetInterfaceExclusions (std::set<uint32_t> exceptions)
{
  //std::cout<<"SetEx"<<std::endl;
  m_interfaceExclusions = exceptions;
}

//
// \brief Processes an incoming %SDN packet (Car Side).
void
RoutingProtocol::RecvSDN (Ptr<Socket> socket)
{
  //std::cout<<"RecvSDN "<<m_mainAddress.Get ()%256<<", Time:"<<Simulator::Now ().GetSeconds ()<<std::endl;
  Ptr<Packet> receivedPacket;
  Address sourceAddress;
  receivedPacket = socket->RecvFrom (sourceAddress);

  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address senderIfaceAddr = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiverIfaceAddr = m_socketAddresses[socket].GetLocal ();
  NS_ASSERT (receiverIfaceAddr != Ipv4Address ());
  NS_LOG_DEBUG ("SDN node " << m_mainAddress
                << " received a SDN packet from "
                << senderIfaceAddr << " to " << receiverIfaceAddr);

  // All routing messages are sent from and to port RT_PORT,
  // so we check it.
  NS_ASSERT (inetSourceAddr.GetPort () == SDN_PORT_NUMBER);

  Ptr<Packet> packet = receivedPacket;

  sdn::PacketHeader sdnPacketHeader;
  packet->RemoveHeader (sdnPacketHeader);
  NS_ASSERT (sdnPacketHeader.GetPacketLength () >= sdnPacketHeader.GetSerializedSize ());
  uint32_t sizeLeft = sdnPacketHeader.GetPacketLength () - sdnPacketHeader.GetSerializedSize ();

  MessageList messages;

  while (sizeLeft)
    {
      MessageHeader messageHeader;
      if (packet->RemoveHeader (messageHeader) == 0)
        NS_ASSERT (false);

      sizeLeft -= messageHeader.GetSerializedSize ();

      NS_LOG_DEBUG ("SDN Msg received with type "
                    << std::dec << int (messageHeader.GetMessageType ())
                    << " TTL=" << int (messageHeader.GetTimeToLive ())
                    << " SeqNum=" << messageHeader.GetMessageSequenceNumber ());
      messages.push_back (messageHeader);
    }

  m_rxPacketTrace (sdnPacketHeader, messages);
  
  for (MessageList::const_iterator messageIter = messages.begin ();
       messageIter != messages.end (); messageIter++)
    {
      const MessageHeader &messageHeader = *messageIter;
      // If ttl is less than or equal to zero, or
      // the receiver is the same as the originator,
      // the message must be silently dropped
      if ((messageHeader.GetTimeToLive () == 0)||(IsMyOwnAddress (sdnPacketHeader.originator)))
        {
          // swallow it
          packet->RemoveAtStart (messageHeader.GetSerializedSize ());
          continue;
        }


      switch (messageHeader.GetMessageType ())
        {
        case sdn::MessageHeader::ROUTING_MESSAGE:
          NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                        << "s SDN node " << m_mainAddress
                        << " received Routing message of size " 
                        << messageHeader.GetSerializedSize ());
          //Controller Node should discare Hello_Message
          if (GetType() == CAR)
            ProcessRm (messageHeader);
          break;

        case sdn::MessageHeader::HELLO_MESSAGE:
          NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                        << "s SDN node " << m_mainAddress
                        << " received Routing message of size "
                        << messageHeader.GetSerializedSize ());
          //Car Node should discare Hello_Message
          if (GetType() == LOCAL_CONTROLLER)
            ProcessHM (messageHeader);
          break;

        default:
          NS_LOG_DEBUG ("SDN message type " <<
                        int (messageHeader.GetMessageType ()) <<
                        " not implemented");
        }

    }
    
}// End of RecvSDN


// \brief Build routing table according to Rm
void
RoutingProtocol::ProcessRm (const sdn::MessageHeader &msg)
{
  NS_LOG_FUNCTION (msg);
  
  const sdn::MessageHeader::Rm &rm = msg.GetRm();
  // Check if this rm is for me
  // Ignore rm that ID does not match.
  if (IsMyOwnAddress (rm.ID))
    {
      Time now = Simulator::Now();
      NS_LOG_DEBUG ("@" << now.GetSeconds() << ":Node " << m_mainAddress
                    << "ProcessRm.");

      NS_ASSERT (rm.GetRoutingMessageSize() >= 0);

      Clear();

      for (std::vector<sdn::MessageHeader::Rm::Routing_Tuple>::const_iterator it = rm.routingTables.begin();
            it != rm.routingTables.end();
            it++)
      {

        AddEntry(it->destAddress,
                 it->mask,
                 it->nextHop,
                 0);
      }
    }
}

void
RoutingProtocol::Clear()
{
  NS_LOG_FUNCTION_NOARGS();
  m_table.clear();
}

void
RoutingProtocol::AddEntry (const Ipv4Address &dest,
                           const Ipv4Address &mask,
                           const Ipv4Address &next,
                           uint32_t interface)
{
  NS_LOG_FUNCTION(this << dest << next << interface << mask << m_mainAddress);
  RoutingTableEntry RTE;
  RTE.destAddr = dest;
  RTE.mask = mask;
  RTE.nextHop = next;
  RTE.interface = interface;
  m_table[dest] = RTE;
}

void
RoutingProtocol::AddEntry (const Ipv4Address &dest,
                           const Ipv4Address &mask,
                           const Ipv4Address &next,
                           const Ipv4Address &interfaceAddress)
{
  NS_LOG_FUNCTION(this << dest << next << interfaceAddress << mask << m_mainAddress);

  NS_ASSERT (m_ipv4);

  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); ++i)
   for (uint32_t j = 0; j< m_ipv4->GetNAddresses(i); j++)
     {
       if (m_ipv4->GetAddress(i,j).GetLocal() == interfaceAddress)
         {
           AddEntry(dest, mask, next, i);
           return;
         }
     }
  NS_ASSERT(false);
  AddEntry(dest, mask, next, 0);
}

bool
RoutingProtocol::Lookup(Ipv4Address const &dest,
                        RoutingTableEntry &outEntry) const
{
  //std::cout<<"|||===|||Lookup "<<m_mainAddress.Get ()%256<<", Dest:" <<dest.Get ()%256<<std::endl;
  std::map<Ipv4Address, RoutingTableEntry>::const_iterator it =
    m_table.find(dest);
  if (it != m_table.end())
    {
      outEntry = it->second;
      return true;
    }
  else
    {
      Ipv4Mask MaskTemp;
      uint16_t max_prefix;
      bool max_prefix_meaningful = false;
      for (it = m_table.begin();it!=m_table.end(); it++)
        {
          MaskTemp.Set (it->second.mask.Get ());
          if (MaskTemp.IsMatch (dest, it->second.destAddr))
            {
              if (!max_prefix_meaningful)
                {
                  max_prefix_meaningful = true;
                  max_prefix = MaskTemp.GetPrefixLength ();
                  outEntry = it->second;
                }
              if (max_prefix_meaningful && (max_prefix < MaskTemp.GetPrefixLength ()))
                {
                  max_prefix = MaskTemp.GetPrefixLength ();
                  outEntry = it->second;
                }
            }
        }
      if (max_prefix_meaningful)
        return true;
      else
        return false;
    }

}

void
RoutingProtocol::RemoveEntry (Ipv4Address const &dest)
{
  m_table.erase (dest);
}


bool
RoutingProtocol::RouteInput(Ptr<const Packet> p,
                            const Ipv4Header &header,
                            Ptr<const NetDevice> idev,
                            UnicastForwardCallback ucb,
                            MulticastForwardCallback mcb,
                            LocalDeliverCallback lcb,
                            ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId () << " " << header.GetDestination ());

  Ipv4Address dest = header.GetDestination();
  Ipv4Address sour = header.GetSource();
  //std::cout<<"RouteInput "<<m_mainAddress.Get ()%256 <<",Source:"<<sour.Get ()%256<< ",Dest:"<<dest.Get ()%256<<std::endl;
  // Consume self-originated packets
  if (IsMyOwnAddress (sour) == true)
    {
      return true;
    }

  // Local delivery
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
  if (m_ipv4->IsDestinationAddress (dest, iif))
    {
      if (!lcb.IsNull ())
        {
          NS_LOG_LOGIC ("Local delivery to " << dest);
          //std::cout<<"RouteInput "<<m_mainAddress.Get ()%256<<"LCB"<<std::endl;
          lcb (p, header, iif);
          return true;
        }
      else
        {
          // The local delivery callback is null.  This may be a multicast
          // or broadcast packet, so return false so that another
          // multicast routing protocol can handle it.  It should be possible
          // to extend this to explicitly check whether it is a unicast
          // packet, and invoke the error callback if so
          return false;
        }
    }

  // Forwarding
  Ptr<Ipv4Route> rtentry;
  RoutingTableEntry entry;
  if (Lookup (header.GetDestination (), entry))
    {
      rtentry = Create<Ipv4Route> ();
      rtentry->SetDestination (dest);
      uint32_t interfaceIdx = entry.interface;
      NS_ASSERT (m_ipv4);
      uint32_t numOfifAddress = m_ipv4->GetNAddresses (interfaceIdx);
      NS_ASSERT (numOfifAddress > 0);
      Ipv4InterfaceAddress ifAddr;
      if (numOfifAddress == 1) {
          ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
        }else{
            // One Interface should only had one address.
            NS_FATAL_ERROR ("Interface had more than one address");
        }
      rtentry->SetSource (ifAddr.GetLocal ());
      rtentry->SetGateway (entry.nextHop);
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));

      NS_LOG_DEBUG ("SDN node " << m_mainAddress
                                 << ": RouteInput for dest=" << header.GetDestination ()
                                 << " --> nextHop=" << entry.nextHop
                                 << " interface=" << entry.interface);
      std::cout<<"RouteInput "<<m_mainAddress.Get ()%256<<"UCB"<<std::endl;
      ucb (rtentry, p, header);
      return true;
    }
  else
    {
      //Fail
      return false;
    }

}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{}
void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{}
void
RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{}
void
RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p,
             const Ipv4Header &header,
             Ptr<NetDevice> oif,
             Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId () << " " << header.GetDestination () << " " << oif);
  Ptr<Ipv4Route> rtentry;
  RoutingTableEntry entry;
  //std::cout<<"RouteOutput "<<m_mainAddress.Get ()%256 << ",Dest:"<<header.GetDestination ().Get ()%256<<std::endl;
  if (Lookup (header.GetDestination (), entry))
    {
      uint32_t interfaceIdx = entry.interface;
      if (oif && m_ipv4->GetInterfaceForDevice (oif) != static_cast<int> (interfaceIdx))
        {
          // We do not attempt to perform a constrained routing search
          // if the caller specifies the oif; we just enforce that
          // that the found route matches the requested outbound interface
          NS_LOG_DEBUG ("SDN node " << m_mainAddress
                                     << ": RouteOutput for dest=" << header.GetDestination ()
                                     << " Route interface " << interfaceIdx
                                     << " does not match requested output interface "
                                     << m_ipv4->GetInterfaceForDevice (oif));
          sockerr = Socket::ERROR_NOROUTETOHOST;
          std::cout<<"does not match requested output interface"<<std::endl;
          return rtentry;
        }
      rtentry = Create<Ipv4Route> ();
      rtentry->SetDestination (header.GetDestination ());
      // the source address is the interface address that matches
      // the destination address (when multiple are present on the
      // outgoing interface, one is selected via scoping rules)
      NS_ASSERT (m_ipv4);
      uint32_t numOifAddresses = m_ipv4->GetNAddresses (interfaceIdx);
      NS_ASSERT (numOifAddresses > 0);
      Ipv4InterfaceAddress ifAddr;
      if (numOifAddresses == 1) {
          ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
        } else {
          /// \todo Implment IP aliasing and OLSR
          NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and OLSR");
        }
      rtentry->SetSource (ifAddr.GetLocal ());
      rtentry->SetGateway (entry.nextHop);
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
      sockerr = Socket::ERROR_NOTERROR;
      NS_LOG_DEBUG ("SDN node " << m_mainAddress
                                 << ": RouteOutput for dest=" << header.GetDestination ()
                                 << " --> nextHop=" << entry.nextHop
                                 << " interface=" << entry.interface);
      NS_LOG_DEBUG ("Found route to " << rtentry->GetDestination () << " via nh " << rtentry->GetGateway () << " with source addr " << rtentry->GetSource () << " and output dev " << rtentry->GetOutputDevice ());
    }
  else
    {
      NS_LOG_DEBUG ("SDN node " << m_mainAddress
                                 << ": RouteOutput for dest=" << header.GetDestination ()
                                 << " No route to host");
      sockerr = Socket::ERROR_NOROUTETOHOST;
      //std::cout<<"No route to host"<<std::endl;
    }
  return rtentry;
}

void
RoutingProtocol::Dump ()
{
#ifdef NS3_LOG_ENABLE
  NS_LOG_DEBUG ("Dumpping For" << m_mainAddress);
#endif //NS3_LOG_ENABLE
}

std::vector<RoutingTableEntry>
RoutingProtocol::GetRoutingTableEntries () const
{
  std::vector<RoutingTableEntry> rtvt;
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator it = m_table.begin ();
       it != m_table.end (); it++)
    {
      rtvt.push_back (it->second);
    }
  return rtvt;
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

uint16_t
RoutingProtocol::GetPacketSequenceNumber ()
{
  m_packetSequenceNumber = (m_packetSequenceNumber + 1) % (SDN_MAX_SEQ_NUM + 1);
  return m_packetSequenceNumber;
}


uint16_t
RoutingProtocol::GetMessageSequenceNumber ()
{
  m_messageSequenceNumber = (m_messageSequenceNumber + 1) % (SDN_MAX_SEQ_NUM + 1);
  return m_messageSequenceNumber;
}

void
RoutingProtocol::HelloTimerExpire ()
{
  //std::cout<<"HelloTimeExpire "<<m_mainAddress.Get ()%256;
  //std::cout<<", Time:"<<Simulator::Now().GetSeconds ()<<std::endl;

  if (GetType() == CAR)
    {
      SendHello ();
      m_helloTimer.Schedule (m_helloInterval);
    }
}


// SDN packets actually send here.
void
RoutingProtocol::SendPacket (Ptr<Packet> packet,
                             const MessageList &containedMessages)
{
  NS_LOG_DEBUG ("SDN node " << m_mainAddress << " sending a SDN packet");
  //std::cout<<"SendPacket  "<<m_mainAddress.Get ()%256 <<std::endl;
  // Add a header
  sdn::PacketHeader header;
  header.originator = this->m_mainAddress;
  header.SetPacketLength (header.GetSerializedSize () + packet->GetSize ());
  header.SetPacketSequenceNumber (GetPacketSequenceNumber ());
  packet->AddHeader (header);

  // Trace it
  m_txPacketTrace (header, containedMessages);

  // Send it
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
         m_socketAddresses.begin (); i != m_socketAddresses.end (); i++)
    {
      Ipv4Address bcast = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
      i->first->SendTo (packet, 0, InetSocketAddress (bcast, SDN_PORT_NUMBER));
    }
}

void
RoutingProtocol::QueueMessage (const sdn::MessageHeader &message, Time delay)
{
  //std::cout<<"QueueMessage  "<<m_mainAddress.Get ()%256 <<std::endl;
  m_queuedMessages.push_back (message);
  if (not m_queuedMessagesTimer.IsRunning ())
    {
      m_queuedMessagesTimer.SetDelay (delay);
      m_queuedMessagesTimer.Schedule ();
    }
}


// NS3 is not multithread, so mutex is unnecessary.
// Here, messages will queue up and send once numMessage is equl to SDN_MAX_MSGS.
// This function will NOT add a header to each message
void
RoutingProtocol::SendQueuedMessages ()
{
  Ptr<Packet> packet = Create<Packet> ();
  int numMessages = 0;

  NS_LOG_DEBUG ("SDN node " << m_mainAddress << ": SendQueuedMessages");
  //std::cout<<"SendQueuedMessages  "<<m_mainAddress.Get ()%256 <<std::endl;
  MessageList msglist;

  for (std::vector<sdn::MessageHeader>::const_iterator message = m_queuedMessages.begin ();
       message != m_queuedMessages.end ();
       message++)
    {
      Ptr<Packet> p = Create<Packet> ();
      p->AddHeader (*message);
      packet->AddAtEnd (p);
      msglist.push_back (*message);
      if (++numMessages == SDN_MAX_MSGS)
        {
          SendPacket (packet, msglist);
          msglist.clear ();
          // Reset variables for next packet
          numMessages = 0;
          packet = Create<Packet> ();
        }
    }

  if (packet->GetSize ())
    {
      SendPacket (packet, msglist);
    }

  m_queuedMessages.clear ();
}

bool
RoutingProtocol::IsMyOwnAddress (const Ipv4Address & a) const
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (a == iface.GetLocal ())
        {
          return true;
        }
    }
  return false;
}

void
RoutingProtocol::SendHello ()
{
  NS_LOG_FUNCTION (this);
  sdn::MessageHeader msg;
  Time now = Simulator::Now ();
  msg.SetVTime (m_helloInterval);
  msg.SetTimeToLive (41993);//Just MY Birthday.
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
  msg.SetMessageType (sdn::MessageHeader::HELLO_MESSAGE);

  sdn::MessageHeader::Hello &hello = msg.GetHello ();
  hello.ID = m_mainAddress;
  Vector pos = m_mobility->GetPosition ();
  Vector vel = m_mobility->GetVelocity ();
  hello.SetPosition (pos.x, pos.y, pos.z);
  hello.SetVelocity (vel.x, vel.y, vel.z);

  NS_LOG_DEBUG ( "SDN HELLO_MESSAGE sent by node: " << hello.ID
                 << "   at " << now.GetSeconds() << "s");
  //std::cout<<"SendHello "<<m_mainAddress.Get ()%256 <<"Pos:"<<pos.x<<","<<pos.y<<","<<pos.z;
  //std::cout<<" Vel:"<<vel.x<<","<<vel.y<<","<<vel.z<<std::endl;
  QueueMessage (msg, JITTER);
}

void
RoutingProtocol::SetMobility (Ptr<MobilityModel> mobility)
{
  m_mobility = mobility;
}

void
RoutingProtocol::SetType (NodeType nt)
{
  /*
  switch (nt)
  {
    case CAR:
      std::cout<<"C"<<std::endl;
      break;
    case LOCAL_CONTROLLER:
      std::cout<<"L"<<std::endl;
      break;
    case OTHERS:
      std::cout<<"O"<<std::endl;
      break;
  }*/
  m_nodetype = nt;
}

NodeType
RoutingProtocol::GetType () const
{
  return m_nodetype;
}

void
RoutingProtocol::SendRoutingMessage ()
{
  // \TODO
}

void
RoutingProtocol::ProcessHM (const sdn::MessageHeader &msg)
{
  // \TODO
}



} // namespace sdn
} // namespace ns3



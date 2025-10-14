/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/rng-seed-manager.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab1Part1Script");

int
main (int argc, char *argv[])
{
  uint32_t nClients = 1;
  uint32_t nPackets = 1;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nClients", "Number of clients (n1, n2, ...). Default is 1 (only n1).", nClients);
  cmd.AddValue ("nPackets", "Number of packets for each client to send. Default is 1.", nPackets);
  cmd.Parse (argc, argv);

  if (nClients < 1)
  {
      nClients = 1;
      NS_LOG_WARN ("nClients was set to less than 1. Setting to default value of 1.");
  }
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (nClients + 1);

  Ptr<Node> n0 = nodes.Get (0);

  NodeContainer clientNodes;
  for (uint32_t i = 1; i <= nClients; ++i)
  {
      clientNodes.Add (nodes.Get (i));
  }

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  InternetStackHelper stack;
  stack.Install (nodes);

  NodeContainer n0n1 = NodeContainer (n0, nodes.Get (1));
  NetDeviceContainer d0d1 = pointToPoint.Install (n0n1);
  Ipv4AddressHelper address1;
  address1.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = address1.Assign (d0d1);

  NetDeviceContainer d0d2;
  Ipv4InterfaceContainer i0i2;
  if (nClients >= 2)
  {
      NodeContainer n0n2 = NodeContainer (n0, nodes.Get (2));
      d0d2 = pointToPoint.Install (n0n2);
      Ipv4AddressHelper address2;
      address2.SetBase ("10.1.2.0", "255.255.255.0");
      i0i2 = address2.Assign (d0d2);
  }

  NetDeviceContainer d0d3;
  Ipv4InterfaceContainer i0i3;
  if (nClients >= 3)
  {
      NodeContainer n0n3 = NodeContainer (n0, nodes.Get (3));
      d0d3 = pointToPoint.Install (n0n3);
      Ipv4AddressHelper address3;
      address3.SetBase ("10.1.3.0", "255.255.255.0");
      i0i3 = address3.Assign (d0d3);
  }

  Ipv4Address serverAddress = i0i1.GetAddress (0);
  uint16_t serverPort = 15;

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  UdpEchoServerHelper echoServer (serverPort);
  ApplicationContainer serverApps = echoServer.Install (n0);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0));

  Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable> ();
  startTime->SetAttribute ("Min", DoubleValue (2.0));
  startTime->SetAttribute ("Max", DoubleValue (7.0));

  for (uint32_t i = 0; i < nClients; ++i)
  {
      UdpEchoClientHelper echoClient (serverAddress, serverPort);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

      ApplicationContainer clientApps = echoClient.Install (clientNodes.Get (i));

      clientApps.Start (Seconds (startTime->GetValue ()));
      clientApps.Stop (Seconds (20.0));
  }

  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

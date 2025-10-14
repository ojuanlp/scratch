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
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptModificado");

int
main (int argc, char *argv[])
{
  uint32_t nCsma = 3;
  uint32_t nPackets = 20;
  bool verbose = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices (n2, n3, ...). Default is 3.", nCsma);
  cmd.AddValue ("nPackets", "Number of packets for UdpEchoClient to send. Default is 20.", nPackets);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
  
  nCsma = nCsma == 0 ? 1 : nCsma;

  NodeContainer p2p1Nodes;
  p2p1Nodes.Create (2); 

  NodeContainer csmaNodes;
  csmaNodes.Add (p2p1Nodes.Get (1)); 
  csmaNodes.Create (nCsma); 

  Ptr<Node> n0 = p2p1Nodes.Get (0);
  Ptr<Node> n4 = csmaNodes.Get (nCsma); 
  
  NodeContainer serverNode;
  serverNode.Create (1); 
  Ptr<Node> n5 = serverNode.Get (0); 
  
  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); 
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("2ms"));    

  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); 
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("2ms"));    

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560))); 

  NetDeviceContainer p2p1Devices;
  p2p1Devices = pointToPoint1.Install (p2p1Nodes);

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer p2p2Nodes = NodeContainer (n4, n5);
  NetDeviceContainer p2p2Devices;
  p2p2Devices = pointToPoint2.Install (p2p2Nodes);

  InternetStackHelper stack;
  stack.Install (p2p1Nodes.Get (0));
  stack.Install (csmaNodes);        
  stack.Install (serverNode);       

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2p1Interfaces;
  p2p1Interfaces = address.Assign (p2p1Devices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer p2p2Interfaces;
  p2p2Interfaces = address.Assign (p2p2Devices);
  
  Ipv4Address serverAddress = p2p2Interfaces.GetAddress (1);
  uint16_t serverPort = 9;

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  UdpEchoServerHelper echoServer (serverPort);

  ApplicationContainer serverApps = echoServer.Install (n5);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0));

  UdpEchoClientHelper echoClient (serverAddress, serverPort);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (n0);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (20.0));

  pointToPoint1.EnablePcapAll ("second-p2p1");
  pointToPoint2.EnablePcapAll ("second-p2p2");
  csma.EnablePcap ("second-csma", csmaDevices.Get (0), true);

  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

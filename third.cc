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
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab1Part3Script");

int
main (int argc, char *argv[])
{
  uint32_t nWifi = 3;
  uint32_t nPackets = 1;

  bool verbose = true;
  bool tracing = false;
  
  CommandLine cmd (__FILE__);
  cmd.AddValue ("nWifi", "Number of STA devices PER WiFi network (nWifi total = 2 * nWifi). Max 9.", nWifi);
  cmd.AddValue ("nPackets", "Number of packets for UdpEchoClient to send. Max 20 for plotting.", nPackets);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  if (nWifi > 9)
    {
      std::cout << "nWifi deve ser 9 ou menos; o número total de nós sem fio excede 18." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  NodeContainer p2pNodes;
  p2pNodes.Create (2);
  Ptr<Node> n0 = p2pNodes.Get (0);
  Ptr<Node> n1 = p2pNodes.Get (1);
  
  NodeContainer wifi1StaNodes;
  wifi1StaNodes.Create (nWifi);

  NodeContainer wifi2StaNodes;
  wifi2StaNodes.Create (nWifi);
  
  Ptr<Node> n7 = wifi1StaNodes.Get (nWifi - 1);
  Ptr<Node> n4 = wifi2StaNodes.Get (nWifi - 1);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  YansWifiPhyHelper phy;
  WifiMacHelper mac;
  WifiHelper wifi;
  Ssid ssid1 = Ssid ("Rede-A");
  Ssid ssid2 = Ssid ("Rede-B");

  YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default ();
  phy.SetChannel (channel1.Create ()); 

  NetDeviceContainer sta1Devices;
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid1));
  sta1Devices = wifi.Install (phy, mac, wifi1StaNodes);

  NetDeviceContainer ap1Devices;
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid1));
  ap1Devices = wifi.Install (phy, mac, n0);

  YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();

  phy.SetChannel (channel2.Create ()); 

  NetDeviceContainer sta2Devices;
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid2));
  sta2Devices = wifi.Install (phy, mac, wifi2StaNodes);

  NetDeviceContainer ap2Devices;
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid2));
  ap2Devices = wifi.Install (phy, mac, n1);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifi1StaNodes);

  mobility.Install (wifi2StaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (n0);
  mobility.Install (n1);

  InternetStackHelper stack;
  stack.Install (p2pNodes);
  stack.Install (wifi1StaNodes);
  stack.Install (wifi2StaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer ap2Interfaces = address.Assign (ap2Devices);
  address.Assign (sta2Devices);
  
  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (ap1Devices);
  address.Assign (sta1Devices);

Ipv4Address serverAddress = n4->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();

uint16_t serverPort = 9;

  UdpEchoServerHelper echoServer (serverPort);
  ApplicationContainer serverApps = echoServer.Install (n4);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0));

  UdpEchoClientHelper echoClient (serverAddress, serverPort);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (n7);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (20.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (20.0));

  if (tracing)
    {
      phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      pointToPoint.EnablePcapAll ("lab1-part3-p2p");

      phy.EnablePcap ("lab1-part3-wifi1-ap", ap1Devices.Get (0));
      phy.EnablePcap ("lab1-part3-wifi2-ap", ap2Devices.Get (0));
    }

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

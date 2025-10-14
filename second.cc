/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Este programa é baseado em second.cc e modificado para lab1-part2.cc.
 * Resolve a Parte 2 do laboratório, adicionando um segundo link P2P
 * ao nó do servidor e configurando a simulação para análise de atraso.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h" // Incluído se desejar visualização (não obrigatório)

// Nova Topologia:
//
//       10.1.1.0           10.1.3.0
// n0 (Client) --- n1 | n2 | n3 | n4 --- n5 (Server)
//    point-to-point  |    |    |    |  point-to-point
//                    ================
//                      LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab1Part2Script");

int
main (int argc, char *argv[])
{
  // Parâmetros de Linha de Comando
  uint32_t nCsma = 3;    // Número de nós "extras" CSMA (n2, n3, n4 no exemplo)
  uint32_t nClients = 1; // nClients para compatibilidade com o enunciado (n0 é o único cliente)
  uint32_t nPackets = 20; // Padrão: 20 pacotes, conforme o enunciado para análise

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices (n2, n3, ...). Default is 3.", nCsma);
  cmd.AddValue ("nPackets", "Number of packets for UdpEchoClient to send. Default is 20.", nPackets);
  // O enunciado pede nClients, mas a topologia tem apenas um cliente (n0). Usamos nClients=1 como valor fixo.
  // cmd.AddValue ("nClients", "Number of clients (fixed at 1 for this topology).", nClients); // Mantido o default

  cmd.Parse (argc,argv);

  // O enunciado original usa o exemplo de 4 nós CSMA (n1, n2, n3, n4), onde n1 é compartilhado com P2P.
  // O número de "extra" CSMA é nCsma. Se nCsma=3, temos 4 nós CSMA no total (n1 + 3 extras).
  // A topologia de exemplo mostra 5 nós CSMA (n1, n2, n3, n4, n5) onde n5 é o servidor.
  // Vamos usar nCsma = 4 para ter n1, n2, n3, n4, com n4 sendo o último nó na LAN.
  // Para 4 CSMA nodes (3 "extra" nodes), o nCsma deve ser 3.

  // O número total de nós na LAN CSMA é nCsma + 1 (onde +1 é o nó n1)
  // O servidor n5 será um nó separado, criando a nova topologia.
  
  // Garantir que nCsma seja pelo menos 1
  nCsma = nCsma == 0 ? 1 : nCsma;

  // 1. Criação dos Nós
  NodeContainer p2pNodes; // Contém n0 e n1
  p2pNodes.Create (2); // n0 (Cliente) e n1

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1)); // Adiciona n1 (compartilhado)
  csmaNodes.Create (nCsma); // Adiciona n2, n3, n4 (se nCsma=3)

  Ptr<Node> n0 = p2pNodes.Get (0); // Cliente
  Ptr<Node> n4 = csmaNodes.Get (nCsma); // Último nó na LAN (n4 no exemplo)
  
  NodeContainer serverNode;
  serverNode.Create (1); // n5 (Servidor)
  Ptr<Node> n5 = serverNode.Get (0); // Servidor
  
  // 2. Helpers e Configuração de Links

  // Link P2P Original (n0 --- n1)
  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); // 5 Mbps (Original)
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("2ms"));    // 2 ms (Original)

  // Link P2P Modificado (n4 --- n5) - Deve ter os MESMOS parâmetros (Nota importante)
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); // 5 Mbps (Novo P2P)
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("2ms"));    // 2 ms (Novo P2P)

  // Link CSMA (LAN 10.1.2.0)
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560))); // 6.56 us

  // 3. Instalação dos Dispositivos

  // P2P 1: n0 --- n1
  NetDeviceContainer p2p1Devices;
  p2p1Devices = pointToPoint1.Install (p2pNodes);

  // CSMA: n1 | n2 | n3 | n4
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  // P2P 2: n4 --- n5 (Último nó da LAN CSMA e Servidor)
  NodeContainer p2p2Nodes = NodeContainer (n4, n5);
  NetDeviceContainer p2p2Devices;
  p2p2Devices = pointToPoint2.Install (p2p2Nodes);

  // 4. Instalação da Stack de Protocolos
  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0)); // n0
  stack.Install (csmaNodes);        // n1, n2, n3, n4
  stack.Install (serverNode);       // n5

  // 5. Configuração de Endereços IP

  // 10.1.1.0: n0 (Client) --- n1
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2p1Interfaces;
  p2p1Interfaces = address.Assign (p2p1Devices);

  // 10.1.2.0: LAN CSMA
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  // 10.1.3.0: n4 --- n5 (Server)
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer p2p2Interfaces;
  p2p2Interfaces = address.Assign (p2p2Devices);
  
  // Endereço do Servidor (n5) para o Cliente (n0)
  Ipv4Address serverAddress = p2p2Interfaces.GetAddress (1); // n5 é o segundo nó do p2p2, IP .2
  uint16_t serverPort = 9;

  // 6. Configuração do Roteamento
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // 7. Configuração da Aplicação Servidora (UdpEchoServer)
  // O Servidor é n5
  UdpEchoServerHelper echoServer (serverPort);

  ApplicationContainer serverApps = echoServer.Install (n5);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0));

  // 8. Configuração da Aplicação Cliente (UdpEchoClient)
  // O Cliente é n0
  UdpEchoClientHelper echoClient (serverAddress, serverPort);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets)); // Usar nPackets (default 20)
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (n0);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (20.0)); // Garantir que pare em 20.0s (conforme a nota importante)

  // 9. Captura de Pacotes (PCAP)
  
  // Capturas solicitadas (mínimo 3):
  // 1. O P2P original ("second")
  pointToPoint1.EnablePcapAll ("lab1-part2-p2p1");
  // 2. O novo P2P ("p2p2")
  pointToPoint2.EnablePcapAll ("lab1-part2-p2p2");
  // 3. A LAN CSMA (n1, dispositivo 0 no container CSMA)
  csma.EnablePcap ("lab1-part2-csma", csmaDevices.Get (0), true);
  
  // Para seguir o original second.cc, a captura de todos os dispositivos na LAN
  // csma.EnablePcapAll ("lab1-part2-csma-all");

  // 10. Execução
  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Este programa é baseado em first.cc e modificado para lab1-part1.cc.
 * Resolve a Parte 1 do laboratório, configurando múltiplos clientes
 * com links ponto-a-ponto e roteamento estático.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/rng-seed-manager.h"

// Topologia:
//
//        10.1.2.0
// n_clients-1 --- n0 (Server) --- n1 (Client)
//                 /
//           10.1.3.0
//                 |
//                 n_clients
//
// n0 é o servidor, n1, n2, ..., n_clients+1 são os clientes.
// O número de clientes é configurável por linha de comando.
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab1Part1Script");

int
main (int argc, char *argv[])
{
  uint32_t nClients = 1; // nClients no enunciado vai de n1 a nX
  uint32_t nPackets = 1; // Número de pacotes por cliente

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nClients", "Number of clients (n1, n2, ...). Default is 1 (only n1).", nClients);
  cmd.AddValue ("nPackets", "Number of packets for each client to send. Default is 1.", nPackets);
  cmd.Parse (argc, argv);

  // Garantir que nClients seja pelo menos 1 (n1)
  if (nClients < 1)
  {
      nClients = 1;
      NS_LOG_WARN ("nClients was set to less than 1. Setting to default value of 1.");
  }
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // 1. Criação dos Nós
  // n0 é o servidor, e os outros nClients nós são os clientes.
  NodeContainer nodes;
  nodes.Create (nClients + 1); // n0 (Server) + nClients (n1, n2, ...)

  Ptr<Node> n0 = nodes.Get (0); // Servidor
  // Os clientes estão nos índices 1 até nClients.
  NodeContainer clientNodes;
  for (uint32_t i = 1; i <= nClients; ++i)
  {
      clientNodes.Add (nodes.Get (i));
  }

  // 2. Criação dos Links Point-to-Point (Configurações baseadas em first.cc)
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // 3. Instalação da Stack de Protocolos
  InternetStackHelper stack;
  stack.Install (nodes);

  // 4. Configuração da Topologia e Endereçamento

  // Configuração Server (n0) <-> Client (n1) - Sub-rede 10.1.1.0
  NodeContainer n0n1 = NodeContainer (n0, nodes.Get (1));
  NetDeviceContainer d0d1 = pointToPoint.Install (n0n1);
  Ipv4AddressHelper address1;
  address1.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = address1.Assign (d0d1);

  // Configuração Server (n0) <-> Client (n2) (se nClients >= 2) - Sub-rede 10.1.2.0
  // Para simplificar e evitar sub-redes não utilizadas quando nClients é baixo,
  // vamos usar um link separado apenas se houver pelo menos 2 clientes.
  // Note: O enunciado sugere que n1 e n2 estejam em sub-redes diferentes.
  // Vamos configurar n1 em 10.1.1.0 e o restante em 10.1.2.0 (e 10.1.3.0 se nClients >= 3).
  
  // Link para o cliente n2 (se existir)
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

  // Link para o cliente n3 (se existir)
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

  // 5. Configuração do Roteamento
  // O servidor (n0) tem múltiplas interfaces (em sub-redes diferentes),
  // o ns-3 deve configurar o roteamento corretamente, mas para garantir que
  // os clientes consigam enviar para o endereço 10.1.1.2 (o endereço de n0 na 10.1.1.0),
  // e para o roteamento estático funcionar (como sugerido para 'second.cc'),
  // vamos forçar o roteamento.

  // Note 1: O enunciado sugere que as requisições de TODOS os clientes devem ir para 10.1.1.2.
  // 10.1.1.2 é o endereço de n0 (Servidor) na sub-rede 10.1.1.0 (interface ligada a n1).
  Ipv4Address serverAddress = i0i1.GetAddress (0); // 10.1.1.2 (endereço do Server n0)
  uint16_t serverPort = 15; // Porta 15 (Nota 4)

  // O ns-3 deve configurar o roteamento automático (Global Routing) quando temos mais de um nó roteador
  // (neste caso, o n0 tem múltiplas interfaces em sub-redes diferentes).
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // 6. Configuração da Aplicação Servidora (UdpEchoServer)
  UdpEchoServerHelper echoServer (serverPort);
  ApplicationContainer serverApps = echoServer.Install (n0);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0)); // Para em 20 segundos (Nota 3)

  // 7. Configuração das Aplicações Clientes (UdpEchoClient)
  Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable> (); // Criação de um gerador de números aleatórios
  startTime->SetAttribute ("Min", DoubleValue (2.0)); // Tempo mínimo de 2s (Nota 3)
  startTime->SetAttribute ("Max", DoubleValue (7.0)); // Tempo máximo de 7s (Nota 3)

  for (uint32_t i = 0; i < nClients; ++i)
  {
      UdpEchoClientHelper echoClient (serverAddress, serverPort); // Todos enviam para 10.1.1.2, Porta 15 (Notas 1 e 4)
      echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets)); // Usar nPackets (argumento de linha de comando)
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0))); // Intervalo de 1.0s (igual a first.cc - Nota 5)
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024)); // Tamanho de 1024 bytes (igual a first.cc - Nota 5)

      ApplicationContainer clientApps = echoClient.Install (clientNodes.Get (i));

      // Horário de início aleatório (Nota 3)
      clientApps.Start (Seconds (startTime->GetValue ()));
      clientApps.Stop (Seconds (20.0)); // Para em 20 segundos (Nota 3)
  }

  // 8. Execução da Simulação
  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

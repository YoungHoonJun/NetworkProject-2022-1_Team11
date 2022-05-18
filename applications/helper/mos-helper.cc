/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "mos-helper.h"
#include "ns3/mos-server.h"
#include "ns3/mos-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3{

MosServerHelper::MosServerHelper(uint16_t port)
{
  m_factory.SetTypeId (MosServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
MosServerHelper::SetAttribute(std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer 
MosServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer 
MosServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer 
MosServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin(); i != c.End(); i++)
  {
    apps.Add (InstallPriv (*i));
  }
  
  return apps;
}

Ptr<Application>
MosServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<MosServer> ();
  node->AddApplication (app);

  return app;
}

MosClientHelper::MosClientHelper(Address ip, uint16_t port)
{
  m_factory.SetTypeId (MosClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (ip));
  SetAttribute ("RemotePort", UintegerValue (port));
}


MosClientHelper::MosClientHelper(Address addr)
{
  m_factory.SetTypeId (MosClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (addr));
}

void
MosClientHelper::SetAttribute(std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer 
MosClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer 
MosClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer 
MosClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin(); i != c.End(); i++)
  {
    apps.Add (InstallPriv (*i));
  }
  
  return apps;
}

Ptr<Application>
MosClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<MosClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3

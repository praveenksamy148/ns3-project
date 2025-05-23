/*
 * Copyright (c) 2007-2009 Strasbourg University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include "ipv6-interface.h"

#include "icmpv6-l4-protocol.h"
#include "ipv6-header.h"
#include "ipv6-l3-protocol.h"
#include "ipv6-queue-disc-item.h"
#include "loopback-net-device.h"
#include "ndisc-cache.h"

#include "ns3/log.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/traffic-control-layer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6Interface");

NS_OBJECT_ENSURE_REGISTERED(Ipv6Interface);

TypeId
Ipv6Interface::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv6Interface")
            .SetParent<Object>()
            .SetGroupName("Internet")
            .AddTraceSource("InterfaceStatus",
                            "Interface has been set up or down.",
                            MakeTraceSourceAccessor(&Ipv6Interface::m_interfaceStatus),
                            "ns3::Ipv6Address::TracedCallback");
    return tid;
}

Ipv6Interface::Ipv6Interface()
    : m_ifup(false),
      m_forwarding(true),
      m_metric(1),
      m_node(nullptr),
      m_device(nullptr),
      m_tc(nullptr),
      m_ndCache(nullptr),
      m_curHopLimit(0),
      m_baseReachableTime(0),
      m_reachableTime(0),
      m_retransTimer(0)
{
    NS_LOG_FUNCTION(this);
}

Ipv6Interface::~Ipv6Interface()
{
}

void
Ipv6Interface::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_node = nullptr;
    m_device = nullptr;
    m_tc = nullptr;
    m_ndCache = nullptr;
    Object::DoDispose();
}

void
Ipv6Interface::DoSetup()
{
    NS_LOG_FUNCTION(this);

    if (!m_node || !m_device)
    {
        return;
    }

    /* set up link-local address */
    if (!DynamicCast<LoopbackNetDevice>(m_device)) /* no autoconf for ip6-localhost */
    {
        Address addr = GetDevice()->GetAddress();
        Ipv6InterfaceAddress ifaddr =
            Ipv6InterfaceAddress(Ipv6Address::MakeAutoconfiguredLinkLocalAddress(addr),
                                 Ipv6Prefix(64));
        AddAddress(ifaddr);
        m_linkLocalAddress = ifaddr;
    }
    else
    {
        return; /* no NDISC cache for ip6-localhost */
    }
    int32_t interfaceId = m_node->GetObject<Ipv6>()->GetInterfaceForDevice(m_device);
    Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
        m_node->GetObject<Ipv6>()->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(),
                                               interfaceId));

    if (icmpv6 && !m_ndCache)
    {
        m_ndCache = icmpv6->CreateCache(m_device, this);
    }
}

void
Ipv6Interface::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

void
Ipv6Interface::SetDevice(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    m_device = device;
}

void
Ipv6Interface::SetTrafficControl(Ptr<TrafficControlLayer> tc)
{
    NS_LOG_FUNCTION(this << tc);
    m_tc = tc;
}

Ptr<NetDevice>
Ipv6Interface::GetDevice() const
{
    NS_LOG_FUNCTION(this);
    return m_device;
}

void
Ipv6Interface::SetMetric(uint16_t metric)
{
    NS_LOG_FUNCTION(this << metric);
    m_metric = metric;
}

uint16_t
Ipv6Interface::GetMetric() const
{
    NS_LOG_FUNCTION(this);
    return m_metric;
}

bool
Ipv6Interface::IsUp() const
{
    NS_LOG_FUNCTION(this);
    return m_ifup;
}

bool
Ipv6Interface::IsDown() const
{
    NS_LOG_FUNCTION(this);
    return !m_ifup;
}

void
Ipv6Interface::SetUp()
{
    NS_LOG_FUNCTION(this);

    if (m_ifup)
    {
        return;
    }
    DoSetup();
    m_ifup = true;

    Ptr<Ipv6> ip = m_node->GetObject<Ipv6>();
    NS_ASSERT_MSG(ip, "IPv6 not installed on node.");
    auto ifIndex = ip->GetInterfaceForDevice(m_device);
    m_interfaceStatus(m_ifup, ifIndex);
}

void
Ipv6Interface::SetDown()
{
    NS_LOG_FUNCTION(this);
    m_ifup = false;
    m_addresses.clear();
    m_ndCache->Flush();

    Ptr<Ipv6> ip = m_node->GetObject<Ipv6>();
    NS_ASSERT_MSG(ip, "IPv6 not installed on node.");
    auto ifIndex = ip->GetInterfaceForDevice(m_device);
    m_interfaceStatus(m_ifup, ifIndex);
}

bool
Ipv6Interface::IsForwarding() const
{
    NS_LOG_FUNCTION(this);
    return m_forwarding;
}

void
Ipv6Interface::SetForwarding(bool forwarding)
{
    NS_LOG_FUNCTION(this << forwarding);
    m_forwarding = forwarding;
}

bool
Ipv6Interface::AddAddress(Ipv6InterfaceAddress iface)
{
    NS_LOG_FUNCTION(this << iface);
    Ipv6Address addr = iface.GetAddress();

    /* DAD handling */
    if (!addr.IsAny())
    {
        for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
        {
            if (it->first.GetAddress() == addr)
            {
                return false;
            }
        }

        Ipv6Address solicited = Ipv6Address::MakeSolicitedAddress(iface.GetAddress());
        m_addresses.emplace_back(iface, solicited);
        if (!m_addAddressCallback.IsNull())
        {
            m_addAddressCallback(this, addr);
        }

        if (!addr.IsAny() || !addr.IsLocalhost())
        {
            /* DAD handling */

            int32_t interfaceId = m_node->GetObject<Ipv6>()->GetInterfaceForDevice(m_device);
            Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
                m_node->GetObject<Ipv6>()->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(),
                                                       interfaceId));

            if (icmpv6)
            {
                if (icmpv6->IsAlwaysDad())
                {
                    Simulator::Schedule(Seconds(0.), &Icmpv6L4Protocol::DoDAD, icmpv6, addr, this);
                    Simulator::Schedule(icmpv6->GetDadTimeout(),
                                        &Icmpv6L4Protocol::FunctionDadTimeout,
                                        icmpv6,
                                        this,
                                        addr);
                }
                else
                {
                    Simulator::Schedule(Seconds(0.),
                                        &Icmpv6L4Protocol::FunctionDadTimeout,
                                        icmpv6,
                                        this,
                                        addr);
                }
            }
        }
        return true;
    }

    /* bad address */
    return false;
}

Ipv6InterfaceAddress
Ipv6Interface::GetLinkLocalAddress() const
{
    /* IPv6 interface has always at least one IPv6 link-local address */
    NS_LOG_FUNCTION(this);

    return m_linkLocalAddress;
}

bool
Ipv6Interface::IsSolicitedMulticastAddress(Ipv6Address address) const
{
    /* IPv6 interface has always at least one IPv6 Solicited Multicast address */
    NS_LOG_FUNCTION(this << address);

    for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
    {
        if (it->second == address)
        {
            return true;
        }
    }

    return false;
}

Ipv6InterfaceAddress
Ipv6Interface::GetAddress(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    uint32_t i = 0;

    if (m_addresses.size() > index)
    {
        for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
        {
            if (i == index)
            {
                return it->first;
            }
            i++;
        }
    }
    else
    {
        NS_FATAL_ERROR("index " << index << " out of bounds");
    }
    Ipv6InterfaceAddress addr;
    return addr; /* quiet compiler */
}

uint32_t
Ipv6Interface::GetNAddresses() const
{
    NS_LOG_FUNCTION(this);
    return m_addresses.size();
}

Ipv6InterfaceAddress
Ipv6Interface::RemoveAddress(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    uint32_t i = 0;

    if (m_addresses.size() < index)
    {
        NS_FATAL_ERROR("Removing index that does not exist in Ipv6Interface::RemoveAddress");
    }

    for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
    {
        if (i == index)
        {
            Ipv6InterfaceAddress iface = it->first;
            m_addresses.erase(it);
            if (!m_removeAddressCallback.IsNull())
            {
                m_removeAddressCallback(this, iface);
            }
            return iface;
        }

        i++;
    }
    NS_FATAL_ERROR("Address " << index << " not found");
    Ipv6InterfaceAddress addr;
    return addr; /* quiet compiler */
}

Ipv6InterfaceAddress
Ipv6Interface::RemoveAddress(Ipv6Address address)
{
    NS_LOG_FUNCTION(this << address);

    if (address == Ipv6Address::GetLoopback())
    {
        NS_LOG_WARN("Cannot remove loopback address.");
        return Ipv6InterfaceAddress();
    }

    for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
    {
        if (it->first.GetAddress() == address)
        {
            Ipv6InterfaceAddress iface = it->first;
            m_addresses.erase(it);
            if (!m_removeAddressCallback.IsNull())
            {
                m_removeAddressCallback(this, iface);
            }
            return iface;
        }
    }
    return Ipv6InterfaceAddress();
}

Ipv6InterfaceAddress
Ipv6Interface::GetAddressMatchingDestination(Ipv6Address dst)
{
    NS_LOG_FUNCTION(this << dst);

    for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
    {
        Ipv6InterfaceAddress ifaddr = it->first;

        if (ifaddr.GetPrefix().IsMatch(ifaddr.GetAddress(), dst))
        {
            return ifaddr;
        }
    }

    /*  NS_ASSERT_MSG (false, "Not matching address."); */
    Ipv6InterfaceAddress ret;
    return ret; /* quiet compiler */
}

void
Ipv6Interface::Send(Ptr<Packet> p, const Ipv6Header& hdr, Ipv6Address dest)
{
    NS_LOG_FUNCTION(this << p << dest);

    if (!IsUp())
    {
        return;
    }

    Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol>();

    /* check if destination is localhost (::1), if yes we don't pass through
     * traffic control layer */
    if (DynamicCast<LoopbackNetDevice>(m_device))
    {
        /** @todo additional checks needed here (such as whether multicast
         * goes to loopback)?
         */
        p->AddHeader(hdr);
        m_device->Send(p, m_device->GetBroadcast(), Ipv6L3Protocol::PROT_NUMBER);
        return;
    }

    NS_ASSERT(m_tc);

    /* check if destination is for one of our interface */
    for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
    {
        if (dest == it->first.GetAddress())
        {
            p->AddHeader(hdr);
            Simulator::ScheduleNow(&TrafficControlLayer::Receive,
                                   m_tc,
                                   m_device,
                                   p,
                                   Ipv6L3Protocol::PROT_NUMBER,
                                   m_device->GetBroadcast(),
                                   m_device->GetBroadcast(),
                                   NetDevice::PACKET_HOST);
            return;
        }
    }

    /* other address */
    if (m_device->NeedsArp())
    {
        NS_LOG_LOGIC("Needs NDISC " << dest);

        int32_t interfaceId = m_node->GetObject<Ipv6>()->GetInterfaceForDevice(m_device);
        Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
            m_node->GetObject<Ipv6>()->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(),
                                                   interfaceId));

        Address hardwareDestination;
        bool found = false;

        NS_ASSERT(icmpv6);

        if (dest.IsMulticast())
        {
            NS_LOG_LOGIC("IsMulticast");
            NS_ASSERT_MSG(
                m_device->IsMulticast(),
                "Ipv6Interface::SendTo (): Sending multicast packet over non-multicast device");

            hardwareDestination = m_device->GetMulticast(dest);
            found = true;
        }
        else
        {
            NS_LOG_LOGIC("NDISC Lookup");
            found = icmpv6->Lookup(p, hdr, dest, GetDevice(), m_ndCache, &hardwareDestination);
        }

        if (found)
        {
            NS_LOG_LOGIC("Address Resolved.  Send.");
            m_tc->Send(m_device,
                       Create<Ipv6QueueDiscItem>(p,
                                                 hardwareDestination,
                                                 Ipv6L3Protocol::PROT_NUMBER,
                                                 hdr));
        }
    }
    else
    {
        NS_LOG_LOGIC("Doesn't need NDISC");
        m_tc->Send(m_device,
                   Create<Ipv6QueueDiscItem>(p,
                                             m_device->GetBroadcast(),
                                             Ipv6L3Protocol::PROT_NUMBER,
                                             hdr));
    }
}

void
Ipv6Interface::SetCurHopLimit(uint8_t curHopLimit)
{
    NS_LOG_FUNCTION(this << curHopLimit);
    m_curHopLimit = curHopLimit;
}

uint8_t
Ipv6Interface::GetCurHopLimit() const
{
    NS_LOG_FUNCTION(this);
    return m_curHopLimit;
}

void
Ipv6Interface::SetBaseReachableTime(uint16_t baseReachableTime)
{
    NS_LOG_FUNCTION(this << baseReachableTime);
    m_baseReachableTime = baseReachableTime;
}

uint16_t
Ipv6Interface::GetBaseReachableTime() const
{
    NS_LOG_FUNCTION(this);
    return m_baseReachableTime;
}

void
Ipv6Interface::SetReachableTime(uint16_t reachableTime)
{
    NS_LOG_FUNCTION(this << reachableTime);
    m_reachableTime = reachableTime;
}

uint16_t
Ipv6Interface::GetReachableTime() const
{
    NS_LOG_FUNCTION(this);
    return m_reachableTime;
}

void
Ipv6Interface::SetRetransTimer(uint16_t retransTimer)
{
    NS_LOG_FUNCTION(this << retransTimer);
    m_retransTimer = retransTimer;
}

uint16_t
Ipv6Interface::GetRetransTimer() const
{
    NS_LOG_FUNCTION(this);
    return m_retransTimer;
}

void
Ipv6Interface::SetState(Ipv6Address address, Ipv6InterfaceAddress::State_e state)
{
    NS_LOG_FUNCTION(this << address << state);

    for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
    {
        if (it->first.GetAddress() == address)
        {
            it->first.SetState(state);
            return;
        }
    }
    /* not found, maybe address has expired */
}

void
Ipv6Interface::SetNsDadUid(Ipv6Address address, uint32_t uid)
{
    NS_LOG_FUNCTION(this << address << uid);

    for (auto it = m_addresses.begin(); it != m_addresses.end(); ++it)
    {
        if (it->first.GetAddress() == address)
        {
            it->first.SetNsDadUid(uid);
            return;
        }
    }
    /* not found, maybe address has expired */
}

Ptr<NdiscCache>
Ipv6Interface::GetNdiscCache() const
{
    NS_LOG_FUNCTION(this);
    return m_ndCache;
}

void
Ipv6Interface::RemoveAddressCallback(
    Callback<void, Ptr<Ipv6Interface>, Ipv6InterfaceAddress> removeAddressCallback)
{
    NS_LOG_FUNCTION(this << &removeAddressCallback);
    m_removeAddressCallback = removeAddressCallback;
}

void
Ipv6Interface::AddAddressCallback(
    Callback<void, Ptr<Ipv6Interface>, Ipv6InterfaceAddress> addAddressCallback)
{
    NS_LOG_FUNCTION(this << &addAddressCallback);
    m_addAddressCallback = addAddressCallback;
}

} /* namespace ns3 */

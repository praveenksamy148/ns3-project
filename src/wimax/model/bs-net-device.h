/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *          Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#ifndef WIMAX_BS_NET_DEVICE_H
#define WIMAX_BS_NET_DEVICE_H

#include "bs-service-flow-manager.h"
#include "dl-mac-messages.h"
#include "ipcs-classifier.h"
#include "wimax-connection.h"
#include "wimax-net-device.h"

#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"

namespace ns3
{

class Node;
class Packet;
class SSRecord;
class SSManager;
class BSScheduler;
class BurstProfileManager;
class BSLinkManager;
class UplinkScheduler;
class BsServiceFlowManager;

/**
 * @ingroup wimax
 *
 * BaseStation NetDevice
 */
class BaseStationNetDevice : public WimaxNetDevice
{
  public:
    /// State enumeration
    enum State
    {
        BS_STATE_DL_SUB_FRAME,
        BS_STATE_UL_SUB_FRAME,
        BS_STATE_TTG,
        BS_STATE_RTG
    };

    /// MacPreamble enumeration
    enum MacPreamble
    {
        SHORT_PREAMBLE = 1,
        LONG_PREAMBLE
    };

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    BaseStationNetDevice();
    /**
     * Constructor
     *
     * @param node the node
     * @param phy the wimax phy
     */
    BaseStationNetDevice(Ptr<Node> node, Ptr<WimaxPhy> phy);
    /**
     * Constructor
     *
     * @param node the node
     * @param phy the wimax phy
     * @param uplinkScheduler uplink scheduler
     * @param bsScheduler bs scheduler
     */
    BaseStationNetDevice(Ptr<Node> node,
                         Ptr<WimaxPhy> phy,
                         Ptr<UplinkScheduler> uplinkScheduler,
                         Ptr<BSScheduler> bsScheduler);
    ~BaseStationNetDevice() override;
    /**
     * @param initialRangInterval Time between initial ranging regions assigned by the BS
     */
    void SetInitialRangingInterval(Time initialRangInterval);
    /**
     * @brief initializes the BS net device and sets its parameters to the default values
     */
    void InitBaseStationNetDevice();
    /**
     * @returns Time between initial ranging regions assigned by the BS
     */
    Time GetInitialRangingInterval() const;
    /**
     * @param dcdInterval Time between transmission of DCD messages
     */
    void SetDcdInterval(Time dcdInterval);
    /**
     * @returns the Time between transmission of DCD messages
     */
    Time GetDcdInterval() const;
    /**
     * @param ucdInterval the Time between transmission of UCD messages
     */
    void SetUcdInterval(Time ucdInterval);
    /**
     * @returns Time between transmission of UCD messages
     */
    Time GetUcdInterval() const;
    /**
     * @param interval the Wait for DSA/DSC Acknowledge timeout
     */
    void SetIntervalT8(Time interval);
    /**
     * @returns the Wait for DSA/DSC Acknowledge timeout
     */
    Time GetIntervalT8() const;
    /**
     * @param maxRangCorrectionRetries the number of retries on contention Ranging Requests
     */
    void SetMaxRangingCorrectionRetries(uint8_t maxRangCorrectionRetries);
    /**
     * @returns the number of retries on contention Ranging Requests
     */
    uint8_t GetMaxRangingCorrectionRetries() const;
    /**
     * @param maxInvitedRangRetries the number of retries on contention Ranging
     */
    void SetMaxInvitedRangRetries(uint8_t maxInvitedRangRetries);
    /**
     * @returns the number of retries on contention Ranging
     */
    uint8_t GetMaxInvitedRangRetries() const;
    /**
     * @param rangReqOppSize The ranging opportunity size in symbols
     */
    void SetRangReqOppSize(uint8_t rangReqOppSize);
    /**
     * @returns The ranging opportunity size in symbols
     */
    uint8_t GetRangReqOppSize() const;
    /**
     * @param bwReqOppSize The bandwidth request opportunity size in symbols
     */
    void SetBwReqOppSize(uint8_t bwReqOppSize);
    /**
     * @returns The bandwidth request opportunity size in symbols
     */
    uint8_t GetBwReqOppSize() const;
    /**
     * @param dlSymbols the number of symbols in the downlink sub-frame
     */
    void SetNrDlSymbols(uint32_t dlSymbols);
    /**
     * @returns the number of symbols in the downlink sub-frame
     */
    uint32_t GetNrDlSymbols() const;
    /**
     * @param ulSymbols the number of symbols in the uplink sub-frame
     */
    void SetNrUlSymbols(uint32_t ulSymbols);
    /**
     * @returns the number of symbols in the uplink sub-frame
     */
    uint32_t GetNrUlSymbols() const;
    /**
     * @returns the number dcd messages already sent
     */
    uint32_t GetNrDcdSent() const;
    /**
     * @returns the number ucd messages already sent
     */
    uint32_t GetNrUcdSent() const;
    /**
     * @returns the DL subframe start time
     */
    Time GetDlSubframeStartTime() const;
    /**
     * @returns the UL subframe start time
     */
    Time GetUlSubframeStartTime() const;
    /**
     * @returns the ranging opp number
     */
    uint8_t GetRangingOppNumber() const;
    /**
     * @returns a pointer to the SS manager
     */
    Ptr<SSManager> GetSSManager() const;
    /**
     * @param ssManager the SS manager to be installed on the BS
     */
    void SetSSManager(Ptr<SSManager> ssManager);
    /**
     * @returns a pointer to the uplink scheduler installed on the device
     */
    Ptr<UplinkScheduler> GetUplinkScheduler() const;
    /**
     * @param ulScheduler the ulScheduler to be isnstalled on the BS
     */
    void SetUplinkScheduler(Ptr<UplinkScheduler> ulScheduler);
    /**
     * @returns a pointer to the link manager installed on the BS
     */
    Ptr<BSLinkManager> GetLinkManager() const;
    /**
     * @param bsSchedule the downlink scheduler to be installed on the BS
     */
    void SetBSScheduler(Ptr<BSScheduler> bsSchedule);
    /**
     * @returns The BS scheduler installed on the BS
     */
    Ptr<BSScheduler> GetBSScheduler() const;
    /**
     * @param linkManager The link manager installed on the BS
     */
    void SetLinkManager(Ptr<BSLinkManager> linkManager);
    /**
     * @returns a pointer to the classifier installed on the BS
     */
    Ptr<IpcsClassifier> GetBsClassifier() const;
    /**
     * @param classifier a classifier to be installed on the BS
     */
    void SetBsClassifier(Ptr<IpcsClassifier> classifier);

    /**
     * @returns PS duration
     */
    Time GetPsDuration() const;
    /**
     * @returns symbol duration
     */
    Time GetSymbolDuration() const;
    /**
     * @brief Start device
     */
    void Start() override;
    /**
     * @brief Stop device
     */
    void Stop() override;
    /**
     * @brief Enqueue a packet into a connection queue
     * @param packet the packet to be enqueued
     * @param hdrType the mac header type to be appended to the packet
     * @param connection the connection to be used
     * @return true if successful
     */
    bool Enqueue(Ptr<Packet> packet,
                 const MacHeaderType& hdrType,
                 Ptr<WimaxConnection> connection) override;
    /**
     * @param cid connection ID
     * @returns a pointer to the connection
     */
    Ptr<WimaxConnection> GetConnection(Cid cid);

    /**
     * @brief Mark uplink allocations
     */
    void MarkUplinkAllocations();
    /**
     * @brief Mark ranging opp start
     * @param rangingOppStartTime start time
     */
    void MarkRangingOppStart(Time rangingOppStartTime);
    /**
     * @returns service flow manager
     */
    Ptr<BsServiceFlowManager> GetServiceFlowManager() const;
    /**
     * @brief Set service flow manager
     * @param sfm the service flow manager
     */
    void SetServiceFlowManager(Ptr<BsServiceFlowManager> sfm);

  private:
    void DoDispose() override;
    /**
     * @brief Start frame function
     */
    void StartFrame();
    /**
     * @brief Start DL subframe function
     */
    void StartDlSubFrame();
    /**
     * @brief End DL subframe function
     */
    void EndDlSubFrame();
    /**
     * @brief Start UL subframe function
     */
    void StartUlSubFrame();
    /**
     * @brief End UL subframe function
     */
    void EndUlSubFrame();
    /**
     * @brief End frame function
     */
    void EndFrame();
    /**
     * @brief Send packet
     * @param packet to send
     * @param source MAC address
     * @param dest destination MAC address
     * @param protocolNumber protocol number
     * @returns true if successful
     */
    bool DoSend(Ptr<Packet> packet,
                const Mac48Address& source,
                const Mac48Address& dest,
                uint16_t protocolNumber) override;
    /**
     * @brief Receive packet
     * @param packet to send
     */
    void DoReceive(Ptr<Packet> packet) override;
    /**
     * @brief creates the MAC management messages DL-MAP and UL-MAP
     */
    void CreateMapMessages();
    /**
     * @brief creates the channel descriptor MAC management messages DCD and UCD
     * @param sendDcd true if send DCD
     * @param sendUcd true if send UCD
     */
    void CreateDescriptorMessages(bool sendDcd, bool sendUcd);
    /**
     * @brief Send burst function
     */
    void SendBursts();

    /**
     * @brief Create DL map
     * @returns packet
     */
    Ptr<Packet> CreateDlMap();
    /**
     * @brief Create DCD
     * @returns packet
     */
    Ptr<Packet> CreateDcd();
    /**
     * @brief Create UL map
     * @returns packet
     */
    Ptr<Packet> CreateUlMap();
    /**
     * @brief Create UCD
     * @returns packet
     */
    Ptr<Packet> CreateUcd();
    /**
     * @brief Send DL burst profiles
     * @param dcd burst profile
     */
    void SetDlBurstProfiles(Dcd* dcd);
    /**
     * @brief Send UL burst profiles
     * @param ucd burst profile
     */
    void SetUlBurstProfiles(Ucd* ucd);

    /**
     * @brief Mark uplink allocation start
     * @param allocationStartTime allocation start time
     */
    void MarkUplinkAllocationStart(Time allocationStartTime);
    /**
     * @brief Mark uplink allocation end
     * @param allocationEndTime allocation end time
     * @param cid connection ID
     * @param uiuc
     */
    void MarkUplinkAllocationEnd(Time allocationEndTime, Cid cid, uint8_t uiuc);
    /**
     * @brief Uplink allocation start
     */
    void UplinkAllocationStart();
    /**
     * @brief Uplink allocation end
     * @param cid connection ID
     * @param uiuc
     */
    void UplinkAllocationEnd(Cid cid, uint8_t uiuc);
    /**
     * @brief Ranging opp start
     */
    void RangingOppStart();

    // parameters defined in Table 342
    Time m_initialRangInterval; ///< in seconds
    Time m_dcdInterval;         ///< in seconds
    Time m_ucdInterval;         ///< in seconds
    Time m_intervalT8;          ///< in milliseconds, wait for DSA/DSC Acknowledge timeout

    uint8_t m_maxRangCorrectionRetries; ///< maximum range correction retries
    uint8_t m_maxInvitedRangRetries;    ///< maximum invited range retries
    uint8_t m_rangReqOppSize;           ///< in symbols
    uint8_t m_bwReqOppSize;             ///< in symbols

    uint32_t m_nrDlSymbols; ///< number of DL symbols
    uint32_t m_nrUlSymbols; ///< number of UL symbols

    // to keep track total number of a certain management messages sent by the BS
    uint32_t m_nrDlMapSent; ///< number DL map sent
    uint32_t m_nrUlMapSent; ///< number UL map sent
    // number of DCDs and UCDs sent even if same
    uint32_t m_nrDcdSent; ///< number DCD sent
    uint32_t m_nrUcdSent; ///< number UCD sent

    uint32_t m_dcdConfigChangeCount; ///< DCD config change count
    uint32_t m_ucdConfigChangeCount; ///< UCD config change count

    uint32_t m_framesSinceLastDcd; ///< frames since last DCD
    uint32_t m_framesSinceLastUcd; ///< frames since last UCD

    // uint32_t m_nrFrames; //temporarily defined in wimax-net-device, as static
    uint32_t m_nrDlFrames; ///< number DL frames
    uint32_t m_nrUlFrames; ///< number UL frames

    // to keep track if number of SSs have changed since the last frame
    uint16_t m_nrSsRegistered; ///< number SS registered

    uint16_t m_nrDlAllocations; ///< number DL allocations
    uint16_t m_nrUlAllocations; ///< number UL allocations

    Time m_dlSubframeStartTime; ///< DL subframe start time
    Time m_ulSubframeStartTime; ///< UL subframe start time

    uint8_t m_ulAllocationNumber; ///< to see UL burst number
    uint8_t m_rangingOppNumber;   ///< current ranging TO number

    CidFactory* m_cidFactory; ///< the CID factory

    uint32_t m_allocationStartTime; ///< allocation start time

    Ptr<SSManager> m_ssManager;                     ///< the SS manager
    Ptr<UplinkScheduler> m_uplinkScheduler;         ///< the uplink scheduler
    Ptr<BSScheduler> m_scheduler;                   ///< the base station scheduler
    Ptr<BSLinkManager> m_linkManager;               ///< the link manager
    Ptr<IpcsClassifier> m_bsClassifier;             ///< the base station classifier
    Ptr<BsServiceFlowManager> m_serviceFlowManager; ///< the service flow manager
    // same fields as in PHY, for quick access
    Time m_psDuration;     ///< ps duration
    Time m_symbolDuration; ///< symbol duration

    TracedCallback<Ptr<const Packet>, Mac48Address, Cid>
        m_traceBSRx; ///< the base station receive trace callback

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, before being queued for transmission.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_bsTxTrace; ///< the base station transmit trace callback

    /**
     * The trace source fired when packets coming into the "top" of the device
     * are dropped at the MAC layer during transmission.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>>
        m_bsTxDropTrace; ///< base station transmit drop trace callback

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a promiscuous trace.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>>
        m_bsPromiscRxTrace; ///< base station promiscuous receive trace callback

    /**
     * The trace source fired for packets successfully received by the device
     * immediately before being forwarded up to higher layers (at the L2/L3
     * transition).  This is a non- promiscuous trace.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_bsRxTrace; ///< base station receive trace callback

    /**
     * The trace source fired when packets coming into the "top" of the device
     * are dropped at the MAC layer during reception.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_bsRxDropTrace; ///< base station receive drop trace callback
};

} // namespace ns3

#endif /* WIMAX_BS_NET_DEVICE_H */

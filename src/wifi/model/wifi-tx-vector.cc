/*
 * Copyright (c) 2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Ghada Badawy <gbadawy@gmail.com>
 */

#include "wifi-tx-vector.h"

#include "wifi-phy-common.h"
#include "wifi-utils.h"

#include "ns3/abort.h"
#include "ns3/eht-phy.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <set>

namespace ns3
{

WifiTxVector::WifiTxVector()
    : m_txPowerLevel(1),
      m_preamble(WIFI_PREAMBLE_LONG),
      m_channelWidth(MHz_u{20}),
      m_guardInterval(NanoSeconds(800)),
      m_nTx(1),
      m_nss(1),
      m_ness(0),
      m_aggregation(false),
      m_stbc(false),
      m_ldpc(false),
      m_bssColor(0),
      m_length(0),
      m_triggerResponding(false),
      m_modeInitialized(false),
      m_inactiveSubchannels(),
      m_ruAllocation(),
      m_center26ToneRuIndication(std::nullopt),
      m_ehtPpduType(1) // SU transmission by default
{
}

WifiTxVector::WifiTxVector(WifiMode mode,
                           uint8_t powerLevel,
                           WifiPreamble preamble,
                           Time guardInterval,
                           uint8_t nTx,
                           uint8_t nss,
                           uint8_t ness,
                           MHz_u channelWidth,
                           bool aggregation,
                           bool stbc,
                           bool ldpc,
                           uint8_t bssColor,
                           uint16_t length,
                           bool triggerResponding)
    : m_mode(mode),
      m_txPowerLevel(powerLevel),
      m_preamble(preamble),
      m_channelWidth(channelWidth),
      m_guardInterval(guardInterval),
      m_nTx(nTx),
      m_nss(nss),
      m_ness(ness),
      m_aggregation(aggregation),
      m_stbc(stbc),
      m_ldpc(ldpc),
      m_bssColor(bssColor),
      m_length(length),
      m_triggerResponding(triggerResponding),
      m_modeInitialized(true),
      m_inactiveSubchannels(),
      m_sigBMcs(EhtPhy::GetVhtMcs(0)),
      m_ruAllocation(),
      m_center26ToneRuIndication(std::nullopt),
      m_ehtPpduType(1) // SU transmission by default
{
}

WifiTxVector::WifiTxVector(const WifiTxVector& txVector)
    : m_mode(txVector.m_mode),
      m_txPowerLevel(txVector.m_txPowerLevel),
      m_preamble(txVector.m_preamble),
      m_channelWidth(txVector.m_channelWidth),
      m_guardInterval(txVector.m_guardInterval),
      m_nTx(txVector.m_nTx),
      m_nss(txVector.m_nss),
      m_ness(txVector.m_ness),
      m_aggregation(txVector.m_aggregation),
      m_stbc(txVector.m_stbc),
      m_ldpc(txVector.m_ldpc),
      m_bssColor(txVector.m_bssColor),
      m_length(txVector.m_length),
      m_triggerResponding(txVector.m_triggerResponding),
      m_modeInitialized(txVector.m_modeInitialized),
      m_inactiveSubchannels(txVector.m_inactiveSubchannels),
      m_sigBMcs(txVector.m_sigBMcs),
      m_ruAllocation(txVector.m_ruAllocation),
      m_center26ToneRuIndication(txVector.m_center26ToneRuIndication),
      m_ehtPpduType(txVector.m_ehtPpduType)
{
    m_muUserInfos.clear();
    if (!txVector.m_muUserInfos.empty()) // avoids crashing for loop
    {
        for (auto& info : txVector.m_muUserInfos)
        {
            m_muUserInfos.insert(std::make_pair(info.first, info.second));
        }
    }
}

bool
WifiTxVector::GetModeInitialized() const
{
    return m_modeInitialized;
}

WifiMode
WifiTxVector::GetMode(uint16_t staId) const
{
    if (!m_modeInitialized)
    {
        NS_FATAL_ERROR("WifiTxVector mode must be set before using");
    }
    if (!IsMu())
    {
        return m_mode;
    }
    NS_ABORT_MSG_IF(staId > 2048, "STA-ID should be correctly set for MU (" << staId << ")");
    const auto userInfoIt = m_muUserInfos.find(staId);
    NS_ASSERT(userInfoIt != m_muUserInfos.cend());
    switch (GetModulationClassForPreamble(m_preamble))
    {
    case WIFI_MOD_CLASS_EHT:
        return EhtPhy::GetEhtMcs(userInfoIt->second.mcs);
    case WIFI_MOD_CLASS_HE:
        return HePhy::GetHeMcs(userInfoIt->second.mcs);
    default:
        NS_ABORT_MSG("Unsupported modulation class: " << GetModulationClassForPreamble(m_preamble));
    }
    return WifiMode(); // invalid WifiMode
}

WifiModulationClass
WifiTxVector::GetModulationClass() const
{
    NS_ABORT_MSG_IF(!m_modeInitialized, "WifiTxVector mode must be set before using");

    if (IsMu())
    {
        NS_ASSERT(!m_muUserInfos.empty());
        // all the modes belong to the same modulation class
        return GetModulationClassForPreamble(m_preamble);
    }
    return m_mode.GetModulationClass();
}

uint8_t
WifiTxVector::GetTxPowerLevel() const
{
    return m_txPowerLevel;
}

WifiPreamble
WifiTxVector::GetPreambleType() const
{
    return m_preamble;
}

MHz_u
WifiTxVector::GetChannelWidth() const
{
    return m_channelWidth;
}

Time
WifiTxVector::GetGuardInterval() const
{
    return m_guardInterval;
}

uint8_t
WifiTxVector::GetNTx() const
{
    return m_nTx;
}

uint8_t
WifiTxVector::GetNss(uint16_t staId) const
{
    if (IsMu())
    {
        NS_ABORT_MSG_IF(staId > 2048, "STA-ID should be correctly set for MU (" << staId << ")");
        NS_ASSERT(m_muUserInfos.contains(staId));
        return m_muUserInfos.at(staId).nss;
    }
    return m_nss;
}

uint8_t
WifiTxVector::GetNssMax() const
{
    // We do not support mixed OFDMA and MU-MIMO
    uint8_t nss = 0;
    if (IsMu())
    {
        for (const auto& info : m_muUserInfos)
        {
            nss = (nss < info.second.nss) ? info.second.nss : nss;
        }
    }
    else
    {
        nss = m_nss;
    }
    return nss;
}

uint8_t
WifiTxVector::GetNssTotal() const
{
    // We do not support mixed OFDMA and MU-MIMO
    uint8_t nss = 0;
    if (IsMu())
    {
        nss = std::accumulate(
            m_muUserInfos.cbegin(),
            m_muUserInfos.cend(),
            0,
            [](uint8_t prevNss, const auto& info) { return prevNss + info.second.nss; });
    }
    else
    {
        nss = m_nss;
    }
    return nss;
}

uint8_t
WifiTxVector::GetNess() const
{
    return m_ness;
}

bool
WifiTxVector::IsAggregation() const
{
    return m_aggregation;
}

bool
WifiTxVector::IsStbc() const
{
    return m_stbc;
}

bool
WifiTxVector::IsLdpc() const
{
    return m_ldpc;
}

bool
WifiTxVector::IsNonHtDuplicate() const
{
    return ((m_channelWidth >= MHz_u{40}) && !IsMu() && (GetModulationClass() < WIFI_MOD_CLASS_HT));
}

void
WifiTxVector::SetMode(WifiMode mode)
{
    m_mode = mode;
    m_modeInitialized = true;
}

void
WifiTxVector::SetMode(WifiMode mode, uint16_t staId)
{
    NS_ABORT_MSG_IF(!IsMu(), "Not a MU transmission");
    NS_ABORT_MSG_IF(staId > 2048, "STA-ID should be correctly set for MU");
    NS_ASSERT_MSG(m_muUserInfos.empty() || (mode.GetModulationClass() == GetModulationClass()),
                  "Cannot add mode " << mode << " because the modulation class is "
                                     << GetModulationClass());
    m_muUserInfos[staId].mcs = mode.GetMcsValue();
    m_modeInitialized = true;
}

void
WifiTxVector::SetTxPowerLevel(uint8_t powerlevel)
{
    m_txPowerLevel = powerlevel;
}

void
WifiTxVector::SetPreambleType(WifiPreamble preamble)
{
    m_preamble = preamble;
}

void
WifiTxVector::SetChannelWidth(MHz_u channelWidth)
{
    m_channelWidth = channelWidth;
}

void
WifiTxVector::SetGuardInterval(Time guardInterval)
{
    m_guardInterval = guardInterval;
}

void
WifiTxVector::SetNTx(uint8_t nTx)
{
    m_nTx = nTx;
}

void
WifiTxVector::SetNss(uint8_t nss)
{
    m_nss = nss;
}

void
WifiTxVector::SetNss(uint8_t nss, uint16_t staId)
{
    NS_ABORT_MSG_IF(!IsMu(), "Not a MU transmission");
    NS_ABORT_MSG_IF(staId > 2048, "STA-ID should be correctly set for MU");
    m_muUserInfos[staId].nss = nss;
}

void
WifiTxVector::SetNess(uint8_t ness)
{
    m_ness = ness;
}

void
WifiTxVector::SetAggregation(bool aggregation)
{
    m_aggregation = aggregation;
}

void
WifiTxVector::SetStbc(bool stbc)
{
    m_stbc = stbc;
}

void
WifiTxVector::SetLdpc(bool ldpc)
{
    m_ldpc = ldpc;
}

void
WifiTxVector::SetBssColor(uint8_t color)
{
    m_bssColor = color;
}

uint8_t
WifiTxVector::GetBssColor() const
{
    return m_bssColor;
}

void
WifiTxVector::SetLength(uint16_t length)
{
    m_length = length;
}

uint16_t
WifiTxVector::GetLength() const
{
    return m_length;
}

bool
WifiTxVector::IsTriggerResponding() const
{
    return m_triggerResponding;
}

void
WifiTxVector::SetTriggerResponding(bool triggerResponding)
{
    m_triggerResponding = triggerResponding;
}

void
WifiTxVector::SetSigBMode(const WifiMode& mode)
{
    m_sigBMcs = mode;
}

WifiMode
WifiTxVector::GetSigBMode() const
{
    return m_sigBMcs;
}

void
WifiTxVector::SetRuAllocation(const RuAllocation& ruAlloc, uint8_t p20Index)
{
    if (ns3::IsDlMu(m_preamble) && !m_muUserInfos.empty())
    {
        NS_ASSERT(ruAlloc == DeriveRuAllocation(p20Index));
    }
    m_ruAllocation = ruAlloc;
}

const RuAllocation&
WifiTxVector::GetRuAllocation(uint8_t p20Index) const
{
    if (ns3::IsDlMu(m_preamble) && m_ruAllocation.empty())
    {
        m_ruAllocation = DeriveRuAllocation(p20Index);
    }
    return m_ruAllocation;
}

void
WifiTxVector::SetEhtPpduType(uint8_t type)
{
    NS_ASSERT(IsEht(m_preamble));
    m_ehtPpduType = type;
}

uint8_t
WifiTxVector::GetEhtPpduType() const
{
    return m_ehtPpduType;
}

bool
WifiTxVector::IsValid(WifiPhyBand band) const
{
    if (!GetModeInitialized())
    {
        return false;
    }
    const auto& modeName = m_mode.GetUniqueName();
    if (m_channelWidth == MHz_u{20})
    {
        if (m_nss != 3 && m_nss != 6)
        {
            if (modeName == "VhtMcs9")
            {
                return false;
            }
        }
    }
    else if (m_channelWidth == MHz_u{80})
    {
        if (m_nss == 3 || m_nss == 7)
        {
            if (modeName == "VhtMcs6")
            {
                return false;
            }
        }
        else if (m_nss == 6)
        {
            if (modeName == "VhtMcs9")
            {
                return false;
            }
        }
    }
    else if (m_channelWidth == MHz_u{160})
    {
        if (m_nss == 3)
        {
            if (modeName == "VhtMcs9")
            {
                return false;
            }
        }
    }
    for (const auto& userInfo : m_muUserInfos)
    {
        if (GetNumStasInRu(userInfo.second.ru) > 8)
        {
            return false;
        }
    }
    std::map<WifiRu::RuSpec, uint8_t> streamsPerRu{};
    for (const auto& info : m_muUserInfos)
    {
        auto it = streamsPerRu.find(info.second.ru);
        if (it == streamsPerRu.end())
        {
            streamsPerRu[info.second.ru] = info.second.nss;
        }
        else
        {
            it->second += info.second.nss;
        }
    }
    for (auto& streams : streamsPerRu)
    {
        if (streams.second > 8)
        {
            return false;
        }
    }

    if (band != WIFI_PHY_BAND_UNSPECIFIED)
    {
        NS_ABORT_MSG_IF(GetModulationClass() == WIFI_MOD_CLASS_OFDM && band == WIFI_PHY_BAND_2_4GHZ,
                        "Cannot use OFDM modulation class in the 2.4 GHz band");
        NS_ABORT_MSG_IF(GetModulationClass() == WIFI_MOD_CLASS_ERP_OFDM &&
                            band != WIFI_PHY_BAND_2_4GHZ,
                        "ERP-OFDM modulation class can only be used in the 2.4 GHz band");
    }

    return true;
}

bool
WifiTxVector::IsMu() const
{
    return IsDlMu() || IsUlMu();
}

bool
WifiTxVector::IsDlMu() const
{
    return ns3::IsDlMu(m_preamble) && !(IsEht(m_preamble) && m_ehtPpduType == 1);
}

bool
WifiTxVector::IsUlMu() const
{
    return ns3::IsUlMu(m_preamble);
}

bool
WifiTxVector::IsDlOfdma() const
{
    if (!IsDlMu())
    {
        return false;
    }
    if (IsEht(m_preamble))
    {
        return m_ehtPpduType == 0;
    }
    if (m_muUserInfos.size() == 1)
    {
        return true;
    }
    std::set<WifiRu::RuSpec> rus{};
    for (const auto& userInfo : m_muUserInfos)
    {
        rus.insert(userInfo.second.ru);
        if (rus.size() > 1)
        {
            return true;
        }
    }
    return false;
}

bool
WifiTxVector::IsDlMuMimo() const
{
    if (!IsDlMu())
    {
        return false;
    }
    if (IsEht(m_preamble))
    {
        return m_ehtPpduType == 2;
    }
    if (m_muUserInfos.size() < 2)
    {
        return false;
    }
    // TODO: mixed OFDMA and MU-MIMO is not supported
    return !IsDlOfdma();
}

uint8_t
WifiTxVector::GetNumStasInRu(const WifiRu::RuSpec& ru) const
{
    return std::count_if(m_muUserInfos.cbegin(),
                         m_muUserInfos.cend(),
                         [&ru](const auto& info) -> bool { return (ru == info.second.ru); });
}

WifiRu::RuSpec
WifiTxVector::GetRu(uint16_t staId) const
{
    NS_ABORT_MSG_IF(!IsMu(), "RU only available for MU");
    NS_ABORT_MSG_IF(staId > 2048, "STA-ID should be correctly set for MU");
    return m_muUserInfos.at(staId).ru;
}

void
WifiTxVector::SetRu(WifiRu::RuSpec ru, uint16_t staId)
{
    NS_ABORT_MSG_IF(!IsMu(), "RU only available for MU");
    NS_ABORT_MSG_IF(staId > 2048, "STA-ID should be correctly set for MU");
    m_muUserInfos[staId].ru = ru;
}

HeMuUserInfo
WifiTxVector::GetHeMuUserInfo(uint16_t staId) const
{
    NS_ABORT_MSG_IF(!IsMu(), "HE MU user info only available for MU");
    return m_muUserInfos.at(staId);
}

void
WifiTxVector::SetHeMuUserInfo(uint16_t staId, HeMuUserInfo userInfo)
{
    NS_ABORT_MSG_IF(!IsMu(), "HE MU user info only available for MU");
    NS_ABORT_MSG_IF(staId > 2048, "STA-ID should be correctly set for MU");
    m_muUserInfos[staId] = userInfo;
    m_modeInitialized = true;
    m_ruAllocation.clear();
}

const WifiTxVector::HeMuUserInfoMap&
WifiTxVector::GetHeMuUserInfoMap() const
{
    NS_ABORT_MSG_IF(!IsMu(), "HE MU user info map only available for MU");
    return m_muUserInfos;
}

WifiTxVector::HeMuUserInfoMap&
WifiTxVector::GetHeMuUserInfoMap()
{
    NS_ABORT_MSG_IF(!IsMu(), "HE MU user info map only available for MU");
    m_ruAllocation.clear();
    return m_muUserInfos;
}

bool
WifiTxVector::IsSigBCompression() const
{
    // SIG-B compression is used in case of full-bandwidth MU-MIMO transmission (27.3.11.8.2
    // HE-SIG-B content channels in IEEE802.11ax-2021) or if a single RU occupies the whole 160 MHz
    // bandwidth (27.3.11.8.3 Common field in IEEE802.11ax-2021)
    return (IsDlMuMimo() && !IsDlOfdma()) ||
           ((m_muUserInfos.size() == 1) && (m_channelWidth >= MHz_u{160}) &&
            (WifiRu::GetRuType(m_muUserInfos.cbegin()->second.ru) ==
             WifiRu::GetRuType(m_channelWidth)));
}

void
WifiTxVector::SetInactiveSubchannels(const std::vector<bool>& inactiveSubchannels)
{
    NS_ABORT_MSG_IF(m_preamble < WIFI_PREAMBLE_HE_SU,
                    "Only HE (or later) authorized for preamble puncturing");
    NS_ABORT_MSG_IF(
        m_channelWidth < MHz_u{80},
        "Preamble puncturing only possible for transmission bandwidth of 80 MHz or larger");
    NS_ABORT_MSG_IF(!inactiveSubchannels.empty() &&
                        inactiveSubchannels.size() != Count20MHzSubchannels(m_channelWidth),
                    "The size of the inactive subchannnels bitmap should be equal to the number of "
                    "20 MHz subchannels");
    m_inactiveSubchannels = inactiveSubchannels;
}

const std::vector<bool>&
WifiTxVector::GetInactiveSubchannels() const
{
    return m_inactiveSubchannels;
}

void
WifiTxVector::SetCenter26ToneRuIndication(Center26ToneRuIndication center26ToneRuIndication)
{
    if (IsDlMu())
    {
        NS_ASSERT(center26ToneRuIndication == DeriveCenter26ToneRuIndication());
    }
    m_center26ToneRuIndication = center26ToneRuIndication;
}

std::optional<Center26ToneRuIndication>
WifiTxVector::GetCenter26ToneRuIndication() const
{
    if (!IsDlMu() || (m_channelWidth < MHz_u{80}))
    {
        return std::nullopt;
    }
    if (!m_center26ToneRuIndication.has_value())
    {
        m_center26ToneRuIndication.emplace(DeriveCenter26ToneRuIndication());
    }
    return m_center26ToneRuIndication;
}

std::ostream&
operator<<(std::ostream& os, const WifiTxVector& v)
{
    if (!v.IsValid())
    {
        os << "TXVECTOR not valid";
        return os;
    }
    os << "txpwrlvl: " << +v.GetTxPowerLevel() << " preamble: " << v.GetPreambleType()
       << " channel width: " << v.GetChannelWidth() << " GI: " << v.GetGuardInterval()
       << " NTx: " << +v.GetNTx() << " Ness: " << +v.GetNess()
       << " MPDU aggregation: " << v.IsAggregation() << " STBC: " << v.IsStbc()
       << " FEC coding: " << (v.IsLdpc() ? "LDPC" : "BCC");
    if (v.GetPreambleType() >= WIFI_PREAMBLE_HE_SU)
    {
        os << " BSS color: " << +v.GetBssColor();
    }
    if (v.IsUlMu())
    {
        os << " Length: " << v.GetLength();
    }
    if (ns3::IsDlMu(v.GetPreambleType()))
    {
        os << " SIG-B mode: " << v.GetSigBMode();
    }
    if (v.IsMu())
    {
        WifiTxVector::HeMuUserInfoMap userInfoMap = v.GetHeMuUserInfoMap();
        os << " num User Infos: " << userInfoMap.size();
        for (auto& ui : userInfoMap)
        {
            os << ", {STA-ID: " << ui.first << ", " << ui.second.ru << ", MCS: " << +ui.second.mcs
               << ", Nss: " << +ui.second.nss << "}";
        }
    }
    else
    {
        os << " mode: " << v.GetMode() << " Nss: " << +v.GetNss();
    }
    const auto& puncturedSubchannels = v.GetInactiveSubchannels();
    if (!puncturedSubchannels.empty())
    {
        os << " Punctured subchannels: ";
        std::copy(puncturedSubchannels.cbegin(),
                  puncturedSubchannels.cend(),
                  std::ostream_iterator<bool>(os, ", "));
    }
    if (IsEht(v.GetPreambleType()))
    {
        os << " EHT PPDU type: " << +v.GetEhtPpduType();
    }
    return os;
}

bool
HeMuUserInfo::operator==(const HeMuUserInfo& other) const
{
    return ru == other.ru && mcs == other.mcs && nss == other.nss;
}

bool
HeMuUserInfo::operator!=(const HeMuUserInfo& other) const
{
    return !(*this == other);
}

WifiTxVector::UserInfoMapOrderedByRus
WifiTxVector::GetUserInfoMapOrderedByRus(uint8_t p20Index) const
{
    auto heRuComparator = WifiRu::RuSpecCompare(m_channelWidth, p20Index);
    UserInfoMapOrderedByRus orderedMap{heRuComparator};
    for (const auto& userInfo : m_muUserInfos)
    {
        const auto ru = userInfo.second.ru;
        if (auto it = orderedMap.find(ru); it != orderedMap.end())
        {
            it->second.emplace(userInfo.first);
        }
        else
        {
            orderedMap.emplace(ru, std::set<uint16_t>{userInfo.first});
        }
    }
    return orderedMap;
}

RuAllocation
WifiTxVector::DeriveRuAllocation(uint8_t p20Index) const
{
    RuAllocation ruAllocations(Count20MHzSubchannels(m_channelWidth), HeRu::EMPTY_242_TONE_RU);
    std::vector<RuType> ruTypes{};
    ruTypes.resize(ruAllocations.size());
    const auto& orderedMap = GetUserInfoMapOrderedByRus(p20Index);
    std::pair<std::size_t /* number of RUs in content channel 1 */,
              std::size_t /* number of RUs in content channel 2 */>
        ccSizes{0, 0};
    for (const auto& [ru, staIds] : orderedMap)
    {
        const auto ruType = WifiRu::GetRuType(ru);
        auto ruIndex = WifiRu::GetPhyIndex(ru, m_channelWidth, p20Index);
        if ((ruType == RuType::RU_26_TONE) && (ruIndex == 19))
        {
            continue;
        }
        const auto ruBw = WifiRu::GetBandwidth(ruType);
        NS_ASSERT_MSG(WifiRu::IsHe(ru), "EHT RUs should not be used yet");
        const auto mc{WIFI_MOD_CLASS_HE};
        const auto rusPerSubchannel =
            WifiRu::GetRusOfType(ruBw > MHz_u{20} ? ruBw : MHz_u{20}, ruType, mc);
        if ((m_channelWidth >= MHz_u{80}) && (ruIndex > 19))
        {
            // "ignore" the center 26-tone RUs in 80 MHz channels
            ruIndex--;
            if (ruIndex > 37)
            {
                ruIndex -= (ruIndex - 19) / 37;
            }
        }
        const auto numSubchannelsForRu = (ruBw < MHz_u{20}) ? 1 : Count20MHzSubchannels(ruBw);
        const auto index = (ruBw < MHz_u{20}) ? ((ruIndex - 1) / rusPerSubchannel.size())
                                              : ((ruIndex - 1) * numSubchannelsForRu);
        NS_ABORT_IF(index >= Count20MHzSubchannels(m_channelWidth));
        auto ruAlloc = WifiRu::GetEqualizedRuAllocation(ruType, false, true, mc);
        if (ruAllocations.at(index) != HeRu::EMPTY_242_TONE_RU)
        {
            if (ruType == ruTypes.at(index))
            {
                continue;
            }
            if (ruType == RuType::RU_26_TONE)
            {
                ruAlloc = WifiRu::GetEqualizedRuAllocation(ruTypes.at(index), true, true, mc);
            }
            else if (ruTypes.at(index) == RuType::RU_26_TONE)
            {
                ruAlloc = WifiRu::GetEqualizedRuAllocation(ruType, true, true, mc);
            }
            else
            {
                NS_ASSERT_MSG(false, "unsupported RU combination");
            }
        }
        std::size_t ccIndex;
        if (ruType >= RuType::RU_484_TONE)
        {
            ccIndex = (ccSizes.first <= ccSizes.second) ? 0 : 1;
        }
        else
        {
            ccIndex = (index % 2 == 0) ? 0 : 1;
        }
        if (ccIndex == 0)
        {
            ccSizes.first += staIds.size();
        }
        else
        {
            ccSizes.second += staIds.size();
        }
        for (std::size_t i = 0; i < numSubchannelsForRu; ++i)
        {
            auto ruAllocNoUsers = HeRu::GetEqualizedRuAllocation(ruType, false, false);
            ruTypes.at(index + i) = ruType;
            ruAllocations.at(index + i) =
                (IsSigBCompression() || ((index + i) % 2) == ccIndex) ? ruAlloc : ruAllocNoUsers;
        }
    }
    return ruAllocations;
}

Center26ToneRuIndication
WifiTxVector::DeriveCenter26ToneRuIndication() const
{
    uint8_t center26ToneRuIndication{0};
    for (const auto& userInfo : m_muUserInfos)
    {
        NS_ASSERT(WifiRu::IsHe(userInfo.second.ru));
        if ((WifiRu::GetRuType(userInfo.second.ru) == RuType::RU_26_TONE) &&
            (WifiRu::GetIndex(userInfo.second.ru) == 19))
        {
            center26ToneRuIndication |=
                (std::get<HeRu::RuSpec>(userInfo.second.ru).GetPrimary80MHz())
                    ? CENTER_26_TONE_RU_LOW_80_MHZ_ALLOCATED
                    : CENTER_26_TONE_RU_HIGH_80_MHZ_ALLOCATED;
        }
    }
    return static_cast<Center26ToneRuIndication>(center26ToneRuIndication);
}

} // namespace ns3

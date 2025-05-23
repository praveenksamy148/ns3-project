/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  kwong yin <kwong-sang.yin@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 *  Erwan Livolant <erwan.livolant@inria.fr>
 */

#ifndef LR_WPAN_MAC_TRAILER_H
#define LR_WPAN_MAC_TRAILER_H

#include "ns3/trailer.h"

namespace ns3
{

class Packet;

namespace lrwpan
{

/**
 * @ingroup lr-wpan
 *
 * Represent the Mac Trailer with the Frame Check Sequence field.
 */
class LrWpanMacTrailer : public Trailer
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor for a MAC trailer with disabled FCS calculation.
     */
    LrWpanMacTrailer();

    // Inherited from the Trailer class.
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Get this trailers FCS value. If FCS calculation is disabled for this
     * trailer, the returned value is always 0.
     *
     * @return the FCS value.
     */
    uint16_t GetFcs() const;

    /**
     * Calculate and set the FCS value based on the given packet.
     *
     * @param p the packet for which the FCS should be calculated
     */
    void SetFcs(Ptr<const Packet> p);

    /**
     * Check the FCS of a given packet against the FCS value stored in the
     * trailer. The packet itself should contain no trailer. If FCS calculation is
     * disabled for this trailer, CheckFcs() will always return true.
     *
     * @param p the packet to be checked
     * @return false, if the FCS values do not match, true otherwise
     */
    bool CheckFcs(Ptr<const Packet> p);

    /**
     * Enable or disable FCS calculation for this trailer.
     *
     * @param enable flag, indicating if FCS calculation should be enabled or not
     */
    void EnableFcs(bool enable);

    /**
     * Query if FCS calculation is enabled for this trailer.
     *
     * @return true, if FCS calculation is enabled, false otherwise.
     */
    bool IsFcsEnabled() const;

  private:
    /**
     * Calculate the 16-bit FCS value.
     * CRC16-CCITT with a generator polynomial = ^16 + ^12 + ^5 + 1, LSB first and
     * initial value = 0x0000.
     *
     * @param data the checksum will be calculated over this data
     * @param length the length of the data
     * @return the checksum
     */
    uint16_t GenerateCrc16(uint8_t* data, int length);

    /**
     * The FCS value stored in this trailer.
     */
    uint16_t m_fcs;

    /**
     * Only if m_calcFcs is true, FCS values will be calculated and used in the
     * trailer
     */
    bool m_calcFcs;
};

} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_MAC_TRAILER_H */

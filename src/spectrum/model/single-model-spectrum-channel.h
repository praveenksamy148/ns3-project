/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef SINGLE_MODEL_SPECTRUM_CHANNEL_H
#define SINGLE_MODEL_SPECTRUM_CHANNEL_H

#include "spectrum-channel.h"
#include "spectrum-model.h"

#include "ns3/traced-callback.h"

namespace ns3
{

/**
 * @ingroup spectrum
 *
 * @brief SpectrumChannel implementation which handles a single spectrum model
 *
 * All SpectrumPhy layers attached to this SpectrumChannel
 */
class SingleModelSpectrumChannel : public SpectrumChannel
{
  public:
    SingleModelSpectrumChannel();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from SpectrumChannel
    void RemoveRx(Ptr<SpectrumPhy> phy) override;
    void AddRx(Ptr<SpectrumPhy> phy) override;
    void StartTx(Ptr<SpectrumSignalParameters> params) override;

    // inherited from Channel
    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

    /// Container: SpectrumPhy objects
    typedef std::vector<Ptr<SpectrumPhy>> PhyList;

  private:
    void DoDispose() override;

    /**
     * Used internally to reschedule transmission after the propagation delay.
     *
     * @param params
     * @param receiver
     */
    void StartRx(Ptr<SpectrumSignalParameters> params, Ptr<SpectrumPhy> receiver);

    /**
     * List of SpectrumPhy instances attached to the channel.
     */
    PhyList m_phyList;

    /**
     * SpectrumModel that this channel instance is supporting.
     */
    Ptr<const SpectrumModel> m_spectrumModel;
};

} // namespace ns3

#endif /* SINGLE_MODEL_SPECTRUM_CHANNEL_H */

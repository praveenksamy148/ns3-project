/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#ifndef ENERGY_HARVESTER_HELPER_H
#define ENERGY_HARVESTER_HELPER_H

#include "energy-harvester-container.h"
#include "energy-source-container.h"

#include "ns3/attribute.h"
#include "ns3/energy-harvester.h"
#include "ns3/energy-source.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ptr.h"

namespace ns3
{

/**
 * @ingroup energy
 * @brief Creates EnergyHarvester objects.
 *
 * This class creates and installs energy harvesters onto network nodes.
 *
 */
class EnergyHarvesterHelper
{
  public:
    virtual ~EnergyHarvesterHelper();

    /**
     * @param name Name of attribute to set.
     * @param v Value of the attribute.
     *
     * Sets one of the attributes of underlying EnergyHarvester.
     */
    virtual void Set(std::string name, const AttributeValue& v) = 0;

    /**
     * @param source Pointer to the energy source where EnergyHarvester will be installed.
     * @returns An EnergyHarvesterContainer which contains all the EnergyHarvesters.
     *
     * This function installs an EnergyHarvester onto an energy source.
     */
    energy::EnergyHarvesterContainer Install(Ptr<energy::EnergySource> source) const;

    /**
     * @param sourceContainer List of nodes where EnergyHarvester will be installed.
     * @returns An EnergyHarvesterContainer which contains all the EnergyHarvester.
     *
     * This function installs an EnergyHarvester onto a list of energy sources.
     */
    energy::EnergyHarvesterContainer Install(energy::EnergySourceContainer sourceContainer) const;

    /**
     * @param sourceName Name of node where EnergyHarvester will be installed.
     * @returns An EnergyHarvesterContainer which contains all the EnergyHarvesters.
     *
     * This function installs an EnergyHarvester onto a node.
     */
    energy::EnergyHarvesterContainer Install(std::string sourceName) const;

  private:
    /**
     * @param source Pointer to node where the energy harvester is to be installed.
     * @returns Pointer to the created EnergyHarvester.
     *
     * Child classes of EnergyHarvesterHelper only have to implement this function,
     * to create and aggregate an EnergyHarvester object onto a single node. Rest of
     * the installation process (eg. installing EnergyHarvester on set of nodes) is
     * implemented in the EnergyHarvesterHelper base class.
     */
    virtual Ptr<energy::EnergyHarvester> DoInstall(Ptr<energy::EnergySource> source) const = 0;
};

} // namespace ns3

#endif /* defined(ENERGY_HARVESTER_HELPER_H) */

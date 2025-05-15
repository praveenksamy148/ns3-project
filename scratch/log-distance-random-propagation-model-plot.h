/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 *
 */

 #ifndef LOG_DISTANCE_RANDOM_PROPAGATION_MODEL_H
 #define LOG_DISTANCE_RANDOM_PROPAGATION_MODEL_H
 
 #include "ns3/object.h"
 #include "ns3/random-variable-stream.h"
 
 #include <unordered_map>
 
 namespace ns3
 {
     /**
      * @ingroup propagation
      *
      * @brief a log distance propagation model.
      *
      * This model calculates the reception power with a so-called
      * log-distance propagation model:
      * \f$ L = L_0 + 10 n log_{10}(\frac{d}{d_0})\f$
      *
      * where:
      *  - \f$ n \f$ : the path loss distance exponent
      *  - \f$ d_0 \f$ : reference distance (m)
      *  - \f$ L_0 \f$ : path loss at reference distance (dB)
      *  - \f$ d \f$ : distance (m)
      *  - \f$ L \f$ : path loss (dB)
      *
      * When the path loss is requested at a distance smaller than
      * the reference distance, the tx power is returned.
      *
      */
     class LogDistanceRandomPropagationModel : public PropagationLossModel
     {
         public:
     /**
      * @brief Get the type ID.
      * @return the object TypeId
      */
     static TypeId GetTypeId();
     LogDistanceRandomPropagationModel();
 
     // Delete copy constructor and assignment operator to avoid misuse
     LogDistanceRandomPropagationModel(const LogDistanceRandomPropagationModel&) = delete;
     LogDistanceRandomPropagationModel& operator=(const LogDistanceRandomPropagationModel&) = delete;
 
     /**
      * @param n the path loss exponent.
      * Set the path loss exponent.
      */
     void SetPathLossExponent(double n);
     /**
      * @returns the current path loss exponent.
      */
     double GetPathLossExponent() const;
 
     /**
      * Set the reference path loss at a given distance
      * @param referenceDistance reference distance
      * @param referenceLoss reference path loss
      */
     void SetReference(double referenceDistance, double referenceLoss);
 
   private:
     double DoCalcRxPower(double txPowerDbm,
                          Ptr<MobilityModel> a,
                          Ptr<MobilityModel> b) const override;
 
     int64_t DoAssignStreams(int64_t stream) override;
 
     /**
      *  Creates a default reference loss model
      * @return a default reference loss model
      */
     static Ptr<PropagationLossModel> CreateDefaultReference();
 
     Ptr<NormalRandomVariable> Xg; //!< random generator
     double m_exponent;          //!< model exponent
     double m_referenceDistance; //!< reference distance
     double m_referenceLoss;     //!< reference loss
     }
 };
 
 
 
 
 #endif /* LOG_DISTANCE_PATH_LOSS_MODEL_H */


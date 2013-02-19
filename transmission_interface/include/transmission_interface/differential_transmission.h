///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2013, PAL Robotics S.L.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of hiDOF, Inc. nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////

/// \author Adolfo Rodriguez Tsouroukdissian

#ifndef TRANSMISSION_INTERFACE_DIFFERENTIAL_TRANSMISSION_H
#define TRANSMISSION_INTERFACE_DIFFERENTIAL_TRANSMISSION_H

#include <cassert>
#include <string>
#include <vector>

#include <transmission_interface/transmission.h>
#include <transmission_interface/transmission_exception.h>

namespace transmission_interface
{

/**
 * \brief Implementation of a differential transmission.
 *
 * This transmission relates <b>two actuators</b> and <b>two joints</b> through a differential mechanism, as illustrated
 * below.
 * \image html differential_transmission.png
 *
 * <CENTER>
 * <table>
 * <tr><th></th><th><CENTER>Effort</CENTER></th><th><CENTER>Velocity</CENTER></th><th><CENTER>Position</CENTER></th></tr>
 * <tr><td>
 * <b> Actuator to joint </b>
 * </td>
 * <td>
 * \f{eqnarray*}{
 * \tau_{j_1} & = & n_{j_1} ( n_{a_1} \tau_{a_1} + n_{a_2} \tau_{a_2} ) \\[2.5em]
 * \tau_{j_2} & = & n_{j_2} ( n_{a_1} \tau_{a_1} + n_{a_2} \tau_{a_2} )
 * \f}
 * </td>
 * <td>
 * \f{eqnarray*}{
 * \dot{x}_{j_1} & = & \frac{ \dot{x}_{a_1} / n_{a_1} + \dot{x}_{a_2} / n_{a_2} }{2 n_{j_1}} \\[1em]
 * \dot{x}_{j_2} & = & \frac{ \dot{x}_{a_1} / n_{a_1} - \dot{x}_{a_2} / n_{a_2} }{2 n_{j_2}}
 * \f}
 * </td>
 * <td>
 * \f{eqnarray*}{
 * x_{j_1} & = & \frac{ x_{a_1} / n_{a_1} + x_{a_2} / n_{a_2} }{2 n_{j_1}}  + x_{off_1} \\[1em]
 * x_{j_2} & = & \frac{ x_{a_1} / n_{a_1} - x_{a_2} / n_{a_2} }{2 n_{j_1}}  + x_{off_2}
 * \f}
 * </td>
 * </tr>
 * <tr><td>
 * <b> Joint to actuator </b>
 * </td>
 * <td>
 * \f{eqnarray*}{
 * \tau_{a_1} & = & \frac{ \tau_{j_1} / n_{j_1} + \tau_{j_2} / n_{j_2} }{2 n_{a_1}} \\[1em]
 * \tau_{a_2} & = & \frac{ \tau_{j_1} / n_{j_1} - \tau_{j_2} / n_{j_2} }{2 n_{a_1}}
 * \f}
 * </td>
 * <td>
 * \f{eqnarray*}{
 * \dot{x}_{a_1} & = & n_{a_1} ( n_{j_1} \dot{x}_{j_1} + n_{j_2} \dot{x}_{j_2} ) \\[2.5em]
 * \dot{x}_{a_2} & = & n_{a_2} ( n_{j_1} \dot{x}_{j_1} - n_{j_2} \dot{x}_{j_2} )
 * \f}
 * </td>
 * <td>
 * \f{eqnarray*}{
 * x_{a_1} & = & n_{a_1} \left[ n_{j_1} (x_{j_1} - x_{off_1}) + n_{j_2} (x_{j_2} - x_{off_2}) \right] \\[2.5em]
 * x_{a_2} & = & n_{a_2} \left[ n_{j_1} (x_{j_1} - x_{off_1}) - n_{j_2} (x_{j_2} - x_{off_2}) \right]
 * \f}
 * </td></tr></table>
 * </CENTER>
 *
 * where:
 * - \f$ x \f$, \f$ \dot{x} \f$ and \f$ \tau \f$ are position, velocity and effort variables, respectively.
 * - Subindices \f$ _a \f$ and \f$ _j \f$ are used to represent actuator-space and joint-space variables, respectively.
 * - \f$ x_{off}\f$ represents the offset between motor and joint zeros, expressed in joint position coordinates
 *   (cf. SimpleTransmission class documentation for a more detailed descrpition of this variable).
 * - \f$ n \f$ represents a transmission ratio. Reducers/amplifiers are allowed on both the actuator and joint sides
 *   (depicted as timing belts in the figure).
 *  A transmission ratio can take any real value \e except zero. In particular:
 *     - If its absolute value is greater than one, it's a velocity reducer / effort amplifier, while if its absolute
 *       value lies in \f$ (0, 1) \f$ it's a velocity amplifier / effort reducer.
 *     - Negative values represent a direction flip, ie. input and output move in opposite directions.
 *     - <b>Important:</b> Use transmission ratio signs to match this class' convention of positive actuator/joint
 *       directions with a given mechanical design, as they will in general not match.
 *
 * \note This implementation currently assumes a specific layout for location of the actuators and joint axes which is
 * common in robotic mechanisms. Please file an enhancement ticket if your use case does not adhere to this layout.
 */
class DifferentialTransmission : public Transmission
{
public:
  /**
   * \param actuator_reduction Reduction ratio of actuators.
   * \param joint_reduction    Reduction ratio of joints.
   * \param joint_offset       Joint position offset used in the position mappings.
   * \pre Nonzero actuator and joint reduction values.
   */
  DifferentialTransmission(const std::vector<double>& actuator_reduction,
                           const std::vector<double>& joint_reduction,
                           const std::vector<double>& joint_offset = std::vector<double>(2, 0.0));

  virtual ~DifferentialTransmission() {}

  virtual void actuatorToJointEffort(const std::vector<double*> actuator_eff,
                                           std::vector<double*> joint_eff);

  virtual void actuatorToJointVelocity(const std::vector<double*> actuator_vel,
                                             std::vector<double*> joint_vel);

  virtual void actuatorToJointPosition(const std::vector<double*> actuator_pos,
                                             std::vector<double*> joint_pos);

  virtual void jointToActuatorEffort(const std::vector<double*> joint_eff,
                                           std::vector<double*> actuator_eff);

  virtual void jointToActuatorVelocity(const std::vector<double*> joint_vel,
                                             std::vector<double*> actuator_vel);

  virtual void jointToActuatorPosition(const std::vector<double*> joint_pos,
                                             std::vector<double*> actuator_pos);

  virtual std::size_t numActuators() const {return 2;}
  virtual std::size_t numJoints()    const {return 2;}

protected:
  std::vector<double>  actuator_reduction_;
  std::vector<double>  joint_reduction_;
  std::vector<double>  joint_offset_;
  std::vector<double*> joint_pos_vec_; ///< Workspace vector used to modify otherwise const parameters.
};

inline DifferentialTransmission::DifferentialTransmission(const std::vector<double>& actuator_reduction,
                                                          const std::vector<double>& joint_reduction,
                                                          const std::vector<double>& joint_offset)
  : Transmission(),
    actuator_reduction_(actuator_reduction),
    joint_reduction_(joint_reduction),
    joint_offset_(joint_offset),
    joint_pos_vec_(2)
{
  if (2 != actuator_reduction.size() ||
      2 != joint_reduction_.size()   ||
      2 != joint_offset_.size())
  {
    throw TransmissionException("Reduction and offset vectors of a differential transmission must have size 2.");
  }

  if (0.0 == actuator_reduction[0] ||
      0.0 == actuator_reduction[1] ||
      0.0 == joint_reduction_[0]   ||
      0.0 == joint_reduction_[1]
  )
  {
    throw TransmissionException("Transmission reduction ratios cannot be zero.");
  }
}

inline void DifferentialTransmission::actuatorToJointEffort(const std::vector<double*> actuator_eff,
                                                                  std::vector<double*> joint_eff)
{
  assert(2 == actuator_eff.size() && 2 == joint_eff.size());
  std::vector<double>& ar = actuator_reduction_;
  std::vector<double>& jr = joint_reduction_;
  *joint_eff[0] = jr[0] * (*actuator_eff[0] * ar[0] + *actuator_eff[1] * ar[1]);
  *joint_eff[1] = jr[1] * (*actuator_eff[0] * ar[0] - *actuator_eff[1] * ar[1]);
}

inline void DifferentialTransmission::actuatorToJointVelocity(const std::vector<double*> actuator_vel,
                                                                    std::vector<double*> joint_vel)
{
  assert(2 == actuator_vel.size() && 2 == joint_vel.size());
  std::vector<double>& ar = actuator_reduction_;
  std::vector<double>& jr = joint_reduction_;
  *joint_vel[0] = (*actuator_vel[0] / ar[0] + *actuator_vel[1] / ar[1]) / (2.0 * jr[0]);
  *joint_vel[1] = (*actuator_vel[0] / ar[0] - *actuator_vel[1] / ar[1]) / (2.0 * jr[1]);
}

inline void DifferentialTransmission::actuatorToJointPosition(const std::vector<double*> actuator_pos,
                                                                    std::vector<double*> joint_pos)
{
  assert(2 == actuator_pos.size() && 2 == joint_pos.size());
  actuatorToJointVelocity(actuator_pos, joint_pos); // Apply flow map...
  *joint_pos[0] += joint_offset_[0];                // ...and add integration constant
  *joint_pos[1] += joint_offset_[1];                // ...to each joint
}

inline void DifferentialTransmission::jointToActuatorEffort(const std::vector<double*> joint_eff,
                                                                  std::vector<double*> actuator_eff)
{
  assert(2 == actuator_eff.size() && 2 == joint_eff.size());
  std::vector<double>& ar = actuator_reduction_;
  std::vector<double>& jr = joint_reduction_;
  *actuator_eff[0] = (*joint_eff[0] / jr[0] + *joint_eff[1] / jr[1]) / (2.0 * ar[0]);
  *actuator_eff[1] = (*joint_eff[0] / jr[0] - *joint_eff[1] / jr[1]) / (2.0 * ar[1]);
}

inline void DifferentialTransmission::jointToActuatorVelocity(const std::vector<double*> joint_vel,
                                                                    std::vector<double*> actuator_vel)
{
  assert(2 == actuator_vel.size() && 2 == joint_vel.size());
  std::vector<double>& ar = actuator_reduction_;
  std::vector<double>& jr = joint_reduction_;
  *actuator_vel[0] = (*joint_vel[0] * jr[0] + *joint_vel[1] * jr[1]) * ar[0];
  *actuator_vel[1] = (*joint_vel[0] * jr[0] - *joint_vel[1] * jr[1]) * ar[1];
}

inline void DifferentialTransmission::jointToActuatorPosition(const std::vector<double*> joint_pos,
                                                                    std::vector<double*> actuator_pos)
{
  assert(2 == actuator_pos.size() && 2 == joint_pos.size());
  double joint_pos_with_offset[2] = {*joint_pos[0] - joint_offset_[0],
                                     *joint_pos[1] - joint_offset_[1]};
  joint_pos_vec_[0] = &joint_pos_with_offset[0];         // Remove integration constant in workspace vector...
  joint_pos_vec_[1] = &joint_pos_with_offset[1];         // ...from each joint
  jointToActuatorVelocity(joint_pos_vec_, actuator_pos); // ...and apply flow map to _workspace_ vector
}

} // transmission_interface

#endif // TRANSMISSION_INTERFACE_DIFFERENTIAL_TRANSMISSION_H

// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <frc2/command/CommandBase.h>
#include <frc2/command/CommandHelper.h>

#include <frc/controller/RamseteController.h>
#include <frc/kinematics/DifferentialDriveKinematics.h>
#include <frc2/command/PIDCommand.h>
#include <frc/controller/SimpleMotorFeedforward.h>

#include <subsystems/Drivetrain.h>
#include <subsystems/Vision.h>

/**
 * An example command.
 *
 * <p>Note that this extends CommandHelper, rather extending CommandBase
 * directly; this is crucially important, or else the decorator functions in
 * Command will *not* work!
 */
class GoToTarget
    : public frc2::CommandHelper<frc2::CommandBase, GoToTarget> {
  public:
    GoToTarget(
      Drivetrain* drivetrain, 
      Vision* vision,
      frc::RamseteController ramseteController,
      frc::DifferentialDriveKinematics kinematics,
      frc::SimpleMotorFeedforward<units::meters> feedforward, 
      frc2::PIDController leftPID, 
      frc2::PIDController rightPID, 
      std::function<frc::DifferentialDriveWheelSpeeds()> wheelSpeedsGetter, 
      std::function<void(units::volt_t left, units::volt_t right)> motorVoltageSetter
    );

    void Initialize() override;

    void Execute() override;

    void End(bool interrupted) override;
    
    bool IsFinished() override;

  private: 
    // subsystem requirements
    Drivetrain* m_drivetrain; 
    Vision* m_vision; 
    
    // tracking
    frc::Timer timer; 
    units::second_t previousTime; 
    frc::DifferentialDriveWheelSpeeds previousSpeed; 

    // calculations
    frc::RamseteController m_controller; 
    frc::DifferentialDriveKinematics m_kinematics;
    frc::SimpleMotorFeedforward<units::meters> m_feedforward;
    struct {
      std::unique_ptr<frc2::PIDController> left;
      std::unique_ptr<frc2::PIDController> right;
    } m_pid; 

    // robot physical parameter getters 
    std::function<frc::DifferentialDriveWheelSpeeds()> getSpeeds;

    // output
    std::function<void(units::volt_t left, units::volt_t right)>m_output;

    // config 
    units::meters_per_second_t desiredVelocity { 0.5 }; 
};

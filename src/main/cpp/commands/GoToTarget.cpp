// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "commands/GoToTarget.h"

GoToTarget::GoToTarget(
  Drivetrain* drivetrain,
  Vision* vision,
  frc::RamseteController ramseteController,
  frc::DifferentialDriveKinematics kinematics,
  frc::SimpleMotorFeedforward<units::meters> feedforward, 
  frc2::PIDController leftPID, 
  frc2::PIDController rightPID, 
  std::function<frc::DifferentialDriveWheelSpeeds()> wheelSpeedsGetter, 
  std::function<void(units::volt_t left, units::volt_t right)> motorVoltageSetter
) : // initilize list 
  m_controller(ramseteController), 
  m_kinematics(kinematics),
  m_feedforward(feedforward)
{

  AddRequirements({ drivetrain, vision }); 

  // initializing pointers
  this->m_pid.left = std::make_unique<frc2::PIDController>(leftPID); 
  this->m_pid.right = std::make_unique<frc2::PIDController>(rightPID); 
  this->m_output = motorVoltageSetter; 
  this->getSpeeds = wheelSpeedsGetter; 
  this->m_drivetrain = drivetrain; 
  this->m_vision = vision; 
}

// Called when the command is initially scheduled.
void GoToTarget::Initialize() {
  previousTime = units::second_t(-1); 

  previousSpeed = m_kinematics.ToWheelSpeeds(frc::ChassisSpeeds());

  m_pid.left->Reset();
  m_pid.right->Reset();

  timer.Reset(); 
  timer.Start();
}

// Called repeatedly when this Command is scheduled to run
void GoToTarget::Execute() {

  auto currentTime = timer.Get(); 
  auto dt = currentTime - previousTime; 
  this->previousTime = currentTime; 

  auto targetPtr = m_vision->GetBestTarget(); 
  if (targetPtr == nullptr) {
    m_output(units::volt_t(0), units::volt_t(0)); 
    return; 
  }
  auto target = *targetPtr;  

  // auto targetPose3D = target.GetBestCameraToTarget(); 

  // converting 3d tracking to 2d translation and rotation
  // auto translationToTarget = targetPose3D.Translation().ToTranslation2d();
  // auto rotationToTarget = targetPose3D.Rotation().ToRotation2d();

  auto rotationToTarget = frc::Rotation2d(units::degree_t(-target.GetYaw()));
  auto translationToTarget = frc::Translation2d(units::meter_t(1), rotationToTarget);
  
  // generating a 2d target pose (not actually to go there, only for calculations)
  auto targetPose = frc::Pose2d(translationToTarget, rotationToTarget); 
  
  auto distance = units::meter_t(std::hypot(translationToTarget.X().value(), translationToTarget.Y().value()));
  


  auto linearVelocity = desiredVelocity; 
  // purpousely lowering speed 
  // if (!target.isCertain) linearVelocity /= 2;

  auto estimatedTime = distance / linearVelocity;  

  auto angularVelocity = rotationToTarget.Radians() / estimatedTime; 
  frc::SmartDashboard::PutNumber("Desired AV", angularVelocity.value());

  // output calculation 
  // calculations referenced from frc::RasmeteCommand
  //https://github.com/wpilibsuite/allwpilib/blob/878cc8defb27f5089395186ad1d907993b57be9c/wpilibNewCommands/src/main/native/cpp/frc2/command/RamseteCommand.cpp

  // this's most likely not gonna work, it's more like a closed looped controller rn cuz it'll be reacting based on the 
  // next to impossible results of wished linear no movement of target & accurate info 

  auto chassieSpeeds = m_controller.Calculate(frc::Pose2d(), targetPose, linearVelocity, angularVelocity); 
  auto targetWheelSpeeds = m_kinematics.ToWheelSpeeds(chassieSpeeds); 

  // feedforward and pid control stuff
  auto leftFeedforward = m_feedforward.Calculate(
      targetWheelSpeeds.left,
      (targetWheelSpeeds.left - previousSpeed.left) / dt);

  auto rightFeedforward = m_feedforward.Calculate(
      targetWheelSpeeds.right,
      (targetWheelSpeeds.right - previousSpeed.right) / dt);

  auto leftOutput =
      units::volt_t{m_pid.left->Calculate(
          getSpeeds().left.value(), targetWheelSpeeds.left.value())} +
      leftFeedforward;

  auto rightOutput =
      units::volt_t{m_pid.right->Calculate(
          getSpeeds().right.value(), targetWheelSpeeds.right.value())} +
      rightFeedforward;

  m_output(leftOutput, rightOutput); 

  previousSpeed = targetWheelSpeeds;
}

// Called once the command ends or is interrupted.
void GoToTarget::End(bool interrupted) {}

// Returns true when the command should end.
bool GoToTarget::IsFinished() {
  return false;
}

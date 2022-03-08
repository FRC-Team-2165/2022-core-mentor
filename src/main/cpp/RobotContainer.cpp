// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "RobotContainer.h"

#include <frc/DriverStation.h>

#include <frc2/command/button/JoystickButton.h>
#include <frc2/command/StartEndCommand.h>
#include <frc2/command/SequentialCommandGroup.h>
#include <frc2/command/InstantCommand.h>
#include <frc2/command/WaitCommand.h>
#include <frc2/command/WaitUntilCommand.h>
#include <frc2/command/FunctionalCommand.h>
#include <frc2/command/RunCommand.h>
#include <frc2/command/ParallelRaceGroup.h>
#include <frc2/command/ConditionalCommand.h>
#include <frc2/command/PrintCommand.h>
#include <frc2/command/button/Trigger.h>

#include <units/time.h>

#include "commands/DriveCommand.h"
#include "commands/FlywheelCommand.h"
#include "commands/DriveToLineCommand.h"
#include "commands/DriveUntilCommand.h"

#include <cstdio>

RobotContainer::RobotContainer() : transportSubsystem(frc::DriverStation::GetAlliance()) {
  // Initialize all of your commands and subsystems here
  // autocmd = frc2::SequentialCommandGroup(
  //     DriveToLineCommand(&driveSubsystem, false),
  //     frc2::InstantCommand([this]{driveSubsystem.resetDistance();}),
  //     DriveUntilCommand(&driveSubsystem, false, [this] {
  //       return driveSubsystem.distance() <= -33;
  //     }),
  //     frc2::InstantCommand([this] {
  //       transportSubsystem.enableInnerBelt();
  //     })
  // );
#ifdef USE_XBOX_CONTROLS
  driveSubsystem.SetDefaultCommand(DriveCommand(&driveSubsystem, &controller));
#else
  driveSubsystem.SetDefaultCommand(DriveCommand(&driveSubsystem, &control1));
#endif
  shooterSubsystem.SetDefaultCommand(FlywheelCommand(&shooterSubsystem));

  innerTimer.Start();
  innerTimer.Reset();
  transportSubsystem.SetDefaultCommand(frc2::RunCommand([this] {
    if(!transportSubsystem.hasOuterBall() || !transportSubsystem.hasInnerBall()) {
      transportSubsystem.enableOuterBelt();
    } else {
      transportSubsystem.disableOuterBelt();
    }

    if(!transportSubsystem.hasInnerBall()) {
      transportSubsystem.enableInnerBelt();
      innerTimer.Reset();
    } else if(innerTimer.HasElapsed(0.15_s)) {
        transportSubsystem.disableInnerBelt();
    } else {
      transportSubsystem.enableInnerBelt();
    }
  }, {&transportSubsystem}));

  // Configure the button bindings
  ConfigureButtonBindings();
}

void RobotContainer::ConfigureButtonBindings() {
  // extends/retracts the roller arm of the intake
  auto toggle_intake_arm = frc2::InstantCommand([this] {intakeSubsystem.toggleArm();}, 
                                                {&intakeSubsystem});

  // Run the intake roller. Run it in reverse if button 11 pressed on Joystick, 
  // or left bumper pressed on Xbox controller.
  auto run_intake_roller = frc2::FunctionalCommand(
      [] {},
      [this]{
        intakeSubsystem.startRoller();
      },
      [this](bool)
      { intakeSubsystem.stopRoller(); },
      []
      { return false; });
  
  auto reverse_intake_roller = frc2::FunctionalCommand(
      [] {},
      [this]{
        intakeSubsystem.reverseRoller();
      },
      [this](bool)
      { intakeSubsystem.stopRoller(); },
      []
      { return false; });

  // Runs the outer transport belt in reverse
  auto reverse_transport = frc2::StartEndCommand(
    [this] {
      transportSubsystem.reverseOuterBelt();
      transportSubsystem.reverseInnerBelt();
    },
    [this] {
      transportSubsystem.disableOuterBelt();
      transportSubsystem.disableInnerBelt();
    },
    {&transportSubsystem}
  );

  auto toggle_lower_arms = frc2::InstantCommand([this]{climberSubsystem.toggleLower();});
  auto upper_arms_release = frc2::StartEndCommand(
    [this]{climberSubsystem.extendUpper(false);},
    [this]{climberSubsystem.retractUpper(false);},
    {&climberSubsystem}
  );

  // Both sets of bindings have equivalant capabilities. 
  // The only difference is which control scheme is being used.
#ifdef USE_XBOX_CONTROLS
  // Button bindings for Xbox Controller
  frc2::JoystickButton(&controller, frc::XboxController::Button::kX).ToggleWhenPressed(toggle_intake_arm);
  frc2::JoystickButton(&controller, frc::XboxController::Button::kY).WhenHeld(run_intake_roller);
  // D-pad left
  frc2::Trigger([this]{return controller.GetPOV() == 270;}).WhileActiveOnce(reverse_outer_transport);
  // D-pad down - drive backwards to line
  frc2::Trigger([this]{return controller.GetPOV() == 180;}).ToggleWhenActive(DriveToLineCommand(&driveSubsystem, false));
  // D-pad down - drive forwards to line
  frc2::Trigger([this]{return controller.GetPOV() == 0;}).ToggleWhenActive(DriveToLineCommand(&driveSubsystem, true));
  frc2::JoystickButton(&controller, frc::XboxController::Button::kB).WhileHeld(&shootCommand);
  frc2::JoystickButton(&controller, frc::XboxController::Button::kStart).WhenPressed(&climb);
  
  (frc2::JoystickButton(&controller, frc::XboxController::Button::kA) && 
    !frc2::JoystickButton(&controller, frc::XboxController::Button::kRightBumper)).WhenPressed(toggle_lower_arms);
  (frc2::JoystickButton(&controller, frc::XboxController::Button::kA) && 
    frc2::JoystickButton(&controller, frc::XboxController::Button::kRightBumper)).ToggleWhenActive(upper_arms_release);
#else
  // Button bindings for Joysticks.
  // Joystick 1 - not including driving
  frc2::JoystickButton(&control1, 1).WhenHeld(frc2::SequentialCommandGroup(
    frc2::RunCommand([this]{
      transportSubsystem.enableInnerBelt();
      transportSubsystem.enableOuterBelt();
    }),
    frc2::WaitUntilCommand([this] {return transportSubsystem.hasInnerBall();}).WithTimeout(2.0_s)
  ));
  // drive forwards to line
  frc2::JoystickButton(&control1, 9).ToggleWhenPressed(DriveToLineCommand(&driveSubsystem, true));
  // drive backwards to line
  frc2::JoystickButton(&control1, 11).ToggleWhenPressed(DriveToLineCommand(&driveSubsystem, false));
  // disable/re-enable shooter
  frc2::JoystickButton(&control1, 7).ToggleWhenPressed(frc2::StartEndCommand(
    [this]{shooterSubsystem.disableFlywheel();},
    [this]{shooterSubsystem.enableFlywheel();},
    {&shooterSubsystem}
  ));

  // Joystick 2
  frc2::JoystickButton(&control2, 1).WhenPressed(toggle_intake_arm);
  (frc2::JoystickButton(&control2, 2) && !frc2::JoystickButton(&control2, 11))
      .WhileActiveOnce(run_intake_roller);
  (frc2::JoystickButton(&control2, 2) && frc2::JoystickButton(&control2, 11))
      .WhileActiveOnce(reverse_intake_roller);
  frc2::JoystickButton(&control2, 4).WhenHeld(reverse_transport);
  frc2::JoystickButton(&control2, 8).WhenPressed(frc2::ConditionalCommand(
    frc2::SequentialCommandGroup(
    frc2::FunctionalCommand(
      [this]{climberSubsystem.retractLower();},
      []{},
      [](bool){},
      [this]{return climberSubsystem.isRetracted();}
    ),
    // frc2::InstantCommand([this]{climberSubsystem.retractLower();}),
    // frc2::WaitUntilCommand([this]{return climberSubsystem.isRetracted();}),
    frc2::WaitCommand(0.5_s),
    frc2::InstantCommand([this]{climberSubsystem.extendUpper();}),
    frc2::WaitCommand(3.0_s),
    frc2::InstantCommand([this]{climberSubsystem.extendLower();}),
    frc2::WaitCommand(3.0_s),
    frc2::InstantCommand([this]{climberSubsystem.retractLower();
                                climberSubsystem.retractUpper();})
    ),
    frc2::InstantCommand([]{}),
    [this]{return !climberSubsystem.isRetracted();}
  ));

  frc2::JoystickButton(&control2, 7).WhenPressed(frc2::InstantCommand([this] {climberSubsystem.isRetracted();}));

  frc2::JoystickButton(&control2, 6).WhenPressed(toggle_lower_arms);
  frc2::JoystickButton(&control2, 5).ToggleWhenPressed(upper_arms_release);
#endif
  
  // TODO list
}

frc2::Command* RobotContainer::autonomousCommand() {
  return &autocmd;  
}


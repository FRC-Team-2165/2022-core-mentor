#pragma once

#include "Constants.h"

#include <functional>
#include <array>

#include <frc2/command/SubsystemBase.h>
#include <rev/CANSparkMax.h>
#include <frc/drive/MecanumDrive.h>
#include <frc/DigitalInput.h>
#include <AHRS.h>
#include <frc/SPI.h>

class DriveSubsystem : public frc2::SubsystemBase {
public:
    DriveSubsystem();
    DriveSubsystem(double deadband);
    DriveSubsystem(double deadband, bool squareXInput, bool squareYInput, bool squareRotInput);
    ~DriveSubsystem() noexcept {}

    void drive(double xSpeed, double ySpeed, double rotation, bool correctRotation = true);
    // turn without resetting the gyro. Useful for rotating to a 
    // specific angle.
    void freeTurn(double speed);

    double distance();
    void resetDistance();

    void resetGyro();
    double orientation();

    bool seesLine();

    void squareXInput(bool square = false) {squareX = square;}
    void squareYInput(bool square = false) {squareY = square;}
    void squareRotInput( bool square = false) {squareRot = square;}
private:
    double deadband;

    bool squareX;
    bool squareY;
    bool squareRot;

    rev::CANSparkMax frontLeft{constants::drive::FRONT_LEFT_MOTOR, rev::CANSparkMax::MotorType::kBrushless};
    rev::CANSparkMax rearLeft{constants::drive::REAR_LEFT_MOTOR, rev::CANSparkMax::MotorType::kBrushless};
    rev::CANSparkMax frontRight{constants::drive::FRONT_RIGHT_MOTOR, rev::CANSparkMax::MotorType::kBrushless};
    rev::CANSparkMax rearRight{constants::drive::REAR_RIGHT_MOTOR, rev::CANSparkMax::MotorType::kBrushless};
    frc::MecanumDrive mecanumDrive{frontLeft, rearLeft, frontRight, rearRight}; 

    AHRS gyro{frc::SPI::Port::kMXP};    

    // frc::DigitalInput lineSensor{constants::drive::LINE_SENSOR};

    // Basic functional iterator. Accumulation can be done by 
    // capturing the accumulator in a lambda.
    void eachMotor(std::function<void(rev::CANSparkMax&)> f) {
        f(frontLeft);
        f(rearLeft);
        f(frontRight);
        f(rearRight);
    }
};
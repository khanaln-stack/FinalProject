# Electronic Keypad Lock System

## Team Members
Niju Khanal  
Madisen Bailey

---

# System Description
This project implements an electronic keypad-controlled locking system using an ESP32-S3 microcontroller. The system allows a user to unlock a servo-driven lock mechanism by entering a four-digit passcode on a 4×4 matrix keypad. A 16×2 LCD display provides user feedback by prompting the user to enter the passcode and displaying system messages such as successful unlock or incorrect password attempts.

When a correct code is entered and confirmed using the enter key (`#`), the system rotates a servo motor to unlock the mechanism and turns on an LED indicator to show that the system is in the unlocked state. After a fixed period of time, the system automatically relocks by rotating the servo back to its original position. If an incorrect code is entered, the LCD displays an error message and the system increments a failed attempt counter. After three incorrect attempts, the system enters an alarm state and activates a buzzer to indicate a security lockout.

The system software is implemented using the ESP-IDF framework and FreeRTOS. Two separate tasks are used in the design. The keypad task continuously scans the keypad matrix and detects user input while implementing debounce logic to prevent repeated key detection. Detected key presses are sent to the control task through a queue. The control task processes user input, updates the LCD display, controls the servo motor, and manages system states. This task-based architecture ensures that the system remains responsive and prevents blocking delays.

---

# Design Alternatives

## Keypad Input Handling

### Option 1: Polling within the main control loop
- The main loop continuously checks the keypad for input.
- This approach is simple to implement but can block other system operations and reduce responsiveness.

### Option 2: Dedicated keypad task (Chosen Approach)
- A separate FreeRTOS task scans the keypad independently.
- Key presses are sent to the main control logic using a queue.
- This approach keeps the system responsive and separates input handling from system control logic.

The dedicated keypad task approach was selected because it provides a cleaner architecture and prevents the control logic from being blocked while waiting for user input.

---

## Lock Mechanism Actuator

### DC Motor
- Could rotate a locking mechanism.
- Requires additional circuitry such as an H-bridge motor driver.
- Difficult to control precise positions.

### Stepper Motor
- Provides precise position control.
- Requires a motor driver and more complex control logic.

### Servo Motor (Chosen Approach)
- Controlled directly with a PWM signal.
- Provides simple position control.
- Only two positions are needed: locked and unlocked.

The servo motor was selected because it simplifies the hardware design and provides reliable position control with minimal additional circuitry.

---

## User Feedback Interface

### Serial Monitor
- Useful during development for debugging.
- Requires connection to a computer.
- Not practical for standalone operation.

### LCD Display (Chosen Approach)
- Provides clear real-time feedback to the user.
- Allows the system to operate independently from a computer.
- Displays instructions, status messages, and error notifications.

A 16×2 LCD display was chosen to provide a simple and intuitive interface for users interacting with the lock system.

---

## Power System Design

### Single Power Source
- Powering all components from the ESP32.
- Servos can draw large currents which may destabilize the microcontroller.

### Separate Power Sources (Chosen Approach)
- ESP32 powered through USB or battery pack.
- Servo motor powered by an external battery pack.
- Grounds connected to maintain a common reference.

Using a separate power supply for the servo motor improves system stability and prevents voltage drops that could reset the microcontroller.

---

# Testing Summary

Testing was performed in stages to verify correct operation of both hardware components and software functionality.

| Test Component | Test Description | Expected Result | Observed Result | Status |
|----------------|-----------------|----------------|----------------|--------|
| LCD Display Initialization | Initialize LCD and print test message | LCD displays text correctly | LCD displayed test messages after initialization | Pass |
| Keypad Matrix Detection | Press each key on the keypad and verify correct character detection | Correct character detected for each key press | All keys correctly detected by keypad scanning task | Pass |
| Keypad Debounce Logic | Press and hold keys to verify debounce filtering | Only one input registered per key press | Debounce logic successfully prevented repeated inputs | Pass |
| Servo Motor Control | Send PWM signals to rotate servo between lock and unlock positions | Servo rotates between defined angles | Servo rotated correctly between locked and unlocked states | Pass |
| Passcode Verification | Enter correct passcode and press enter key | Servo unlocks and LCD displays success message | Correct code triggered unlock sequence | Pass |
| Incorrect Code Handling | Enter incorrect passcode | LCD displays error message and increments attempt counter | Incorrect attempts tracked correctly | Pass |
| Security Lockout | Enter incorrect passcode three times | Buzzer activates and system enters alarm state | Alarm triggered after three failed attempts | Pass |
| Auto Relock Function | Wait after successful unlock | System relocks automatically after timeout | Servo returned to locked position after delay | Pass |
| Integrated System Test | Full system operation with keypad input, LCD feedback, and servo control | System performs all functions correctly | System operated reliably during repeated tests | Pass |

Testing confirmed that the system responds correctly to user input, maintains stable operation across all components, and enforces security behavior as designed.

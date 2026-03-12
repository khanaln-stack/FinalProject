# Smart Door Lock Security System

Team Members: Niju Khanal, Madisen Bailey

---

## System Behavior

This project implements a smart door locking system using an ESP32-S3 microcontroller, a 4×4 keypad, a 16×2 LCD display, a servo motor, an LED indicator, and a buzzer. The keypad allows a user to enter a four-digit security code, which is processed by the microcontroller. The LCD provides instructions and feedback to the user, such as prompting the user to enter the code and displaying whether access is granted or denied. As the user enters digits, the display shows masked characters to indicate progress.

When the correct code is entered, the system unlocks the door by rotating the servo motor to the unlock position. At the same time, the LED turns on to indicate successful access. The door remains unlocked for approximately ten seconds, after which the system automatically returns the servo to the locked position and turns the LED off. This automatic relock ensures the door does not remain unlocked if the user forgets to secure it.

If the user enters an incorrect code, the LCD displays a message indicating that access is denied and shows how many incorrect attempts have occurred. The system keeps track of failed attempts, and after three incorrect entries, it enters an alarm state. In this state, the buzzer activates and the system becomes locked, preventing further access attempts. The software uses FreeRTOS tasks to separate keypad scanning from the main control logic so the system can continuously read input, update the display, and control the locking mechanism without delays.

---

## Design Alternatives

Several design options were considered before selecting the final configuration of the system.

For the access method, options included keypad PIN entry, biometric authentication such as fingerprint recognition, and unlocking through a mobile application. The keypad method was chosen because it is simple, reliable, and does not require additional hardware such as sensors or wireless modules.

For user feedback, options included LED indicators, an LCD display, or both. The final design uses both an LCD and an LED. The LCD allows the system to display detailed messages such as prompts and error notifications, while the LED provides a quick visual indication when the door has been successfully unlocked.

For the locking mechanism, a servo motor and a DC motor were considered. A servo motor was selected because it allows precise control of the locked and unlocked positions using PWM signals, which simplifies the design compared to a DC motor that would require additional mechanical stopping mechanisms.

For security alerts, the system could either show visual warnings or produce an audible alert. The final design uses a buzzer together with LCD messages so the system clearly indicates when too many incorrect attempts have occurred.

The system also implements two security features: a limit of three incorrect attempts and an automatic relock after ten seconds. These features help improve security and prevent the door from remaining unlocked accidentally.

---

## Summary of Testing Results

| Subsystem | Behavior Tested | Expected Result | Observed Result |
|-----------|----------------|----------------|----------------|
| Keypad | Detect key press | Correct key is detected when pressed | Key presses were detected correctly |
| Keypad | Debounce behavior | Single key press registers once | No repeated key signals occurred |
| LCD | Display prompt | LCD shows "Enter Code" | Prompt displayed correctly |
| LCD | Display access result | LCD shows access granted or denied | Messages displayed correctly |
| Servo Motor | Unlock action | Servo rotates to unlock position | Servo moved to unlock position |
| Servo Motor | Automatic relock | Servo returns to lock after 10 seconds | Servo relocked after delay |
| LED | Unlock indicator | LED turns on when door unlocks | LED turned on correctly |
| Security Logic | Failed attempt counting | Attempts increase after wrong code | Attempt counter worked correctly |
| Security Logic | Alarm after 3 attempts | Buzzer activates and system locks | Alarm triggered correctly |

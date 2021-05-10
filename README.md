![alt text](https://github.com/DanielDW5555/flamingo/blob/main/photos/PCBA.jpg)

This is the first original project I have completed from start to finish. My inspiration for this project was flamingos, and how they are usually seen balanced upright on one leg. The goal of the project was to create a self balancing robot that could stand upright for a short period of time.

The system works by interfacing the LIS3LV02DLTR accelerometer with the STM32F3 microcontroller to monitor the x and z component of acceleration. The data from the accelerometer is then used by a PID control system implemented on the microcontroller which configures the PWM duty cycle of the brushed DC motors.

The following where the requirements I used to design the robot:
  - The robot must be able to stand up right during operation
  - The robot will be powered by a 9V battery
  - All electronic devices will be implemented on a single PCB
  - The size of the PCB will be equal to or smaller than a credit card
  - No price limit on the BOM (Although my wallet will regret this later...)

The assembled robot fulfills the requirements listed above, but the PID controller requires a lot of tuning to get it to stand upright. Having a larger robot would make tuning the PID control system easier.

The following images depict the blank flamingo PCB and the rev 2 schematic.

Blank PCB
![alt text](https://github.com/DanielDW5555/flamingo/blob/main/photos/PCB.jpg)

Rev 2 schematic
![alt text](https://github.com/DanielDW5555/flamingo/blob/Rev2/photos/sch.PNG)

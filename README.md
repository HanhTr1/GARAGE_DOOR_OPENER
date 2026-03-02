# GARAGE_DOOR_OPENER
Garage door opener consists of the following parts: a stepper motor to move the door up and down,
a rotary encoder detect door movement and movement direction, two limit switches to detect when
the door has reached the end of the movement range, and a door. The door in our test setup is a
plastic block that is driven back and forth by the motor.
The door opener can be operated both locally and remotely. Buttons are used for local control and
the controller subscribes to an MQTT topic to receive remote commands. The garage door opener
communicates its state by publishing MQTT messages. The status indicates if the door is open or
closed and if the controller is in an error state, for example the door is stuck.

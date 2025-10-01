# AUTOSAR Architecture Layers

## Application Layer
This layer is responsible for defining the application software that controls various vehicle functions. It includes the software components (SW-Cs) that interact with the vehicle's ECUs.

## Runtime Environment (RTE)
The RTE acts as an intermediary layer between the application and the basic software layer, ensuring communication and data exchange between SW-Cs.

## Basic Software Layer
This layer consists of standardized software modules responsible for hardware abstraction, communication, diagnostics, and other low-level functions. It provides a uniform interface for the application software.

## ECU Abstraction Layer
The ECU abstraction layer abstracts the underlying hardware differences among various ECUs, making it easier to port and adapt software to different hardware platforms.

## Communication Stack
AUTOSAR includes communication stacks that enable various ECUs to communicate through standardized protocols like CAN, Ethernet, and FlexRay.

## Complex Device Drivers
These drivers provide low-level access to hardware components like sensors and actuators, allowing the application software to interact with them.

# AUTOSAR Compliance Evaluation Criteria

Runnable Timing → Each software component runnable (e.g., app_autobrake_step, app_wipers_step) must execute within its defined cycle period of 10 ms, without exceeding the CPU budget.

Port Interface Data Types → All inputs/outputs exchanged between components (e.g., RainLevel %, VehicleSpeed, Distance_mm) must use AUTOSAR-defined fixed-width types (uint8, uint16, int16) instead of plain int or float.

Sender-Receiver Communication → Sensor values (RainLevel, Speed, Distance) must be received through sender-receiver ports with a freshness check (ignore values older than 100 ms).

Client-Server Communication → Actuator commands (BrakeRequest, WiperMode, FanStage) must be invoked as client-server ports with defined synchronous timing (response ≤ 20 ms).

Initialization Safety → Each SW-C must start in a safe default state (e.g., BrakeRequest = OFF, WiperMode = OFF, FanStage = 0) before valid sensor data is received.

Error Handling (DEM Integration) → If a sensor read fails more than 3 times in 1 second, the SW-C must report a Diagnostic Trouble Code (DTC) through AUTOSAR DEM.

Memory Safety → No dynamic memory allocation (malloc, free) is allowed inside SW-Cs; only static or stack-based variables are permitted.

Global Variable Access → All inter-SW-C communication must go via RTE ports, not by direct global variable access.

Coding Standards → All SW-Cs must pass static analysis with MISRA C:2012 rules enforced under AUTOSAR profile; no critical violations (Category 1) allowed in reports.

Configuration Consistency → For each SW-C, ARXML definitions of ports, interfaces, and runnable entities must match the implemented C code signatures exactly (e.g., RainLevel as uint8, not int).
Compact, battery-powered electronic device that controls a relay for switching an 110/240 VAC 6A load.

I. Description
This circuit is a compact battery-powered system based on an ATmega328P microcontroller that can monitor battery
voltage, control a relay, and provide user interaction through buttons and status indicators. The design emphasizes low
power consumption with a voltage regulator to provide stable power to the microcontroller.
Power Supply
- Three 1.5V AAA batteries connected in series provide 4.5V input power
- MCP1700 low-dropout regulator (U3) converts the battery voltage to a stable 3.3V for the microcontroller
- Decoupling capacitors (C1, C2, C3, C7) ensure clean power delivery
Microcontroller
- ATmega328P in TQFP package (U1) serves as the main controller
- Reset circuit with push button (SW2) and pull-up resistor (R7) allows for manual resets
- Analog input connected to a voltage divider monitors battery level
- Digital I/O pins control external components
Output Control
- Relay (K1) allows switching of external high-voltage/high-current loads
- Optocoupler (PC817, U2) provides electrical isolation between the microcontroller and relay driver
- NPN transistor (2N3904, Q2) amplifies the control signal to drive the relay coil
- Protection diode (1N4007, D2) prevents inductive kickback when the relay is de-energized
- LED indicator (D1) provides visual status feedback
User Interface
- Push button input (SW1) allows for user interaction
- LED output provides visual feedback on system status

II. Control: Microcontroller ATMEL ATmega328P
1. Low Power
- Active mode: ~5mA at 3.3V, 8MHz
- Power-down sleep mode: <1μA (with proper power management)
- If running at 1MHz, it can drop active current below 1mA.
- Sleep modes allow long battery life
2. Simple and Cheap
- Readily available and has a huge ecosystem of code and libraries.
- Modular
- Cost-efficient.
3. Adequate Performance for the Task
- 10-bit ADC sufficient for battery voltage monitoring.
- Computing power more than sufficient for current project requirements.
- Enough flash and RAM (32KB Flash / 2KB RAM) for basic sensor logging and control logic.
4. Ease of Development and Debugging
- Arduino-compatible for quick prototyping.
- Analog and digital pin flexibility (for sensing battery, controlling relays, etc.) - makes it great choice for further
project adaptations.
- AVR ISP / UPDI flashing - can be pre-programmed before soldering.
In current configuration assuming pulling up unused digital pins and running by using internal oscillator.
Considerations:
STM32L0 series (for example STM32L011K or other low power variant) is a very strong contender for an alternative MCU.
It could offer substantial processing power increase (modern Cortex-M0 32-bit processor), allow for easy integrations if
for example wireless communication was considered for the future, but also opens possibility for even cheaper
components when ordering in bulk/partnering with STM.
If low power chip with integrated Bluetooth LE was a requirement from the start, then nRF52832 is another great option.
Employing wireless functionality in any of these variants would make for an attractive product to compete with SONOFF
products for example.

III. Power source: 3 x AAA batteries + LDO regulator
Due to the requirement of having battery powered controlling device, that will be inherently ineffective when driving a
power-hungry coil of the relay, the simplest solution has been chosen in form of using 3 x AAA batteries. They are
designed to be connected in series, in order to boost voltage to 4.5V, which is then shifted down to MCU and relay
compatible 3.3V level using LDO regulator. Since it’s unlikely that any compact battery will be able to power circuit
satisfying the requirements for more than 7 days, choosing AAA battery was crucial due to ease of their replacement.
1. High popularity and availability
2. Very easy to replace
3. Relatively high capacity, but dependent on the quality of batteries (about 1200 mAh)
4. Enables possibility to commercially distribute the device without batteries (passing this requirement on to the
consumer)
Considerations:
Since this project would likely be used as a device used for switching mains power supply, then most logical would be
adding a switching power supply and filtering its output to power both MCU and relay coil, instead of using batteries.
For more premium devices (relay switching noise unwanted) double MOSFET (totem pole) configuration would be a
great solution, that could be actually run with batteries. If heat dissipation or low volume were critical factors, then it
would still be possible, but would require choosing more high-end MOSFET with lower Rds(on).

IV. Components, calculations, values.
All components have been chosen with the goal of being easily sourceable and replaceable. With further testing
adjustments towards limiting optocoupler and transistor current could and should be made, as that would allow
further energy savings.
Capacitors: All ceramic for decoupling purposes, values according to chip manufacturer’s guidelines.
Resistors: Same principle with exception of:
R2 = 1000 Ohms = 3.3 V / 0.0025 A – limiting current to about 2.5 mA, enough for BJT transistor operation.
R3 = 560 Ohms = 3.3 V / 0.005 A – goal of limiting opto-coupler current down to about 5 mA, to keep small overhead in
case of PC817 current transfer ratio drops.
R4 = 150 Ohms = 3.3 V – 1.2 V / 0.0025 A – goal to maintain about 15mA LED current, assuming standard diode’s 1.2 Vf.
Dependent on final components.
R5 = 36 kOhm R6 = 100kOhm – typical voltage divider for ADC that is designed to take 3.3V as a reference (AREF pin
grounded with C4 capacitor).
Vadc = Vbat × R6/ (R5+R6)
3.3V = 4.5V x R2 / (R5+R6)
R6 / (R5+R6) = 3.3V / 4.5V = 0.733
R6 = 0.733 × (R5+R6)
Designating R6 to be 100 kOhm and solving for R5:
R5 = (100 kOhm / 0.733) − 100kΩ ≈ 36 kOhm

V. Battery life
Assuming high current draw in 30% of the time - device idling (50% of the time) and off time when toggling relay.
ATmega328P Current Consumption
Active Mode (when relay is ON): ~5mA
Sleep Mode (when relay is OFF): ~0.1mA
Relay Consumption
ON (30% of the time): ~20mA – nominal relay current ~ 70mA
OFF (70% of the time): 0mA
Other consumers
Optocoupler + transistor sinking: 7.5mA
Average Current Calculation
(5mA+20mA+7.5mA) × 30% + (0.1mA) × 70% =
= (32.5mA × 30%) + (0.1mA × 70%) =
= 9.75mA + 0.07mA = 9.82 mA
Battery Life (1200mAh AAA in series)
1200 mAh ÷ (~9.82mA) = 122 hours = about 5 days

VI. Costs
Details in BOM.csv.
Estimate for making 5 pieces:
10,16 USD
Estimate for making 1000 pieces:
4,20 USD

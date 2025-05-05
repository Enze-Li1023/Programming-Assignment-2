# Programming-Assignment-2
# Selective Repeat Protocol Simulator

This project simulates the **Selective Repeat (SR)** reliable transport protocol using a custom event-driven network simulator.

##  Files

- `emulator.c` – The main simulation framework that handles events and manages time.
- `sr.c` – Your implementation of the Selective Repeat protocol (student-modifiable).
- `emulator.h` – Header file for the network emulator.
- `sr.h` – Header file for protocol logic definitions.

##  Compilation

To compile the program using GCC:

```bash
gcc -Wall -std=c99 -o sr emulator.c sr.c

./sr
# Running the Simulator
./sr
On Windows (Command Prompt or PowerShell):
sr

# Sample Run
-----  Stop and Wait Network Simulator Version 1.1 --------
Enter the number of messages to simulate: 10
Enter packet loss probability [enter 0.0 for no loss]: 0.1
Enter packet corruption probability [0.0 for no corruption]: 0.1
...
Simulator terminated at time 145.514160
after attempting to send 10 msgs from layer5
number of packet resends by A: 3
number of messages delivered to application: 12

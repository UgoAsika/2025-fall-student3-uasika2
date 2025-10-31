CSF Assignment 3: Cache Simulator
Milestone 1

Team Members:
Just Me (Ugo Asika)

Milestone 1 Description:
This submission compiles an executable named 'csim' using the provided makefile. The program currently parses and validates com line args for the cache config but we stil have not perfomed full simulation, this meetss all of mile stone 1s requirements.

Compilation: 

make clean && make

Execution Example:

./csim 256 4 16 write-allocate write-back lru < some_trace_file

Notes: 
The makefile uses required flags:-wall -wextra -pedantic -std=c++17
The program exits 0 on valid params, nonzero otherwise

Milestone 2

Description: This submisssion implements a functional cache simulator that supports all required cache parameters and policies. The program now reads memory traces from standard input and simulates cache behaviour using both LRU and FIFO replacement strategies.
It correctly tracks and reports:
    Total loads and stores
    Load hits and misses
    Store hits and misses
    Total cycles (within +- 5 tolerance)
The simualtor supports all combinations of:
    write-allocate/no-write-allocate
    write-through / write-back
    lru/FIFO


Milestone 3

Description:
This final milestone completes the cache simulator with all assignment specifications fully implemented and tested, the simulator now handles every valid cache config and policy combination correctly. It performs full memory access simulation with accurate cycle counting an good eviction logic for both LRU and FIFO policies.

Key Features Implemented:
* Handles variable sets, ways and block sizes (all powers of time)
* Supports both write allocate and no write allocate
* Supports both write through and write back
* Implements both LRU and FIFO replacement strategies

*Fully tracks and reports:
    - Total loads
    - Total stores
    -Load hits/misses
    -Store hits/misses
    -Total cycles (within +-5% tolerance)


    Best cache report:
        After testing across the provded traces ,
        the config that achieved the best overall balance of hit rate and total cycles was 
        256 sets by 4 ways by 16 byte blocks
        write - allocate + write - back + LRU replacement
        This set up achieved the best total runtime efficiency 
        


Compilation:

make clean && make

Execution Example:
./csim 256 4 16 write-allocate write-back lru < gcc.traces

Output example:
Total loads: <count>
Total stores: <count>
Load hits: <count>
Load mises: <count>
Store hits: <count>
Store misses: <count>
Total cycles: <count>

Notes:
The make file uses the requred flags
I kept the validation checks from Ms1 intact and in place
This milestone completes the LRU implementation and full cache simulation.


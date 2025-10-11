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


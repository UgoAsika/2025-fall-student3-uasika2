CONTRIBUTIONS
Ugo Asika uasika2

This project was done by me (and my partner if any). I wrote the mmap and fork parts for parallel quicksort. i also tested the program with different threshold values and fixed a few bugs with child processes not exiting right. 
Testing the timing runs and verifying the results with is_sorted and seqsort.

REPORT

i tested the program using the run_experiments.sh script on a 16mb randomdata file. below are the results from the time command (real time in seconds):

threshold    |  real time (s)
------------- | --------------
2097152       |  2.84
1048576       |  2.35
524288        |  1.97
262144        |  1.72
131072        |  1.58
65536         |  1.54
32768         |  1.50
16384         |  1.49

as the threshold got smaller, the sort time got faster at first because more of the recursive calls ran in parallel on different cpu cores. after a point, the improvement stopped because making too many child processes adds overhead and they start competing for the same cpu cores. the best performance was around the middle thresholds where the number of child processes matched the number of available cores.

in short, the results show good parallel speedup at first but then leveling off when process creation becomes too much overhead.

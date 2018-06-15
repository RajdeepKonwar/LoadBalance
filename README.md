# LoadBalance
Load balance computation of sine values of an unknown number of angles across MPI ranks.

## Compile Instruction
```
mpicxx LoadBalance.cpp -o LoadBalance -std=c++11 -O3
```

## Run Instruction
```
mpirun -n 4 ./LoadBalance
```
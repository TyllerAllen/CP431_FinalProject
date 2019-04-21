# CP431_Project
Parallel Programming Final Project -- N x N values

Objective is to calculate the number of unique values in an N x N multiplication table.

Compile using:

  mpicc main.c -o main
  
Run using:

  mpirun -np <number of processors> main <N>

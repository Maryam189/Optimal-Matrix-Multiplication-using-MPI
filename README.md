# Optimal-Matrix-Multiplication-using-MPI
In this course project, I utilized MPI (Message Passing Interface) to address a complex problem in parallel and distributed computing involving matrix operations. 

## Project Overview
This repository contains a project demonstrating optimal matrix multiplication using MPI (Message Passing Interface). The implementations cover blocking, non-blocking, and Strassen algorithms to illustrate different approaches to parallel matrix multiplication.

## Installation
To set up this project locally, follow these steps:

```bash
git clone https://github.com/Maryam189/Optimal-Matrix-Multiplication-using-MPI.git
cd Optimal-Matrix-Multiplication-using-MPI
```

## Usage
Compile and run the application using MPI with the following commands:

```bash
mpicc <filename>.c -o <outputname>
mpiexec -n <number_of_processes> ./<outputname>
```

# Performance Analysis

## Blocking Matrix Multiplication Execution Time

### One Machine Results:
- 2 processes: ~0.0146 seconds
- 4 processes: ~0.0034 seconds
- 6 processes: ~0.0144 seconds
- 8 processes: ~0.0353 seconds

### Two Machines Results:
- 2 processes: ~0.0008 seconds
- 4 processes: ~1.1635 seconds
- 6 processes: ~0.0643 seconds
- 8 processes: ~0.1038 seconds

## Non-Blocking Matrix Multiplication Execution Time

### One Machine Results:
- 2 processes: ~0.0020 seconds
- 4 processes: ~0.0408 seconds
- 6 processes: ~0.0568 seconds
- 8 processes: ~0.1223 seconds

### Two Machines Results:
- 2 processes: ~0.0002 seconds
- 4 processes: ~0.0457 seconds
- 6 processes: ~0.605 seconds
- 8 processes: ~0.8745 seconds

# Conclusion
The diverse results in execution times across different algorithms and configurations demonstrate the complexities and trade-offs in parallel computing. The blocking and non-blocking approaches show varying efficiency depending on the number of processes and the type of machine setup, whereas the Strassen algorithm offers stability but at higher execution times.

This README is ready to be used in your repository. Make sure to replace `<filename>.c`, `<outputname>`, and `<number_of_processes>` with the actual filenames and desired process numbers for your specific project. If there's anything else you'd like to add or modify, let me know. Feel free to reach me out at Maryamkhalid590@gmail.com


# Parallel-Systems-Projects


A collection of projects developed as part of the Parallel Systems course, exploring parallel programming techniques, concurrency, and performance optimization using C.

The projects cover different approaches to parallel computing, including **POSIX Threads (Pthreads), OpenMP, and MPI**, with implementations of computational problems using both sequential and parallel algorithms.

### Projects

**1. Multithreading with POSIX Threads (Pthreads)**
- Practical exercises exploring concurrent programming using threads.
- Thread creation, synchronization, and parallel task execution.

**2. Shared-Memory Parallelism with OpenMP**
- **Conway's Game of Life:** Sequential and parallel implementations using OpenMP to distribute grid computations across multiple threads.
- **Gaussian Elimination:** Implementation of sequential and parallel algorithms using both row-oriented and column-oriented approaches.
- Performance evaluation through execution-time measurements and different thread configurations.

**3. Distributed-Memory Parallelism with MPI**
- **Conway's Game of Life:** Distributed grid computation using MPI, including data distribution, inter-process communication, and ghost rows.
- **Matrix-Vector Multiplication:** Parallel computation through column-wise matrix distribution, custom MPI datatypes, and MPI reduction operations.
- Performance evaluation using different numbers of processes and problem sizes.

**4. Completion of the NVIDIA course: Getting Started with Accelerated Computing in CUDA C/C++**

### Technologies
- C
- POSIX Threads (Pthreads)
- OpenMP
- MPI (Message Passing Interface)
- Makefiles
- Parallel performance analysis and benchmarking

### Objectives
The main objective of these projects is to understand different parallel programming models, implement parallel algorithms, explore communication and synchronization between concurrent execution units, and compare their performance against sequential implementations.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include "files.h"

#define SOFTENING 1e-9f

typedef struct { float x, y, z, vx, vy, vz; } Body;

//bodyForce kernel (each GPU thread computes the forces for a single body)
__global__ void bodyForce(Body *p, float dt, int n) {

    //calculate index i to assign each GPU thread a unique body 
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    
    float Fx = 0.0f, Fy = 0.0f, Fz = 0.0f;
    
    for (int j = 0; j < n; j++) {
        float dx = p[j].x - p[i].x;
        float dy = p[j].y - p[i].y;
        float dz = p[j].z - p[i].z;
        float distSqr = dx*dx + dy*dy + dz*dz + SOFTENING;
        float invDist = rsqrtf(distSqr);
        float invDist3 = invDist * invDist * invDist;

        Fx += dx * invDist3;
        Fy += dy * invDist3;
        Fz += dz * invDist3;
    }

    p[i].vx += dt * Fx;
    p[i].vy += dt * Fy;
    p[i].vz += dt * Fz;
}

//integratePosition kernel 
__global__ void integratePosition(Body *p, float dt, int n) {
    //calculate index i to assign each GPU thread a unique body 
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    
    p[i].x += p[i].vx * dt;
    p[i].y += p[i].vy * dt;
    p[i].z += p[i].vz * dt;
}

int main(const int argc, const char** argv) {
    int nBodies = 2<<11;
    if (argc > 1) nBodies = 2<<atoi(argv[1]);

    const char * initialized_values = (nBodies == 2<<11) ? "09-nbody/files/initialized_4096" : "09-nbody/files/initialized_65536";
    const char * solution_values = (nBodies == 2<<11) ? "09-nbody/files/solution_4096" : "09-nbody/files/solution_65536";
    if (argc > 2) initialized_values = argv[2];
    if (argc > 3) solution_values = argv[3];

    const float dt = 0.01f;
    const int nIters = 10;
    int bytes = nBodies * sizeof(Body);
    Body *p, *d_p;

    
    p = (Body*)malloc(bytes);
    cudaMalloc(&d_p, bytes); //allocate GPU memory
    read_values_from_file(initialized_values, (float*)p, bytes);
    cudaMemcpy(d_p, p, bytes, cudaMemcpyHostToDevice); //tranfer memory from CPU to GPU

    int threadsPerBlock = 256;
    int blocksPerGrid = (nBodies + threadsPerBlock - 1) / threadsPerBlock; //calculate the blocks per grid

    double totalTime = 0.0;

    for (int iter = 0; iter < nIters; iter++) {

        StartTimer();
        
        //run bodyForce as a CUDA kernel to run in parallel 
        bodyForce<<<blocksPerGrid, threadsPerBlock>>>(d_p, dt, nBodies);
        cudaDeviceSynchronize();

        //run integratePosition as a CUDA kernel to run in parallel 
        integratePosition<<<blocksPerGrid, threadsPerBlock>>>(d_p, dt, nBodies);
        cudaDeviceSynchronize();
        
        const double tElapsed = GetTimer() / 1000.0;
        totalTime += tElapsed;

    }

    //transfer memory from GPU back to CPU
    cudaMemcpy(p, d_p, bytes, cudaMemcpyDeviceToHost);
    write_values_to_file(solution_values, (float*)p, bytes);

    double avgTime = totalTime / (double)(nIters);
    float billionsOfOpsPerSecond = 1e-9 * nBodies * nBodies / avgTime;
    printf("%0.3f Billion Interactions / second\n", billionsOfOpsPerSecond);

    //free allocated memory
    free(p);
    cudaFree(d_p);
}

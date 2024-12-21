#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <iostream>
#include <getopt.h>
#include <functional>
#include <cuda_runtime_api.h>


// Set to 0 to disable debugging
#define DDEBUG 1
#define MAX_GPU_THREADS 256

// CUDA Helper Directives
#define checkCudaErrors(call)                                           \
    do {                                                                \
        cudaError_t err = call;                                         \
        if (err != cudaSuccess) {                                       \
            std::cerr << "CUDA error at " << __FILE__ << " "            \
                      << __LINE__ << ": " << cudaGetErrorString(err)    \
                      << std::endl;                                     \
            std::exit(EXIT_FAILURE);                                    \
        }                                                               \
    } while (0)

typedef struct {
    std::vector<std::int32_t> left;
    std::vector<std::int32_t> right;
} InputData;

// Performs final bitonic check and swaps if conditions aren't meant
__device__ void checkAndSwap(int32_t* arr, int32_t current_position, int32_t other_position) {
    // Store in registers to avoid multiple global reads
    int32_t current = arr[current_position], other = arr[other_position];
    // Perform the swap
    if (other < current) {
        arr[current_position] = other;
        arr[other_position] = current;
    }
}

// Generates bitonic sequences within the array (note: we do this for both arrays)
__global__ void sortIntoBitonicSequences(int32_t* left, int32_t* right, size_t size, size_t mask) {
    size_t current_position = blockIdx.x * blockDim.x + threadIdx.x;
    if (current_position < size) {
        size_t other_position = current_position ^ mask;
        if (current_position < other_position && other_position < size) {
            checkAndSwap(left, current_position, other_position);
            checkAndSwap(right, current_position, other_position);
        }
    }
}

// Does a parallel reduction by taking different of two arrays at a given position
__global__ void differenceReduction(int32_t* left, int32_t* right, int32_t* answer, size_t size) {
    extern __shared__ int32_t shared_memory[];
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    size_t threadIndex = threadIdx.x;

    // Store in shared memory (less reads to global memory)
    if (index < size) {
        shared_memory[threadIndex] = abs(left[index] - right[index]);
    } else {
        shared_memory[threadIndex] = 0;
    }
    __syncthreads();

    // Perform parallel reduction (similarly to a tree converging to the root)
    for (size_t stride = 1; stride < blockDim.x; stride = stride << 1) {
        if ((threadIndex % (stride << 1)) == 0) {
            shared_memory[threadIndex] += shared_memory[threadIndex + stride];
        }
        __syncthreads();
    }

    // Write result atomically to global memory
    if (threadIndex == 0) {
        atomicAdd(answer, shared_memory[threadIndex]);
    }

}

__global__ void findFrequencies(int32_t* frequencies, int32_t* right, int32_t size) {
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < size) {
        atomicAdd(&frequencies[right[index]], 1);
    }

}

// Does a parallel reduction for similarlity score
__global__ void similarityScoreReduction(int32_t* frequencies, int32_t* left, int32_t* answer, size_t size) {
    extern __shared__ int32_t shared_memory[];
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    size_t threadIndex = threadIdx.x;

    // Store in shared memory (less reads to global memory)
    if (index < size) {
        shared_memory[threadIndex] = left[index] * frequencies[left[index] ];
    } else {
        shared_memory[threadIndex] = 0;
    }
    __syncthreads();

    // Perform parallel reduction (similarly to a tree converging to the root)
    for (size_t stride = 1; stride < blockDim.x; stride = stride << 1) {
        if ((threadIndex % (stride << 1)) == 0) {
            shared_memory[threadIndex] += shared_memory[threadIndex + stride];
        }
        __syncthreads();
    }

    // Write result atomically to global memory
    if (threadIndex == 0) {
        atomicAdd(answer, shared_memory[threadIndex]);
    }

}

void solvePartOne(std::unique_ptr<InputData>& data) {
    if (data->left.size() != data->right.size()) {
        std::cerr << "Error: Sizes don't match for left and right array" << std::endl;
    }

    std::int32_t* host_answer = nullptr;
    std::int32_t* device_answer = nullptr;
    std::int32_t* device_left = nullptr;
    std::int32_t* device_right = nullptr;
    size_t size = data->left.size();
    std::size_t nThreads = MAX_GPU_THREADS;
    std::size_t nBlocks = (size + nThreads - 1) / nThreads;

    // Allocate pinned host memory
    checkCudaErrors(cudaHostAlloc(&host_answer, sizeof(std::int32_t), cudaHostAllocDefault));

    // Allocate device memory and copy over the unsorted array to device
    checkCudaErrors(cudaMalloc(&device_answer, sizeof(std::int32_t)));
    checkCudaErrors(cudaMalloc(&device_left, sizeof(std::int32_t) * size));
    checkCudaErrors(cudaMalloc(&device_right, sizeof(std::int32_t) * size));
    checkCudaErrors(cudaMemcpy(device_left, data->left.data(), sizeof(std::int32_t) * size, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(device_right, data->right.data(), sizeof(std::int32_t) * size, cudaMemcpyHostToDevice));

    // This is moditification of biotonic sort to work on non-powers of 2
    // k is the size of the bitonic sequence (starting with base case solved k = 1)
    // It will iterate from size 2, 4, 8, 16....
    // Below is an example (i: increasing, d: decreasing, >: start):
    // > [ 2 9 4 1 6 3 0 5 ]
    //   [ 2 9 4 1 3 6 5 0 ]
    //     i   d   i   d
    //   [ 1 2 4 9 6 5 3 0 ]
    //     i       d
    //   [ 0 1 2 3 4 5 6 9 ]
    //     i
    for (std::size_t k = 2; (k >> 1) < size; k = k << 1) {
        // Deals with the elements that don't fall into the power of 2 case (forward comparator)
        sortIntoBitonicSequences<<<nThreads, nBlocks>>>(device_left, device_right, size, k - 1);
        // Main forward comparator
        for (std::size_t j = k >> 1; j > 0; j = j >> 1) {
            sortIntoBitonicSequences<<<nThreads, nBlocks>>>(device_left, device_right, size, j);
        }
    }

    // Performs a parallel reduction (this is done considering the different between left and right list)
    differenceReduction<<<nThreads, nBlocks, MAX_GPU_THREADS * sizeof(std::int32_t)>>>(device_left, device_right, device_answer, size);

    // Copy back the answer from GPU to CPU
    checkCudaErrors(cudaMemcpy(host_answer, device_answer, sizeof(std::int32_t), cudaMemcpyDeviceToHost));
    std::cout << "Part 1 Answer: " << *host_answer << std::endl;

    // Copy the sorted data back to the host and free device memory
    checkCudaErrors(cudaFree(device_answer));
    checkCudaErrors(cudaFree(device_left));
    checkCudaErrors(cudaFree(device_right));
}

void solvePartTwo(std::unique_ptr<InputData>& data) {
    if (data->left.size() != data->right.size()) {
        std::cerr << "Error: Sizes don't match for left and right array" << std::endl;
    }

    std::int32_t* host_answer = nullptr;
    std::int32_t* device_answer = nullptr;
    std::int32_t* device_left = nullptr;
    std::int32_t* device_right = nullptr;
    std::int32_t* device_frequencies = nullptr;
    size_t size = data->left.size();
    size_t max_number = 150000;
    std::size_t nThreads = MAX_GPU_THREADS;
    std::size_t nBlocks = (size + nThreads - 1) / nThreads;

    // Allocate pinned host memory
    checkCudaErrors(cudaHostAlloc(&host_answer, sizeof(std::int32_t), cudaHostAllocDefault));

    // Allocate device memory and copy over the unsorted array to device
    checkCudaErrors(cudaMalloc(&device_answer, sizeof(std::int32_t)));
    checkCudaErrors(cudaMalloc(&device_left, sizeof(std::int32_t) * size));
    checkCudaErrors(cudaMalloc(&device_right, sizeof(std::int32_t) * size));
    checkCudaErrors(cudaMalloc(&device_frequencies, sizeof(std::int32_t) * max_number));
    checkCudaErrors(cudaMemcpy(device_left, data->left.data(), sizeof(std::int32_t) * size, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(device_right, data->right.data(), sizeof(std::int32_t) * size, cudaMemcpyHostToDevice));

    // Performs parallel frequency counting
    findFrequencies<<<nThreads, nBlocks>>>(device_frequencies, device_right, size);

    // Performs similarity score calcuating with parallel reduction
    similarityScoreReduction<<<nThreads, nBlocks>>>(device_frequencies, device_left, device_answer, size);

    // Copy back the answer from GPU to CPU
    checkCudaErrors(cudaMemcpy(host_answer, device_answer, sizeof(std::int32_t), cudaMemcpyDeviceToHost));
    std::cout << "Part 2 Answer: " << *host_answer << std::endl;

    // Copy the sorted data back to the host and free device memory
    checkCudaErrors(cudaFree(device_answer));
    checkCudaErrors(cudaFree(device_left));
    checkCudaErrors(cudaFree(device_right));
    checkCudaErrors(cudaFree(device_frequencies));
}

void usage(char* argv[]) {
    std::cout << "Usage: " << argv[0] <<" [OPTIONS]" << std::endl
              << "Options:" << std::endl
              << "  -f, --file <file>    Advent of code sample file" << std::endl
              << "  -debug, --debug      Sets debug flag" << std::endl;

    return;
}

std::string parseArgs(std::int32_t& argc, char* argv[]) {
    struct option longOptions[] = {{"file", required_argument, NULL, 'f'}};
    std::string filename;
    char flag;

    while ((flag = getopt_long(argc, argv, "f:", longOptions, NULL)) != -1) {
        switch (flag) {
            case 'f':
                filename = std::string(optarg);
                break;
            default:
                usage(argv);
        }
    }

    if (filename.empty()) {
        usage(argv);
        exit(EXIT_FAILURE);
    }

    return filename;
}

void parseFile(std::string& filename, 
               std::unique_ptr<InputData>& data, 
               std::function<void(std::istringstream, std::unique_ptr<InputData>&)> func) {
                
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error: Couldn't open file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    while (std::getline(file, line)) {
        func(std::istringstream(line), data);
    }

    return;
}

void dayOneParse(std::istringstream iss, std::unique_ptr<InputData>& data) {
    std::int32_t left_num, right_num;
    iss >> left_num >> right_num;
    data->left.push_back(left_num);
    data->right.push_back(right_num);
    return;
}

int main(std::int32_t argc, char* argv[]) {
    std::unique_ptr<InputData> data = std::make_unique<InputData>();
    std::string filename = parseArgs(argc, argv);
    parseFile(filename, data, dayOneParse);

    // Day 1: Sort and find difference between positions on list
    solvePartOne(data);

    // Day 2: Count frequencies and find similarlity score
    solvePartTwo(data);
}
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/unistd.h>

#include "shared/shared.hpp"
#include "accelerator.hpp"
#define MAP_SIZE (1024UL*1024UL)
void hw_accelerator(int target,             // control register target
                   float * mem,            // global memory pointer

                    unsigned int inputByteOffset,       // offset of inputs in BYTES
                    unsigned int outputByteOffset,      // offset of outputs in BYTES
                    unsigned int parametersByteOffset,  // offset of parameters in BYTES
                    const unsigned int batchSize,            // batch size
                    const bool useReLu, //whether to use ReLu (ALL)

                    layerType type,

                    const unsigned int k,           // output number of kernels (CONV, POOL), or number of outputs (FC)
                    const unsigned int n,           // output width (CONV)
                    const unsigned int m,           // output height (CONV)
                    const unsigned int c,           // input number of channels  (CONV, POOL), or number of inputs (FC)
                    const unsigned int w,           // input width (CONV, POOL)
                    const unsigned int h,           // input height (CONV, POOL)
                    const unsigned int stride,            // stride (CONV, POOL)
                    const unsigned int kernelSize ,        // kernel size (CONV, POOL)
                    const unsigned int pad  //pad size (CONV, POOL)
                    )
{

    volatile void* map_base;
    const char * pPath = getenv("XDMA");

    const char * ctrl_device_0 = "/dev/xdma0_user";
    const char * dma_to_device_0 = "/dev/xdma0_h2c_0";
    const char * dma_from_device_0 = "/dev/xdma0_c2h_0";

    const char * ctrl_device_1 = "/dev/xdma1_user";
    const char * dma_to_device_1 = "/dev/xdma1_h2c_0";
    const char * dma_from_device_1 = "/dev/xdma1_c2h_0";


    // Setup control registers
    int ctrl_fd;
    if(strcmp(pPath, "/dev/xdma0") == 0)
      ctrl_fd =  open((const char *)ctrl_device_0, O_RDWR | O_SYNC);
    else
      ctrl_fd =  open((const char *)ctrl_device_1, O_RDWR | O_SYNC);
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ctrl_fd, 0);
    printf("Memory mapped at address %p.\n", map_base);

    // Write arguments
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_INPUTBYTEOFFSET_DATA, inputByteOffset);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_OUTPUTBYTEOFFSET_DATA, outputByteOffset);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_PARAMETERSBYTEOFFSET_DATA, parametersByteOffset);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_BATCHSIZE_DATA, batchSize);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_USERELU_DATA, useReLu);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_TYPE_R_DATA, type);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_K_DATA, k); //output number of kernels
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_N_DATA, n);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_M_DATA, m);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_C_DATA, c);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_W_DATA, w);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_H_DATA, h);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_STRIDE_DATA, stride);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_KERNELSIZE_DATA, kernelSize);
    write_int(map_base, target + XACCELERATOR_CTRL_BUS_ADDR_PAD_DATA, pad);

    char* in_buffer = NULL;
    char* allocated = NULL;
    // Size to DMA in
    //int size = id*od*k*k+od+id*ix*iy*b;
    int size = c*k*kernelSize*kernelSize + k + h*w*c*batchSize;
    int wait_time = 0;

    // Create aligned memory alloc for DMA (should do this during initial read)
    posix_memalign((void **)&allocated, 4096/*alignment*/, size*sizeof(float) + 4096);
    in_buffer = allocated;
    memcpy(in_buffer, (void*)mem, size*sizeof(float));
    printf("Copied input to buffer\n");

    // DMA input values
    int dma_to_device_fd;
    if(strcmp(pPath, "/dev/xdma0") == 0)
      dma_to_device_fd = open((const char *)dma_to_device_0, O_RDWR | O_NONBLOCK);
    else
       dma_to_device_fd = open((const char *)dma_to_device_1, O_RDWR | O_NONBLOCK);

    //uint32_t addr = inputByteOffset;
    uint32_t addr = 0;
    lseek(dma_to_device_fd, addr, SEEK_SET);
    write(dma_to_device_fd, (char*)in_buffer, size*sizeof(float));
    free(in_buffer);
    printf("Wrote inputs via DMA\n");

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    write_int(map_base, target, 0x1);
    do {
      sleep(1);
      printf("\rSleep wait %d", wait_time++);
      fflush(stdout);
    }
    while (!(read_int(map_base, target) & 0xe));
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    timespec_sub(&ts_end, &ts_start);
    printf("CLOCK_MONOTONIC reports %ld.%09ld seconds (total) for core\n", ts_end.tv_sec, ts_end.tv_nsec);


    // DMA outputs back
    char *out_buffer = NULL;
    int dma_from_device_fd;
    if(strcmp(pPath, "/dev/xdma0") == 0)
      dma_from_device_fd  = open((const char *)dma_from_device_0, O_RDWR | O_NONBLOCK);
    else
      dma_from_device_fd  = open((const char *)dma_from_device_1, O_RDWR | O_NONBLOCK);
    //int out_size = od*oy*ox*b;
    int out_size = k*m*n*batchSize;

    posix_memalign((void **)&allocated, 4096/*alignment*/, out_size*sizeof(float) + 4096);
    out_buffer = allocated;
    lseek(dma_from_device_fd, outputByteOffset, SEEK_SET);
    read(dma_from_device_fd, out_buffer, out_size*sizeof(float));
    printf("Outputs read via DMA\n");

    // copy results to result buffer
    memcpy((void*)(mem+size), out_buffer, out_size*sizeof(float));
    free(out_buffer);
    close(dma_from_device_fd);
    close(dma_to_device_fd);
}

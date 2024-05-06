#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <errno.h>

#define DEVICE_PATH "/dev/ringbuf"

#define RINGBUF_IOC_MAGIC 'q'
#define RINGBUF_IOCRESET  _IO(RINGBUF_IOC_MAGIC, 0)
#define RINGBUF_IOCGETSTATUS _IOR(RINGBUF_IOC_MAGIC, 1, int)

#define BUF_SIZE 1024
#define THREAD_COUNT 1000
#define OPERATIONS_PER_THREAD 1000

void *thread_function(void *arg) {
    int fd, i;
    char write_buf[] = "Hello, ring buffer thread!";
    char read_buf[sizeof(write_buf)];
    ssize_t ret;

    fd = open(DEVICE_PATH, O_RDWR);
    if(fd < 0) {
        perror("Failed to open the device");
        pthread_exit(NULL);
    }

    for(i=0; i < OPERATIONS_PER_THREAD; i++) {
        ret = write(fd, write_buf, strlen(write_buf));
        if(ret < 0) {
            fprintf(stderr, "Thread %ld: Failed to write to the devic: %s\n", (long)arg, strerror(errno));
            break;
        }

        lseek(fd, 0, SEEK_SET);
        ret = read(fd, read_buf, sizeof(read_buf));
        if(ret < 0) {
            fprintf(stderr, "Thread %ld: Failed to read from the device: %s\n", (long)arg, strerror(errno));
        }
    }

    close(fd);
    pthread_exit(NULL);
}
int main(){
    int fd;
    char write_buf[] = "Hello, ring buffer!!!";
    char read_buf[sizeof(write_buf)];
    int buffer_status;
    pthread_t threads[THREAD_COUNT];
    
    fd = open(DEVICE_PATH, O_RDWR);
    if(fd < 0) {
        perror("Failed to open the device");
        return errno;
    }
    char *buffer = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(buffer == MAP_FAILED){
        perror("mmap");
        close(fd);
        return -1;
    }


    if(write(fd, write_buf, strlen(write_buf)) < 0) {
        perror("Failed to write the device");
        close(fd);
        return errno;
    }
    printf("Written to device: '%s'\n", write_buf);
    
    ioctl(fd, RINGBUF_IOCGETSTATUS, &buffer_status);
    printf("Buffer contains %d characters\n", buffer_status);

    lseek(fd, 0, SEEK_SET);

    if(read(fd, read_buf, sizeof(read_buf)) < 0) {
        perror("Failed to read from the device");
        close(fd);
        return errno;
    }

    printf("Read from device: '%s'\n", read_buf);


    ioctl(fd, RINGBUF_IOCGETSTATUS, &buffer_status);
    printf("Buffer contains %d characters\n", buffer_status);

    strcpy(buffer, "Hello, mmap!");
    printf("Buffer mmap-ed: '%s'\n", buffer);

    // Unmap when done
    munmap(buffer, BUF_SIZE);
    // Close the device
    close(fd);

    for(int i = 0; i < THREAD_COUNT; i++) {
        if(pthread_create(&threads[i], NULL, thread_function, (void *)(long)i) != 0) {
            perror("Failed to create thread");
            return EXIT_FAILURE;
        }
    }
    for(int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("Stress test completed\n");
    return 0;
}
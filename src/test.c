#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/syslimits.h>
#include <unistd.h>
#include <errno.h>

#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>

uint32_t* get_pids(uint32_t* size) {
    uint16_t number_of_pids = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    if(number_of_pids < 0) {
        printf("Error with retrieving number of pids: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    uint32_t *buffer = malloc(sizeof(uint32_t) * number_of_pids);
    uint16_t count = proc_listpids(PROC_ALL_PIDS, 0, buffer, sizeof(buffer) * number_of_pids);

    uint32_t j = 0;
    for(int i = 0; i < count; i++) {
        if(buffer[i] != 0) {
            buffer[j++] = buffer[i];
        }
    }
    buffer[j++] = 0;

    buffer = (uint32_t* ) realloc(buffer, j * sizeof(uint32_t));
    *size = j;
    return buffer;
}

char* get_path_from_pid(pid_t pid) {
    char* file_name = malloc(PATH_MAX * sizeof(char));
    memset(file_name, '\0', sizeof(PATH_MAX));
    int ret = proc_pidpath(pid, file_name, sizeof(char) * PATH_MAX);
    file_name = (char* ) realloc(file_name, ret * sizeof(char));
    if(ret < 0) {
        printf("Error with retrieving file path of pid %d: %s", pid,strerror(errno));
        exit(EXIT_FAILURE);
    }
    return file_name;

}

uint32_t get_pid_from_path(char* path) {
    uint32_t size;
    uint32_t* pidlist = get_pids(&size);
    for(int i = 0; i < size; i++) {
        char* name = get_path_from_pid(pidlist[i]);
        if (strstr(name, path) != NULL) {
            free(name);
            free(pidlist);
            return pidlist[i];
        }
        free(name);
    }
    free(pidlist);
    return -1;
}

mach_port_t get_task(pid_t pid) {
    mach_port_t task;
    kern_return_t kern_return = task_for_pid( mach_task_self(), pid, &task);
    if(kern_return != KERN_SUCCESS) {
        printf("task_for_pid() failed: %s\n", mach_error_string( kern_return ) );
        return 0;
    }
    return task;
}

void* read_process_memory(vm_map_read_t task, mach_vm_address_t address, mach_vm_size_t size) {
    // MAKE SURE WE HAVE PERMISSION TO READ - change later
    /*
    kern_return_t ret = mach_vm_protect(task, address, size, true, VM_PROT_READ);
    if (ret != KERN_SUCCESS) {
        printf("mach_vm_protect() failed: %s\n", mach_error_string(ret));
    }
    */

    // READ MEMORY
    vm_offset_t data;
    mach_msg_type_number_t dataCnt;

    kern_return_t ret = mach_vm_read(task, address, size, &data, &dataCnt);
    if(ret != KERN_SUCCESS) {
        printf("mach_vm_read() failed: %s\n", mach_error_string(ret));
    }
    return (void*) data;
}

void write_process_memory(vm_map_read_t task, mach_vm_address_t address, vm_offset_t data, mach_vm_size_t dataCnt) {

    kern_return_t ret = mach_vm_write(task, address, data, dataCnt);
    if(ret != KERN_SUCCESS) {
        printf("mach_vm_write() failed: %s\n", mach_error_string(ret));
    }
}
int main(int argc, char** argv) {
    printf("%d\n", getpid());
    uint32_t data = 511232;
    uint32_t size;
    uint32_t* pidlist = get_pids(&size);
    pid_t pid = get_pid_from_path("Python");
    mach_port_t task = get_task(46053);
    write_process_memory(task, 0x104cf5408, (vm_offset_t) &data, 5);
    uint32_t* addy = read_process_memory(task, 0x104cf5408, 5);
    printf("%d\n", *addy);

    return EXIT_SUCCESS;
}
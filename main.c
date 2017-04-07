/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Global Variables
const char *alg;
int npages;
int nframes;
struct disk *disk;
char *virtmem;
char *physmem;
int *frameTable;


// Functions
void page_fault_handler( struct page_table *pt, int page )
{
    // map pages to frames    eventually more pages than frames
    int i; // going through the page table
    int frame;
    int bits;
    virtmem = page_table_get_virtmem(pt);
    physmem = page_table_get_physmem(pt);

    for (i = 0; i < npages; i++){
        page_table_get_entry(pt, i, &frame, &bits);
        printf("frame: %d \tbits: %d \n", frame, bits);
       if (bits == 0){
            frameTable[i] = frame;
            page_table_set_entry(pt, i, frame, PROT_READ);
            disk_read(disk, i, &physmem[frame*bits]);
           // break;
        }
        page_table_get_entry(pt, i, &frame, &bits);
        printf("frame: %d \tbits: %d \n", frame, bits);
    }
    printf("page is %d\n",page );
  // write functions that performs a linear search to check if the physical memory is empty
  // write a map thingy for frame, bits, dirty bit, & page
  // if an item has a dirty bit, we have to update the disk before overwriting it
  // 
  
    printf("page fault on page #%d\n",page);
    exit(1);
}

// Main
int main( int argc, char *argv[] )
{
    if(argc!=5) {
        printf("use: virtmem <npages> <nframes> <rand|fifo|lru|custom> <sort|scan|focus>\n");
        return 1;
    }

    npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    const char *program = argv[4];
    alg = argv[3];

    frameTable = malloc(nframes *sizeof(int));
    int i; //what frame is in this page
    for (i = 0; i < nframes; i++){
        frameTable[i] = -1;
    }
//    printf("npages: %d\n", npages);
//    printf("nframes: %d\n", nframes);

//    struct disk *disk = disk_open("myvirtualdisk",npages);
    disk = disk_open("myvirtualdisk",npages);

    if(!disk) {
        fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
        return 1;
    }
    
    struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
    if(!pt) {
        fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
        return 1;
    }

    virtmem = page_table_get_virtmem(pt);
    physmem = page_table_get_physmem(pt);
    
  /*if(!strcmp(alg,"rand")) {
        rand_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(alg,"fifo")) {
        fifo_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(alg,"custom")) {
        custom_program(virtmem,npages*PAGE_SIZE);

    } else {
        fprintf(stderr,"unknown algorithm: %s\n",argv[3]);
        return 1;
    }*/

    if(!strcmp(program,"sort")) {
        sort_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(program,"scan")) {
        scan_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(program,"focus")) {
        focus_program(virtmem,npages*PAGE_SIZE);

    } else {
        fprintf(stderr,"unknown program: %s\n",argv[4]);
        return 1;
    }

    page_table_delete(pt);
    disk_close(disk);
    free(frameTable);

    return 0;
}

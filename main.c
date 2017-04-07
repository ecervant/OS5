/*
 * Main program for the virtual memory project.
 * Make all of your modifications to this file.
 * You may add or rearrange any code or data as you need.
 * The header files page_table.h and disk.h explain
 * how to use the page table and disk interfaces.
 * */

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int npages;
int nframes;
struct disk *disk;
int *frameTable; // frameTable[iframe] = -1: unset, 0: set
char *virtmem; 
char *physmem;
int total_reads = 0;
int total_writes = 0;
const char * alg;
const char * program;
int pagefaults = 0;

int *replacement_frame;
int * replacement_frame2;
int full = 0;

int *pageTable; // pageTable[frame] = page;

void page_fault_handler( struct page_table *pt, int page )
{
    int frame, bits;
    pagefaults++;


    page_table_get_entry(pt, page, &frame, &bits);
    if ( bits == 0 && full == 0){
        int i;
        for(i = 0; i < nframes; i++){
            if (frameTable[i] == -1){
                page_table_set_entry(pt, page, i, PROT_READ);
                disk_read(disk, page, &physmem[i*PAGE_SIZE]);
                total_reads++;
                frameTable[i] = 1;
                pageTable[i]=page;
                break;
            }
        }
      
        if (i == nframes - 1) full = 1;
    }
    
    else if (bits == PROT_READ){
        page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
        frameTable[frame] = 1; // added
        pageTable[frame] = page; // added
    }
    
    else if (bits == 0 && full == 1){
        int kick_out;
        int new_frame;
      
        if(!strcmp(alg,"rand")) {
            new_frame = rand() % nframes;
            kick_out = pageTable[new_frame];
            page_table_get_entry(pt, kick_out, &frame, &bits);
                
            if (bits == (PROT_READ|PROT_WRITE)) { // use compare to check for writing
                disk_write(disk, kick_out, &physmem[new_frame*PAGE_SIZE]);
                total_writes++;
            }

        
            disk_read(disk, page, &physmem[new_frame*PAGE_SIZE]);
            page_table_set_entry(pt, page, new_frame, PROT_READ);
            page_table_set_entry(pt, kick_out, new_frame, 0);
        
            frameTable[new_frame] = 1;
            pageTable[new_frame] = page;
            total_reads++;
        } 
        else if(!strcmp(alg,"fifo")) {
            page_table_get_entry(pt, page, &frame, &bits);
            new_frame = replacement_frame[0];
            kick_out = pageTable[new_frame];    
            int s; // keep track
            for (s = 0; s < nframes-1; s++){
                replacement_frame[s] = replacement_frame[s+1];

            }
            replacement_frame[nframes-1] = new_frame;

            page_table_get_entry(pt, kick_out, &frame, &bits);
            
            if (bits == (PROT_READ|PROT_WRITE)) { // use compare to check for writing
                disk_write(disk, kick_out, &physmem[new_frame*PAGE_SIZE]);
                total_writes++;
            }
            
            if (frameTable[new_frame] != -1){
                disk_read(disk, page, &physmem[new_frame*PAGE_SIZE]);
                page_table_set_entry(pt, page, new_frame, PROT_READ);
                page_table_set_entry(pt, kick_out, new_frame, 0);
        
                total_reads++;
                frameTable[new_frame] = 1;

                pageTable[new_frame] = page;
            }

        } 
        
        else if(!strcmp(alg,"custom")) {
            
            if (!strcmp(program, "sort")){
                new_frame = rand() % nframes;
                new_frame = rand() % nframes;

                kick_out = pageTable[new_frame];
                page_table_get_entry(pt, kick_out, &frame, &bits);
                
                if (bits == (PROT_READ|PROT_WRITE)) { // use compare to check for writing
                    disk_write(disk, kick_out, &physmem[new_frame*PAGE_SIZE]);
                    total_writes++;
                }
        
                disk_read(disk, page, &physmem[new_frame*PAGE_SIZE]);
                page_table_set_entry(pt, page, new_frame, PROT_READ);
                page_table_set_entry(pt, kick_out, new_frame, 0);
        
                frameTable[new_frame] = 1;
                pageTable[new_frame] = page;
                total_reads++;




            }
            else{
                
                page_table_get_entry(pt, page, &frame, &bits);
                new_frame = nframes-1;
                kick_out = pageTable[new_frame];
                    
                int s; // keep track
                for (s = 0; s < nframes-1; s++){
                    replacement_frame2[s] = replacement_frame2[s+1];
                }
                replacement_frame2[nframes-1] = new_frame;
    
                page_table_get_entry(pt, kick_out, &frame, &bits);
                
                if (bits == (PROT_READ|PROT_WRITE)) { // use compare to check for writing
                    disk_write(disk, kick_out, &physmem[new_frame*PAGE_SIZE]);
                    total_writes++;
                }
                
                if (frameTable[new_frame] != -1){
                    disk_read(disk, page, &physmem[new_frame*PAGE_SIZE]);
                    page_table_set_entry(pt, page, new_frame, PROT_READ);
                    page_table_set_entry(pt, kick_out, new_frame, 0);
            
                    total_reads++;
                    frameTable[new_frame] = 1;
    
                    pageTable[new_frame] = page;
            
                }
            }
        }
        else {
            fprintf(stderr,"unknown algorithm: %s\n",alg);
            exit(1);
        }
        
    }
    else { 
        exit(0);
    }  
  

}

int main( int argc, char *argv[] )
{
    if(argc!=5) {
        printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
        return 1;
    }

    npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    program = argv[4];
    alg = argv[3];


    frameTable = malloc(nframes * sizeof(int));
    pageTable = malloc(npages * sizeof(int));
    replacement_frame = malloc(nframes * sizeof(int));
    replacement_frame2 = malloc(nframes * sizeof(int));

    int i;
    for (i=0; i < nframes; i++){
         frameTable[i] = -1; // frame unset
    }
 
    for (i = 0; i < nframes; i++){
          replacement_frame[i] = i;
    }
    for (i = 0; i < nframes; i++){
          replacement_frame2[i] = nframes-1-i;
    }

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

    if(!strcmp(program,"sort")) {
        sort_program(virtmem,npages*PAGE_SIZE);
    } 
    else if(!strcmp(program,"scan")) {
        scan_program(virtmem,npages*PAGE_SIZE);

     } 
    else if(!strcmp(program,"focus")) {
        focus_program(virtmem,npages*PAGE_SIZE);

    } 
    else {
        fprintf(stderr,"unknown program: %s\n",argv[3]);
        return 1;
    }
     
    printf("page faults: %d\n", pagefaults);
    printf("disk reads: %d\n", total_reads);
    printf("disk writes: %d\n", total_writes);
     
    page_table_delete(pt);
    disk_close(disk);


    free(frameTable);
    free(pageTable);
    free(replacement_frame);
    free(replacement_frame2);
    return 0;
}


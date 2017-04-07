/*
 * Main program for the virtual memory project.
 * Make all of your modifications to this file.
 * You may add or rearrange any code or data as you need.
 * The header files page_table.h and disk.h explain
 * how to use the page table and disk interfaces.
 * */

// Project 5: Virtual Memory
// Jessica Cioffi and Esmeralda Cervantes
// jcioffi & ecervant

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Global Variables

int npages; // number of pages
int nframes; // number of frames
struct disk *disk; // instance of disk struct
int *frameTable; // our frame table to keep track of frames
char *virtmem; // virtual memory
char *physmem; // physical memory
int total_reads = 0; // total reads performed
int total_writes = 0; // total writes performed
const char * alg; // the algorithm selected by the user
const char * program; // the program selected by the user
int pagefaults = 0; // the number of pagefaults

int *replacement_frame; // the replacement frame for fifo
int * replacement_frame2; // the replacement fram for custom 
int full = 0; // if the physical memory is full

int *pageTable; // our page table to keep track of pages

// Functions
void page_fault_handler( struct page_table *pt, int page )
{
    int frame, bits;
    pagefaults++;

    page_table_get_entry(pt, page, &frame, &bits);
    if ( bits == 0 && full == 0){ // if there is no dirty bit and physical memory is not full
        int i;
        for(i = 0; i < nframes; i++){ // initialize most of our parameters
            if (frameTable[i] == -1){
                page_table_set_entry(pt, page, i, PROT_READ);
                disk_read(disk, page, &physmem[i*PAGE_SIZE]);
                total_reads++;
                frameTable[i] = 1;
                pageTable[i]=page;
                break;
            }
        }
      
        if (i == nframes - 1) full = 1; // once we get to the end of physical memory, set to full
    }
    
    else if (bits == PROT_READ){ // if the bits signifies a protected read
        page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE); // set the page table entry accordingly
        frameTable[frame] = 1;
        pageTable[frame] = page; 
    }
    
    else if (bits == 0 && full == 1){ // if there was no protected read or write, and physical memort is full
        int kick_out;
        int new_frame;
      
        // choosing between algorithms depending on user input
        if(!strcmp(alg,"rand")) { // perform rand
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
        else if(!strcmp(alg,"fifo")) { // perform fifo
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
        else if(!strcmp(alg,"custom")) { // perform our custom sort
            
            if (!strcmp(program, "sort")){
                new_frame = rand() % nframes; // performed the double seeded rand
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
            else{ // performs the lifo implementation   
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
        else { // if the string compare does not match the user entry, exit
            fprintf(stderr,"unknown algorithm: %s\n",alg);
            exit(1);
        }
        
    }
    else { 
        exit(0);
    }  
}

// Main
int main( int argc, char *argv[] )
{
    if(argc!=5) { // account for user error
        printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
        return 1;
    }

    // Initialize the user entries to variable values
    npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    program = argv[4];
    alg = argv[3];

    // malloc space for our tables and frame measurements
    frameTable = malloc(nframes * sizeof(int));
    pageTable = malloc(npages * sizeof(int));
    replacement_frame = malloc(nframes * sizeof(int));
    replacement_frame2 = malloc(nframes * sizeof(int));

    // Initialize all of the tables and frame measurements
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

    // open the virtual disk
    disk = disk_open("myvirtualdisk",npages);
    if(!disk) { // if you can't open, error out
         fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
         return 1;
    }

    // creates the page table struct
    struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
    if(!pt) { // if you can't create, error out
        fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
        return 1;
     }

    // delcare virtual and physical mem
    virtmem = page_table_get_virtmem(pt);
    physmem = page_table_get_physmem(pt);

    // perform string compares for the program of the user's choice
    if(!strcmp(program,"sort")) {
        sort_program(virtmem,npages*PAGE_SIZE);
    } 
    else if(!strcmp(program,"scan")) {
        scan_program(virtmem,npages*PAGE_SIZE);

     } 
    else if(!strcmp(program,"focus")) {
        focus_program(virtmem,npages*PAGE_SIZE);

    } 
    else { // error out if the input does not match
        fprintf(stderr,"unknown program: %s\n",argv[3]);
        return 1;
    }
     
    // correctly formatted output
    printf("page faults: %d\n", pagefaults);
    printf("disk reads: %d\n", total_reads);
    printf("disk writes: %d\n", total_writes);

    // delete the table and close the disk    
    page_table_delete(pt);
    disk_close(disk);

    // free up all the space we used
    free(frameTable);
    free(pageTable);
    free(replacement_frame);
    free(replacement_frame2);
    return 0;
}


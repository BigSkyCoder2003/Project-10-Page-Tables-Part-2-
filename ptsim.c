#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1;  // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//

int setup_page_table(){
int page_table = 0;
    for (int i = 1; i < PAGE_COUNT; i++) {
        if (mem[i] == 0) {
            page_table = i;
            mem[i] = 1;
            return page_table;
        }
    }
    return 0;
}


void find_free_pages(int proc_num, int page_count, int page_table){
for (int i = 0; i < page_count; i++) {
        int page = 0;
        for (int j = 1; j < PAGE_COUNT; j++) {
            if (mem[j] == 0) {
                page = j;
                mem[j] = 1;
                break;
            }
        }
        if(page == 0)
        {
          printf("OOM: proc %d: page %d\n", proc_num, i);
          exit(1);
        }
        //page table
        mem[get_address(page_table, i)] = page;
    }
}

void new_process(int proc_num, int page_count){
    // Allocate a page table page
    int page_table = 0;
    page_table = setup_page_table();
    if(page_table == 0)
    {
      printf("OOM: proc %d: page table\n", proc_num);
      exit(1);
    }
    
    //allocate process page(s)
    find_free_pages(proc_num, page_count, page_table);

    //page table pointer
    mem[get_address(0, PTP_OFFSET + proc_num)] = page_table;
    //printf("page table page:%d", mem[get_address(0, PTP_OFFSET + proc_num)]);

}

void free_process_pages(int page_table){
for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);
        int page = mem[addr];
        if (page != 0) {
            mem[page] = 0;
        }
    }
}

void free_process_page_table(int proc_num, int page_table){
mem[page_table] = 0;
mem[get_address(0, PTP_OFFSET + proc_num)] = 0;
}

void kill_process(int proc_num){
    int page_table = get_page_table(proc_num);
    free_process_pages(page_table);
    free_process_page_table(proc_num, page_table);
}

int vaddr_to_paddr(int proc_num, int vaddr)
{
int page_table = get_page_table(proc_num);

  int virtual_page = vaddr >> PAGE_SHIFT;
  int offset = vaddr & 255;
  int page_table_entry = mem[get_address(page_table, virtual_page)];
  return (page_table_entry << PAGE_SHIFT) | offset;
}

void store_value(int proc_num, int vaddr, int value){
  
  int phys_addr = vaddr_to_paddr(proc_num, vaddr);

  mem[phys_addr] = value;

 printf("Store proc %d: %d => %d, value=%d\n",
    proc_num, vaddr, phys_addr, value);   
}

void get_value(int proc_num, int vaddr){
    
  int phys_addr = vaddr_to_paddr(proc_num, vaddr);

  int value = mem[phys_addr];

  printf("Load proc %d: %d => %d, value=%d\n",
    proc_num, vaddr, phys_addr, value);
}


//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++) {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];
        
        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }
    
    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "pfm") == 0) {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0) {
            int proc_num = atoi(argv[++i]);
            int page_count = atoi(argv[++i]);
            new_process(proc_num, page_count);
        }
        else if (strcmp(argv[i], "kp") == 0) {
            int proc_num = atoi(argv[++i]);
            kill_process(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0) {
            int proc_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);
            int value = atoi(argv[++i]);
            store_value(proc_num, vaddr, value);
        }
        else if (strcmp(argv[i], "lb") == 0) {
            int proc_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);
            get_value(proc_num, vaddr);
        }
            
// sb n a b: For process n at virtual address a, store the value b.
// lb n a: For process n, get the value at virtual address a.

        // TODO: more command line arguments
    }
}

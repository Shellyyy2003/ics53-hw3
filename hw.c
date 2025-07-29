/*
    Authors:
        Thomas Nguyen 20843831
        Yiwen Wu – 42326616
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VM_SIZE 128
#define MEM_SIZE 32
#define PAGE_SIZE 8
#define NUM_VPAGES (VM_SIZE / PAGE_SIZE)
#define NUM_PPAGES (MEM_SIZE / PAGE_SIZE)

typedef struct {
    int valid;
    int dirty;
    int page_number; // main memory ppn if valid, else disk page num (== vpn)
} PageTableEntry;

int disk[VM_SIZE];
int memory[MEM_SIZE];
int priority[NUM_PPAGES];
PageTableEntry page_table[NUM_VPAGES];
char* algorithm = "FIFO";
int oldest = 0;
int full = 0;

void update_LRU(int low) { // Updates LRU priorities
    for (int i = 0; i < NUM_PPAGES; i++) {
        if (priority[i] > low)
            priority[i] -= 1;
    }
}

void fill_memory(int vpn, int ppn) { // Fills Memory from Disk
    int vaddr = vpn * PAGE_SIZE;
    int paddr = ppn * PAGE_SIZE;
    for (int i = 0; i < PAGE_SIZE; i++) {
        memory[paddr + i] = disk[vaddr + i];
    }
}

void copy_memory(int vpn, int ppn) { // Copies Memory onto the Disk
    int vaddr = vpn * PAGE_SIZE;
    int paddr = ppn * PAGE_SIZE;
    for (int i = 0; i < PAGE_SIZE; i++) {
        disk[vaddr + i] = memory[paddr + i];
    }
}

int page_fault_handler(int vpn) {
    int temp = -1; // Returns at function end (Done for 100% line coverage)
    if (strcmp(algorithm, "LRU") == 0) { // LRU
        if (full < NUM_PPAGES) { // Main memory still empty
            update_LRU(1);
            priority[full] = full + 1;

            fill_memory(vpn, full);

            return full++;
        }
        else { // Evict and replace least recently used
            for (int i = 0; i < NUM_PPAGES; i++) {
                if (priority[i] == 1) {
                    update_LRU(1);
                    priority[i] = NUM_PPAGES;

                    for (int j = 0; j < NUM_VPAGES; j++) { // Find VPN for page to be evicted
                        if (page_table[j].valid) {
                            if (page_table[j].page_number == i) { // Found Page
                                if (page_table[j].dirty) {
                                    copy_memory(j, i);
                                }
                                fill_memory(vpn, i);

                                page_table[j].page_number = j; // Reset page entry of evicted
                                page_table[j].valid = 0;
                                page_table[j].dirty = 0;
                            }
                        }
                    }
                    temp = i;
                    break; // Break loop and return page number
                }
            }
        }
    }
    else { // FIFO
        if (full < NUM_PPAGES) { // Main memory still empty
            priority[full] = 1;

            fill_memory(vpn, full);

            return full++;
        }
        else { // Evict and replace oldest
            for (int j = 0; j < NUM_VPAGES; j++) { // Find VPN for page to be evicted
                if (page_table[j].valid) {
                    if (page_table[j].page_number == oldest) { // Found Page
                        if (page_table[j].dirty) {
                            copy_memory(j, oldest);
                        }
                        fill_memory(vpn, oldest);

                        page_table[j].page_number = j; // Reset page entry of evicted
                        page_table[j].valid = 0;
                        page_table[j].dirty = 0;

                        temp = oldest;
                        oldest = (oldest + 1) % NUM_PPAGES;
                        break;
                    }
                }
            }
        }
    }
    return temp;
} // returns physical page number

// Util: Virtual address → (vpn, offset)
void translate(int vaddr, int* vpn, int* offset) {
    *vpn = vaddr / PAGE_SIZE;
    *offset = vaddr % PAGE_SIZE;
}

// Read
void handle_read(int vaddr) {
    if (vaddr < 0 || vaddr >= VM_SIZE) {
        printf("Invalid virtual address\n");
        return;
    }
    int vpn, offset;
    translate(vaddr, &vpn, &offset);

    if (!page_table[vpn].valid) {
        printf("A Page Fault Has Occurred\n");
        page_table[vpn].page_number = page_fault_handler(vpn);
        page_table[vpn].valid = 1;
        page_table[vpn].dirty = 0;
    }

    int ppn = page_table[vpn].page_number;
    int phys_addr = ppn * PAGE_SIZE + offset;
    printf("%d\n", memory[phys_addr]);
    if (strcmp(algorithm, "LRU") == 0) { // Added to handle LRU
        update_LRU(priority[ppn]);
        priority[ppn] = NUM_PPAGES;
    }
}

// Write
void handle_write(int vaddr, int value) {
    if (vaddr < 0 || vaddr >= VM_SIZE) {
        printf("Invalid virtual address\n");
        return;
    }
    int vpn, offset;
    translate(vaddr, &vpn, &offset);

    if (!page_table[vpn].valid) {
        printf("A Page Fault Has Occurred\n");
        page_table[vpn].page_number = page_fault_handler(vpn);
        page_table[vpn].valid = 1;
        page_table[vpn].dirty = 0;
    }

    int ppn = page_table[vpn].page_number;
    int phys_addr = ppn * PAGE_SIZE + offset;
    memory[phys_addr] = value;
    page_table[vpn].dirty = 1;
    if (strcmp(algorithm, "LRU") == 0) { // Added to handle LRU
        update_LRU(priority[ppn]);
        priority[ppn] = NUM_PPAGES;
    }
}

void handle_showmain(int ppn) {
    if (ppn < 0 || ppn >= NUM_PPAGES) {
        printf("Invalid physical page number\n");
        return;
    }
    int base = ppn * PAGE_SIZE;
    for (int i = 0; i < PAGE_SIZE; i++) {
        printf("%d: %d\n", base + i, memory[base + i]);
    }
}

void handle_showptable() {
    for (int i = 0; i < NUM_VPAGES; i++) {
        printf("%d:%d:%d:%d\n", i, page_table[i].valid, page_table[i].dirty, page_table[i].page_number);
    }
}

void initialize_system() {
    for (int i = 0; i < VM_SIZE; i++) {
        disk[i] = -1;
    }
    for (int i = 0; i < MEM_SIZE; i++) {
        memory[i] = -1;
    }
    for (int i = 0; i < NUM_VPAGES; i++) {
        page_table[i].valid = 0;
        page_table[i].dirty = 0;
        page_table[i].page_number = i; // maps to disk initially
    }
    for (int i = 0; i < NUM_PPAGES; i++) {
        priority[i] = -1;
    }
}

int main(int argc, char* argv[]) {
    initialize_system();
    
    // Moved algorithm init to global
    if (argc == 2) {
        if (strcmp(argv[1], "LRU") == 0) // Algorithm only changes if LRU, else FIFO
            algorithm = argv[1];
        else if (strcmp(argv[1], "FIFO") != -1)
            printf("Invalid page replacement algorithm\n");
    }
    printf("Using %s replacement algorithm\n", algorithm);

    char cmd[100];
    printf("> ");
    while (fgets(cmd, sizeof(cmd), stdin)) {
        char op[20];
        int a1;
        int a2;
        if (sscanf(cmd, "%s %d %d", op, &a1, &a2) >= 1) {
            if (strcmp(op, "read") == 0) {
                handle_read(a1);
            } else if (strcmp(op, "write") == 0) {
                handle_write(a1, a2);
            } else if (strcmp(op, "showmain") == 0) {
                handle_showmain(a1);
            } else if (strcmp(op, "showptable") == 0) {
                handle_showptable();
            } else if (strcmp(op, "quit") == 0) {
                break;
            } else {
                printf("Unknown command\n");
            }
        }
        printf("> ");
    }

    return 0;
}

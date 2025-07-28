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
PageTableEntry page_table[NUM_VPAGES];

// two functions need to be implemented
int page_fault_handler(int vpn); // returns physical page number
void evict_page_if_needed();     // if full, evict; else do nothing

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
}

int main(int argc, char* argv[]) {
    initialize_system();
    
    const char* algorithm = "FIFO";
    if (argc == 2) {
        algorithm = argv[1];
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

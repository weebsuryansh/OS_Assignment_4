#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
void *entryPointAddress = NULL;
int fd;
void *mem;
int temp=0;
typedef int (*EntryPoint)();
bool flag=true;
size_t pageNum=0;
size_t pageSize=4096;
size_t rem=0;
size_t totalSize=0;
int pagefault=0;
int totPage=0;
int totFragmentation=0;
bool ppnExists=false;
int ppnCount=0;
void *ppn[100000];
void *vpn[100000];
void *fault_addr;
off_t phyoffset=0;
bool tempflag=true;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
free(ehdr);
free(phdr);
close(fd);
}

void segfault_handler(int signo, siginfo_t *info, void *context) {
    tempflag=true;
    //updating t            printf("%p\n",phyoffset);he number of pagefaults that happened;
    pagefault++;

    // getting the address for seg fault
    void *f_addr = info->si_addr;
    printf("%p\n",phyoffset);
    if(phyoffset==0){
        fault_addr=f_addr;
    }
    else{
        //fixing the physical address  to the virtual address
        for (int i=0; i<10000;i++){
            if(ppn[i]==f_addr){
                ppnExists=true;
                fault_addr=vpn[i];
                break;
            }
        }
        //if it doesn't exist in ppn-vpn table then, it's added
        if (!ppnExists){
            printf("yes\n");
            fault_addr=f_addr-phyoffset;
            ppn[ppnCount]=f_addr;
            vpn[ppnCount++]=fault_addr; 
        }
    }
    ppnExists=false;
  
    //printing out the address at segmentation fault is occuring
    printf("Segmentation fault at ppn address: %p\n", f_addr);
    printf("Segmentation fault at vpn address: %p\n", fault_addr);

    // usleep(2500);
    // sleep(1);

    //iterating over the phdr
    for (int i=0; i<ehdr->e_phnum;i++){
        //checking the segment in which the fault address resides
        if (((int)fault_addr) >= phdr[i].p_vaddr && ((int)fault_addr) < phdr[i].p_vaddr + phdr[i].p_memsz) {
            tempflag=false;
            printf("working\n");
            //calculating the number of pages required
            pageNum=phdr[i].p_memsz/pageSize;
            rem=phdr[i].p_memsz%pageSize;
            if (rem>0){
                pageNum++;
            }
            //updating total pages allocated
            totPage+=(int)pageNum; 

            totalSize=pageNum*pageSize;

            //allocating the pages
            mem = mmap(
                NULL,
                totalSize,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS,
                0,
                0
            );
            temp=i;
            if (mem == MAP_FAILED) {
                perror("mmap failed");
                exit(1);
            }

            //sertting the offset
            phyoffset= (off_t)mem-(off_t)phdr[i].p_vaddr;
            // if(phyoffset==0){
            //     phyoffset= (off_t)mem-(off_t)phdr[i].p_vaddr;
            //     printf("%p\n",phyoffset);
            // }
            // else{
            //     phyoffset= (off_t)f_addr-(off_t)fault_addr;
            //     printf("%p\n",phyoffset);
            // }

            //copying the segment
            lseek(fd, 0, SEEK_SET);
            lseek(fd, phdr[i].p_offset, SEEK_SET);
            ssize_t bytesRead = read(fd, mem, phdr[i].p_filesz);
            //setting the physical addr for fault_addr
            off_t entryOffset = ((int)fault_addr) - phdr[i].p_vaddr;
            fault_addr = (char *)mem + entryOffset;
            // printf("Internal fragmentation: %zu\n", (totalSize-phdr[i].p_memsz));
            totFragmentation+=(int)(totalSize-phdr[i].p_memsz); //calculating total fragmentation
            // printf("memory: %p\n", (char *)mem);
            // printf("segment size: %p\n", phdr[i].p_memsz);
            // printf("page size allocated: %p\n", totalSize);
            // printf("fault address: %p\n", fault_addr);
            break;
        } 
        
    }
    if(tempflag){
        exit(0);
    }
    // get the context
    ucontext_t *ucontext = (ucontext_t *)context;
    // Update the instruction pointer (EIP) to resume execution
    ucontext->uc_mcontext.gregs[14] = (uintptr_t)fault_addr;
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
    //opening the elf file
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    //reading the elf header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    ssize_t bytesRead = read(fd, ehdr, sizeof(Elf32_Ehdr));

    //moving pointer to phdr offset
    off_t phdrOffset = ehdr->e_phoff;
    lseek(fd, phdrOffset, SEEK_SET);

    //reading program header
    phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));
    bytesRead = read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));

    //typecasting entry point
    entryPointAddress=(char *)ehdr->e_entry;
    
    //calling the function which will give segfault
    int (*start)() = (EntryPoint)entryPointAddress;
    int result = start();
    printf("The result is %d\n", result); 
}


int main(int argc, char** argv) 
{
    struct sigaction sa;
    sa.sa_sigaction = segfault_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    if(argc != 2) {
        printf("Usage: %s <ELF Executable> \n",argv[0]);
        exit(1);
    }
    // 1. carry out necessary checks on the input ELF file
    // 2. passing it to the loader for carrying out the loading/execution
    load_and_run_elf(argv);
    // 3. invoke the cleanup routine inside the loader  
    printf("1. No. of page faults:\t\t\t%d\n", pagefault);
    printf("2. No. of pages allocated:\t\t%d\n", totPage);
    printf("3. Total internal fragmentation:\t%.3fKB\n", (float)totFragmentation/1024);
    loader_cleanup();
    return 0;
}

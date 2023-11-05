/*
 * No changes are allowed to this file
 */

#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdbool.h>
#include <ucontext.h>
#include <sys/ucontext.h>


void load_and_run_elf(char** exe);
void loader_cleanup();

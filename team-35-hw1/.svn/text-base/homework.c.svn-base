/*
 * file:        homework.c
 * description: Skeleton for homework 1
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, Jan. 2012
 * $Id: homework.c 500 2012-01-15 16:15:23Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fuse.h>
#include "uprog.h"

/***********************************/
/* Declarations for code in misc.c */
/***********************************/

typedef int *stack_ptr_t;
extern void init_memory(void);
extern void do_switch(stack_ptr_t *location_for_old_sp, stack_ptr_t new_value);
extern stack_ptr_t setup_stack(int *stack, void *func);
extern int get_term_input(char *buf, size_t len);
extern void init_terms(void);

extern void  *proc1;
extern void  *proc1_stack;
extern void  *proc2;
extern void  *proc2_stack;
extern void **vector;


/***********************************************/
/********* Your code starts here ***************/
/***********************************************/

// Global variables

// These are to hold the arguments for functions
int argc;
char argv[10][128];

stack_ptr_t saved_stack_main;
stack_ptr_t saved_stack_proc1;
stack_ptr_t saved_stack_proc2;





int load_at_proc(char *cmd, void *location)
{
    // Try to outpen cmd to ptr_cmd.
    FILE *ptr_cmd = fopen(cmd, "r");
    if (!ptr_cmd) {
        perror("can't open file ");
        printf("%s", cmd);
        return 0;
    }

    // This is to get the size of file ptr_q1prog. Got hints from
    // http://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(ptr_cmd, 0, SEEK_END);     // seek to end of file
    int size = ftell(ptr_cmd);       // get current file pointer
    fseek(ptr_cmd, 0, SEEK_SET);     // seek back to beginning of file

    // Read the file into the specified memory proc1.
    fread(location, sizeof(char), size, ptr_cmd);

    return 1;
}

/*
 * Question 1.
 *
 * The micro-program q1prog.c has already been written, and uses the
 * 'print' micro-system-call (index 0 in the vector table) to print
 * out "Hello world".
 *
 * You'll need to write the (very simple) print() function below, and
 * then put a pointer to it in vector[0].
 *
 * Then you read the micro-program 'q1prog' into memory starting at
 * address 'proc1', and execute it, printing "Hello world".
 *
 */
void print(char *line)
{
    /*
     * Your code goes here.
     */
    printf("%s", line);
}

void q1(void)
{
    /*
     * Your code goes here. Initialize the vector table, load the
     * code, and go.
     */
    // Initialize the vector table.
    vector[0] = print;

    load_at_proc("q1prog", proc1);

    // Jump to the address and execute.
    void (*run)(void) = proc1;
    run();
}


/*
 * Question 2.
 *
 * Add two more functions to the vector table:
 *   void readline(char *buf, int len) - read a line of input into 'buf'
 *   char *getarg(int i) - gets the i'th argument (see below)

 * Write a simple command line which prints a prompt and reads command
 * lines of the form 'cmd arg1 arg2 ...'. For each command line:
 *   - save arg1, arg2, ... in a location where they can be retrieved
 *     by 'getarg'
 *   - load and run the micro-program named by 'cmd'
 *   - if the command is "quit", then exit rather than running anything
 *
 * Note that this should be a general command line, allowing you to
 * execute arbitrary commands that you may not have written yet. You
 * are provided with a command that will work with this - 'q2prog',
 * which is a simple version of the 'grep' command.
 *
 * NOTE - your vector assignments have to mirror the ones in vector.s:
 *   0 = print
 *   1 = readline
 *   2 = getarg
 */
void readline(char *buf, int len) /* vector index = 1 */
{
    /*
     * Your code here.
     */
    // Read file stdin into buf. fgets will always produce a NULL ended
    // str.
    if (fgets(buf, len, stdin) != NULL) {
        // printf("%s\n", buf);
    } else if (feof(stdin)) {
        printf("Reached the end of file");
    } else if (ferror(stdin)) {
        perror("fgets()");
    }
}

/* find the next word starting at 's', delimited by characters
* in the string 'delim', and store up to 'len' bytes into *buf
* returns pointer to immediately after the word, or NULL if done.
*/
char *strwrd(char *s, char *buf, size_t len, char *delim)
{
    s += strspn(s, delim);
    int n = strcspn(s, delim);  /* count the span (spn) of bytes in */
    if (len - 1 < n)            /* the complement (c) of *delim */
        n = len - 1;
    memcpy(buf, s, n);
    buf[n] = 0;
    s += n;
    return (*s == 0) ? NULL : s;
}

char *getarg(int i)     /* vector index = 2 */
{
    /*
     * Your code here.
     */
    // The argv[0] is actually the cmd. So the first argument is at
    // argv[1].
    if (argv[i + 1][0] == '\0')
        return 0;
    return argv[i + 1];
}

/*
 * Note - see c-programming.pdf for sample code to split a line into
 * separate tokens.
 */
void q2(void)
{
    /* Your code goes here */
    vector[0] = print;
    vector[1] = readline;
    vector[2] = getarg;

    while (1) {
        /* get a line */
        int len = 1024;
        char buf[len];
        char *ptr_buf = buf;
        readline(buf, len);
        /* split it into words */
        argc = -1;
        for (argc = 0; argc < 10; argc++) {
            ptr_buf = strwrd(ptr_buf, argv[argc], sizeof(argv[argc]), " \t\n");
            if (ptr_buf == NULL)
                break;
        }
        /* if zero words, continue */
        if (!strlen(argv[0]))
            continue;
        // if first word is "quit", break
        char *firstWord = getarg(-1);
        if (strcmp(firstWord, "quit") == 0)
            break;
        /* make sure 'getarg' can find the remaining words */
        /* load and run the command */
        else {
            // Actuall argc should be minus 1.
            argc -= 1;

            // Jump to the address and execute.
            if (load_at_proc(firstWord, proc2)) {
                void (*run)(void) = proc2;
                run();
            }
        }
    }
    /*
     * Note that you should allow the user to load an arbitrary command,
     * and print an error if you can't find and load the command binary.
     */
}

/*
 * Question 3.
 *
 * Create two processes which switch back and forth.
 *
 * You will need to add another 3 functions to the table:
 *   void yield12(void) - save process 1, switch to process 2
 *   void yield21(void) - save process 2, switch to process 1
 *   void uexit(void) - return to original homework.c stack
 *
 * The code for this question will load 2 micro-programs, q3prog1 and
 * q3prog2, which are provided and merely consists of interleaved
 * calls to yield12() or yield21() and print(), finishing with uexit().
 *
 * Hints:
 * - Use setup_stack() to set up the stack for each process. It returns
 *   a stack pointer value which you can switch to.
 * - you need a global variable for each process to store its context
 *   (i.e. stack pointer)
 * - To start you use do_switch() to switch to the stack pointer for
 *   process 1
 */

void yield12(void)      /* vector index = 3 */
{
    /* Your code here */
    do_switch(&saved_stack_proc1, saved_stack_proc2);
}

void yield21(void)      /* vector index = 4 */
{
    /* Your code here */
    do_switch(&saved_stack_proc2, saved_stack_proc1);
}

void uexit(void)        /* vector index = 5 */
{
    /* Your code here */
    // The old location could be anywhere actually. I set it to proc1.
    do_switch(&saved_stack_proc1, saved_stack_main);
}

void q3(void)
{
    /* Your code here */
    // Initialize the vectors.
    vector[0] = print;
    vector[1] = readline;
    vector[2] = getarg;
    vector[3] = yield12;
    vector[4] = yield21;
    vector[5] = uexit;
    /* load q3prog1 and q3prog2 into proc1 and proc2. Then set function process 1 and process 2 at location proc1 and proc2*/
    load_at_proc("q3prog1", proc1);
    load_at_proc("q3prog2", proc2);

    // Setup for both micro-programs.
    saved_stack_proc1 = setup_stack(proc1_stack, proc1);
    saved_stack_proc2 = setup_stack(proc2_stack, proc2);

    // Give control from main to process1.
    do_switch(&saved_stack_main, saved_stack_proc1);
}


/***********************************************/
/*********** Your code ends here ***************/
/***********************************************/

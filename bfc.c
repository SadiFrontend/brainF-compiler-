/*
 * Brainfuck Compiler
 * Compiles Brainfuck code to x86-64 assembly (AT&T syntax)
 * Supports all 8 Brainfuck operations: + - > < . , [ ]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_CODE_SIZE 1000000
#define MEMORY_SIZE 30000

typedef struct {
    char *code;
    size_t length;
    size_t position;
} Source;

typedef struct {
    FILE *output;
    int label_counter;
    int *loop_stack;
    int loop_stack_top;
    int loop_stack_size;
} Compiler;

// Error handling
void error(const char *msg, int pos) {
    fprintf(stderr, "Error at position %d: %s\n", pos, msg);
    exit(1);
}

// Initialize compiler
Compiler *create_compiler(const char *output_file) {
    Compiler *c = malloc(sizeof(Compiler));
    if (!c) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    c->output = fopen(output_file, "w");
    if (!c->output) {
        fprintf(stderr, "Could not open output file: %s\n", output_file);
        exit(1);
    }
    
    c->label_counter = 0;
    c->loop_stack_size = 100;
    c->loop_stack = malloc(sizeof(int) * c->loop_stack_size);
    c->loop_stack_top = -1;
    
    return c;
}

// Push loop label to stack
void push_loop(Compiler *c, int label) {
    if (c->loop_stack_top >= c->loop_stack_size - 1) {
        c->loop_stack_size *= 2;
        c->loop_stack = realloc(c->loop_stack, sizeof(int) * c->loop_stack_size);
    }
    c->loop_stack[++c->loop_stack_top] = label;
}

// Pop loop label from stack
int pop_loop(Compiler *c) {
    if (c->loop_stack_top < 0) {
        error("Unmatched ']'", 0);
    }
    return c->loop_stack[c->loop_stack_top--];
}

// Get next label number
int next_label(Compiler *c) {
    return c->label_counter++;
}

// Emit assembly header
void emit_header(Compiler *c) {
    fprintf(c->output, "    .section .data\n");
    fprintf(c->output, "memory:\n");
    fprintf(c->output, "    .zero %d\n\n", MEMORY_SIZE);
    
    fprintf(c->output, "    .section .text\n");
    fprintf(c->output, "    .globl _start\n\n");
    fprintf(c->output, "_start:\n");
    fprintf(c->output, "    # Initialize data pointer in r12\n");
    fprintf(c->output, "    leaq memory(%%rip), %%r12\n\n");
}

// Emit assembly footer
void emit_footer(Compiler *c) {
    fprintf(c->output, "\n    # Exit program\n");
    fprintf(c->output, "    movq $60, %%rax      # sys_exit\n");
    fprintf(c->output, "    xorq %%rdi, %%rdi    # exit code 0\n");
    fprintf(c->output, "    syscall\n");
}

// Compile single instruction
void compile_instruction(Compiler *c, char instruction) {
    switch (instruction) {
        case '+':
            fprintf(c->output, "    incb (%%r12)         # +\n");
            break;
            
        case '-':
            fprintf(c->output, "    decb (%%r12)         # -\n");
            break;
            
        case '>':
            fprintf(c->output, "    incq %%r12           # >\n");
            break;
            
        case '<':
            fprintf(c->output, "    decq %%r12           # <\n");
            break;
            
        case '.':
            fprintf(c->output, "    # Output character (.)\n");
            fprintf(c->output, "    movq $1, %%rax       # sys_write\n");
            fprintf(c->output, "    movq $1, %%rdi       # stdout\n");
            fprintf(c->output, "    movq %%r12, %%rsi    # buffer\n");
            fprintf(c->output, "    movq $1, %%rdx       # length\n");
            fprintf(c->output, "    syscall\n\n");
            break;
            
        case ',':
            fprintf(c->output, "    # Input character (,)\n");
            fprintf(c->output, "    movq $0, %%rax       # sys_read\n");
            fprintf(c->output, "    movq $0, %%rdi       # stdin\n");
            fprintf(c->output, "    movq %%r12, %%rsi    # buffer\n");
            fprintf(c->output, "    movq $1, %%rdx       # length\n");
            fprintf(c->output, "    syscall\n\n");
            break;
            
        case '[': {
            int label = next_label(c);
            push_loop(c, label);
            fprintf(c->output, "loop_start_%d:           # [\n", label);
            fprintf(c->output, "    cmpb $0, (%%r12)\n");
            fprintf(c->output, "    je loop_end_%d\n\n", label);
            break;
        }
            
        case ']': {
            int label = pop_loop(c);
            fprintf(c->output, "    cmpb $0, (%%r12)\n");
            fprintf(c->output, "    jne loop_start_%d    # ]\n", label);
            fprintf(c->output, "loop_end_%d:\n\n", label);
            break;
        }
    }
}

// Optimize repeated instructions
int count_repeats(Source *src, char instruction) {
    int count = 0;
    size_t pos = src->position;
    while (pos < src->length && src->code[pos] == instruction) {
        count++;
        pos++;
    }
    return count;
}

// Compile optimized instruction
void compile_optimized(Compiler *c, Source *src, char instruction) {
    int count = count_repeats(src, instruction);
    
    if (count > 1 && (instruction == '+' || instruction == '-' || 
                      instruction == '>' || instruction == '<')) {
        switch (instruction) {
            case '+':
                fprintf(c->output, "    addb $%d, (%%r12)    # + x%d\n", count, count);
                break;
            case '-':
                fprintf(c->output, "    subb $%d, (%%r12)    # - x%d\n", count, count);
                break;
            case '>':
                fprintf(c->output, "    addq $%d, %%r12      # > x%d\n", count, count);
                break;
            case '<':
                fprintf(c->output, "    subq $%d, %%r12      # < x%d\n", count, count);
                break;
        }
        src->position += count - 1;
    } else {
        compile_instruction(c, instruction);
    }
}

// Main compilation function
void compile(Compiler *c, Source *src) {
    emit_header(c);
    
    while (src->position < src->length) {
        char ch = src->code[src->position];
        
        // Only compile valid Brainfuck instructions
        if (strchr("+-><.,[]", ch)) {
            compile_optimized(c, src, ch);
        }
        
        src->position++;
    }
    
    // Check for unmatched brackets
    if (c->loop_stack_top >= 0) {
        error("Unmatched '['", 0);
    }
    
    emit_footer(c);
}

// Read source file
Source *read_source(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        exit(1);
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *code = malloc(size + 1);
    if (!code) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    fread(code, 1, size, f);
    code[size] = '\0';
    fclose(f);
    
    Source *src = malloc(sizeof(Source));
    src->code = code;
    src->length = size;
    src->position = 0;
    
    return src;
}

// Cleanup
void free_compiler(Compiler *c) {
    if (c->output) fclose(c->output);
    if (c->loop_stack) free(c->loop_stack);
    free(c);
}

void free_source(Source *src) {
    if (src->code) free(src->code);
    free(src);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.bf> [output.s]\n", argv[0]);
        fprintf(stderr, "Compiles Brainfuck code to x86-64 assembly\n");
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *output_file = argc > 2 ? argv[2] : "output.s";
    
    printf("Brainfuck Compiler\n");
    printf("Input:  %s\n", input_file);
    printf("Output: %s\n", output_file);
    
    Source *src = read_source(input_file);
    Compiler *compiler = create_compiler(output_file);
    
    compile(compiler, src);
    
    printf("Compilation successful!\n");
    printf("\nTo assemble and run:\n");
    printf("  as %s -o output.o\n", output_file);
    printf("  ld output.o -o program\n");
    printf("  ./program\n");
    
    free_compiler(compiler);
    free_source(src);
    
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BUG_ON(cond, fmt)                                              \
    do {                                                               \
        if ((cond)) {                                                  \
            fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, \
                    __func__);                                         \
            exit(EXIT_FAILURE);                                        \
        }                                                              \
    } while (0)

#define WARN_ON(cond, fmt)                                             \
    do {                                                               \
        if ((cond)) {                                                  \
            fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, \
                    __func__);                                         \
        }                                                              \
    } while (0)

#ifdef PRINT_OUT_CMD
#define __jit_c_system(cmd)          \
    ({                               \
        printf("[JIT-C] %s\n", cmd); \
        system(cmd);                 \
    })
#else
#define __jit_c_system(cmd) system(cmd)
#endif /* PRINT_OUT_CMD */

#define jit_c_system(fmt, ...)                    \
    ({                                            \
        char __jit_c_cmd[CMD_MAX_SIZE] = { 0 };   \
        sprintf(__jit_c_cmd, fmt, ##__VA_ARGS__); \
        __jit_c_cmd[CMD_MAX_SIZE - 1] = '\0';     \
        __jit_c_system(__jit_c_cmd);              \
    })

#define BLOCK_MAX_SIZE 4096
#define LINE_MAX_SIZE 256
#define NAME_MAX_SIZE 64
#define CMD_MAX_SIZE 128

#define TERMINAL_CMD "//0\n"
#define QUIT_CMD "quit\n"

#define EXEC_FILE "generated-jit-c"

struct code_block {
    char name[NAME_MAX_SIZE];
    struct code_block *next;
};

struct jit_data {
    struct code_block *block;
    struct code_block *main;
    size_t count;
};
static struct jit_data jit_data;

static char block_buffer[BLOCK_MAX_SIZE];

static void delete_files(struct code_block *block)
{
    BUG_ON(!block, "block == NULL");
    jit_c_system("rm -f %s.c", block->name);
    jit_c_system("rm -f %s.o", block->name);
}

static void create_files(struct code_block *block)
{
    char name[NAME_MAX_SIZE + 2] = { 0 };
    struct code_block *tmp;
    FILE *file;
    int ret;

    BUG_ON(!block, "block == NULL");
    sprintf(name, "%s.c", block->name);
    jit_c_system("touch %s", name);

    file = fopen(name, "w");
    BUG_ON(!file, "open file failed");
    block_buffer[BLOCK_MAX_SIZE - 1] = '\0';
    ret = fputs(block_buffer, file);
    WARN_ON(ret < 0, "write failed");
    memset(block_buffer, '\0', BLOCK_MAX_SIZE);
    fclose(file);

    ret = jit_c_system("gcc -c %s", name);
    if (ret != 0) {
        jit_c_system("rm -f %s", name);
        if (jit_data.main == block)
            jit_data.main = NULL;
        free(block);
    } else if (block != jit_data.main) {
        tmp = jit_data.block;
        jit_data.block = block;
        block->next = tmp;
    }
}

static void free_all_files(void)
{
    struct code_block *next, *current;

    for (next = current = jit_data.block; next; current = next) {
        delete_files(current);
        next = current->next;
        free(current);
    }
    if (jit_data.main) {
        delete_files(jit_data.main);
        free(jit_data.main);
        jit_c_system("rm -f %s", EXEC_FILE);
    }
}

static struct code_block *read_block(void)
{
    char line[LINE_MAX_SIZE] = { 0 };
    struct code_block *block = NULL;
    size_t offset = 0;
    bool has_main = false;

    do {
        printf("> ");
        memset(line, '\0', LINE_MAX_SIZE);
        fgets(line, LINE_MAX_SIZE, stdin);
        line[LINE_MAX_SIZE - 1] = '\0';
        if (strncmp(QUIT_CMD, line, sizeof(QUIT_CMD)) == 0)
            return NULL;
        if (strstr(line, "main") != NULL)
            has_main = true;
        strncpy(block_buffer + offset, line, LINE_MAX_SIZE);
        offset += strlen(line);
        WARN_ON(offset >= BLOCK_MAX_SIZE, "read overflow");
    } while (!(strncmp(TERMINAL_CMD, line, sizeof(TERMINAL_CMD)) == 0));

    block = malloc(sizeof(struct code_block));
    sprintf(block->name, "jit-c-%ld", jit_data.count);
    block->next = NULL;
    jit_data.count++;

    if (has_main) {
        if (jit_data.main) {
            delete_files(jit_data.main);
            free(jit_data.main);
        }
        jit_data.main = block;
    }

    return block;
}

static void jit_exec(void)
{
    char cmd[BLOCK_MAX_SIZE] = { 0 };
    unsigned int offset = 0;

    offset = sprintf(cmd, "gcc -o %s ", EXEC_FILE);
    for (struct code_block *current = jit_data.block; current;
         current = current->next) {
        offset += sprintf(cmd + offset, "%s.o ", current->name);
    }
    offset += sprintf(cmd + offset, "%s.o ", jit_data.main->name);
    WARN_ON(offset >= BLOCK_MAX_SIZE, "overflow");
    __jit_c_system(cmd);
    jit_c_system("./%s", EXEC_FILE);
}

int main(void)
{
    for (;;) {
        struct code_block *block = read_block();
        if (!block)
            break;
        create_files(block);
        printf("----------\n");
        if (jit_data.main) {
            jit_exec();
            printf("----------\n");
        }
    }
    free_all_files();
    return 0;
}

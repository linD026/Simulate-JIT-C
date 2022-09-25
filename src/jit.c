#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

#define jit_c_system(string)            \
    do {                                \
        printf("[JIT-C] %s\n", string); \
        system(string);                 \
    } while (0)

#define BLOCK_MAX_SIZE 4096
#define LINE_MAX_SIZE 256
#define NAME_MAX_SIZE 64
#define CMD_MAX_SIZE 128

struct code_block {
    char name[NAME_MAX_SIZE];
    struct code_block *next;
};

struct jit_data {
    struct code_block *block;
    size_t count;
};
static struct jit_data jit_data;

static char block_buffer[BLOCK_MAX_SIZE];

static void create_block(struct code_block *block)
{
    char name[NAME_MAX_SIZE] = { 0 };
    char cmd[CMD_MAX_SIZE] = { 0 };
    FILE *file;
    int ret;

    BUG_ON(!block, "block == NULL");
    sprintf(name, "%s.c", block->name);
    sprintf(cmd, "touch %s", name);
    jit_c_system(cmd);

    file = fopen(name, "w");
    BUG_ON(!file, "open file failed");
    block_buffer[BLOCK_MAX_SIZE - 1] = '\0';
    ret = fputs(block_buffer, file);
    WARN_ON(ret < 0, "write failed");
    memset(block_buffer, '\0', BLOCK_MAX_SIZE);
    fclose(file);

    sprintf(cmd, "gcc -c %s", name);
    jit_c_system(cmd);
}

static void delete_files(struct code_block *block)
{
    char name[NAME_MAX_SIZE] = { 0 };
    char cmd[CMD_MAX_SIZE] = { 0 };

    BUG_ON(!block, "block == NULL");
    sprintf(name, "%s.c", block->name);
    sprintf(cmd, "rm -f %s", name);
    jit_c_system(cmd);
    sprintf(name, "%s.o", block->name);
    sprintf(cmd, "rm -f %s", name);
    jit_c_system(cmd);
}

static void free_all_files(void)
{
    struct code_block *next, *current;

    for (next = current = jit_data.block; next; current = next) {
        delete_files(current);
        next = current->next;
        free(current);
    }
}

#define TERMINAL_CMD "//0\n"
#define QUIT_CMD "quit\n"

static struct code_block *read_block(void)
{
    char line[LINE_MAX_SIZE] = { 0 };
    struct code_block *tmp, *block = NULL;
    size_t offset = 0;

    do {
        printf("> ");
        memset(line, '\0', LINE_MAX_SIZE);
        fgets(line, LINE_MAX_SIZE, stdin);
        line[LINE_MAX_SIZE - 1] = '\0';
        if (strncmp(QUIT_CMD, line, sizeof(QUIT_CMD)) == 0)
            return NULL;
        offset += strlen(line);
        strncpy(block_buffer + offset, line, LINE_MAX_SIZE);
        WARN_ON(offset >= BLOCK_MAX_SIZE, "read overflow");
    } while (!(strncmp(TERMINAL_CMD, line, sizeof(TERMINAL_CMD)) == 0));

    block = malloc(sizeof(struct code_block));
    sprintf(block->name, "jit-c-%ld", jit_data.count);
    block->next = NULL;
    jit_data.count++;

    tmp = jit_data.block;
    jit_data.block = block;
    block->next = tmp;

    return block;
}

#define EXEC_FILE "generated-jit-c"

static void jit_exec(void)
{
    char cmd[BLOCK_MAX_SIZE] = { 0 };
    unsigned int offset = 0;

    offset = sprintf(cmd, "gcc -o %s ", EXEC_FILE);
    for (struct code_block *current = jit_data.block; current;
         current = current->next) {
        offset += sprintf(cmd + offset, "%s.o ", current->name);
    }
    jit_c_system(cmd);
}

int main(void)
{
    for (;;) {
        struct code_block *block = read_block();
        if (!block)
            break;
        create_block(block);
        printf("----------\n");
        jit_exec();
        printf("----------\n");
    }
    free_all_files();
    return 0;
}

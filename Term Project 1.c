// PART 1: Includes, Constants, and Superblock Validation
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#define BLOCK_SIZE      4096
#define TOTAL_BLOCKS    64
#define DIRECT_COUNT    12
#define MAX_INODES      ((BLOCK_SIZE * 5) / sizeof(struct inode))
#define DATA_START_BLOCK 8

#pragma pack(push,1)
struct superblock {
    uint16_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap;
    uint32_t data_bitmap;
    uint32_t inode_table;
    uint32_t first_data;
    uint32_t inode_size;
    uint32_t inode_count;
    uint8_t  _pad[4058];
};

struct inode {
    uint32_t mode, uid, gid, size;
    uint32_t atime, ctime, mtime, dtime;
    uint32_t links, blocks;
    uint32_t direct[DIRECT_COUNT];
    uint32_t single, dbl, triple;
    uint8_t  _pad[156];
};
#pragma pack(pop)

static uint8_t *image;
static uint8_t inode_bitmap[BLOCK_SIZE];
static uint8_t data_bitmap[BLOCK_SIZE];
static uint32_t block_refs[TOTAL_BLOCKS];

uint32_t *get_block(uint32_t block_num) {
    return (uint32_t *)(image + BLOCK_SIZE * block_num);
}

int check_superblock(int fix) {
    struct superblock sb;
    memcpy(&sb, image, sizeof(sb));
    int errs = 0;
    if (sb.magic != 0xD34D) {
        printf("Bad magic: %04X\n", sb.magic);
        if (fix) sb.magic = 0xD34D;
        errs++;
    }
    if (sb.block_size != BLOCK_SIZE) {
        printf("Bad block size: %u\n", sb.block_size);
        if (fix) sb.block_size = BLOCK_SIZE;
        errs++;
    }
    if (sb.total_blocks != TOTAL_BLOCKS) {
        printf("Bad total blocks: %u\n", sb.total_blocks);
        if (fix) sb.total_blocks = TOTAL_BLOCKS;
        errs++;
    }
    if (sb.inode_bitmap != 1) {
        printf("Bad inode bitmap block: %u\n", sb.inode_bitmap);
        if (fix) sb.inode_bitmap = 1;
        errs++;
    }
    if (sb.data_bitmap != 2) {
        printf("Bad data bitmap block: %u\n", sb.data_bitmap);
        if (fix) sb.data_bitmap = 2;
        errs++;
    }
    if (sb.inode_table != 3) {
        printf("Bad inode table start: %u\n", sb.inode_table);
        if (fix) sb.inode_table = 3;
        errs++;
    }
    if (sb.first_data != 8) {
        printf("Bad first data block: %u\n", sb.first_data);
        if (fix) sb.first_data = 8;
        errs++;
    }
    if (sb.inode_size != sizeof(struct inode)) {
        printf("Bad inode size: %u\n", sb.inode_size);
        if (fix) sb.inode_size = sizeof(struct inode);
        errs++;
    }
    if (sb.inode_count != MAX_INODES) {
        printf("Bad inode count: %u\n", sb.inode_count);
        if (fix) sb.inode_count = MAX_INODES;
        errs++;
    }
    if (fix && errs) memcpy(image, &sb, sizeof(sb));
    return errs;
}


// PART 2: Inode Traversal and Block References
void add_block_ref(uint32_t b) {
    if (b >= DATA_START_BLOCK && b < TOTAL_BLOCKS) {
        block_refs[b]++;
    }
    // corner case not handled: invalid block references in indirect blocks beyond TOTAL_BLOCKS
}

void process_indirect_block(uint32_t block_num) {
    if (block_num < DATA_START_BLOCK || block_num >= TOTAL_BLOCKS) return;
    uint32_t *entries = get_block(block_num);
    for (int i = 0; i < BLOCK_SIZE / sizeof(uint32_t); i++) {
        if (entries[i]) add_block_ref(entries[i]);
    }
}

void traverse_inodes() {
    struct inode *inodes = (struct inode *)(image + BLOCK_SIZE * 3);
    memcpy(inode_bitmap, image + BLOCK_SIZE, BLOCK_SIZE);

    for (int i = 0; i < MAX_INODES; i++) {
        int byte = i / 8, bit = i % 8;
        int marked = inode_bitmap[byte] & (1 << bit);

        struct inode *ino = &inodes[i];
        int valid = (ino->links > 0 && ino->dtime == 0);

        if (marked && !valid) {
            printf("Inode %d marked in bitmap but invalid\n", i);
        } else if (!marked && valid) {
            printf("Inode %d valid but not marked in bitmap\n", i);
        }

        if (!valid) continue;

        // Simplified: handle each direct pointer clearly
for (int d = 0; d < DIRECT_COUNT; d++) {
    uint32_t block_num = ino->direct[d];
    if (block_num != 0) {
        add_block_ref(block_num);
    }
}

        if (ino->single) process_indirect_block(ino->single);
        // corner case not handled: nested indirects (double/triple indirect blocks)
    }
}

int check_data_bitmap(int fix) {
    memcpy(data_bitmap, image + BLOCK_SIZE * 2, BLOCK_SIZE);
    int errs = 0;

    for (int b = DATA_START_BLOCK; b < TOTAL_BLOCKS; b++) {
        int byte = b / 8, bit = b % 8;
        int marked = (data_bitmap[byte] >> bit) & 1;
        int used = (block_refs[b] > 0);

        if (marked && !used) {
            printf("Data block %d marked but not used\n", b);
            if (fix) data_bitmap[byte] &= ~(1 << bit);
            errs++;
        } else if (!marked && used) {
            printf("Data block %d used but not marked\n", b);
            if (fix) data_bitmap[byte] |= (1 << bit);
            errs++;
        }
    }

    if (fix && errs) {
        memcpy(image + BLOCK_SIZE * 2, data_bitmap, BLOCK_SIZE);
        msync(image, BLOCK_SIZE * TOTAL_BLOCKS, MS_SYNC);
    }

    return errs;
}


// PART 3: Duplicate Checking, Bad Block Checking, and Main Entry Point
void check_block_issues() {
    for (int b = DATA_START_BLOCK; b < TOTAL_BLOCKS; b++) {
        if (block_refs[b] > 1) {
            printf("Block %d duplicated %u times\n", b, block_refs[b]);
        }
        if (block_refs[b] && (b >= TOTAL_BLOCKS || b < DATA_START_BLOCK)) {
            printf("Block %d is out of valid range\n", b);
        }
    }
}

int main(int argc, char *argv[]) {
    int fix = (argc == 3 && strcmp(argv[1], "-fix") == 0);
    const char *img = fix ? argv[2] : argv[1];
    int fd = open(img, fix ? O_RDWR : O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    image = mmap(NULL, BLOCK_SIZE * TOTAL_BLOCKS,
                 PROT_READ | (fix ? PROT_WRITE : 0), MAP_SHARED, fd, 0);

    printf("=== Superblock Validation (PART 1) ===\n");
    int e1 = check_superblock(fix);
    printf(e1 ? "Superblock errors: %d\n" : "Superblock OK\n", e1);

    printf("\n=== Inode Traversal and Reference Check (PART 2) ===\n");
    traverse_inodes();

    printf("\n=== Data Bitmap Checking (PART 2 continued) ===\n");
    int e2 = check_data_bitmap(fix);
    printf(e2 ? "Data bitmap errors: %d\n" : "Data bitmap OK\n", e2);

    printf("\n=== Duplicate & Bad Block Checking (PART 3) ===\n");
    check_block_issues();

    munmap(image, BLOCK_SIZE * TOTAL_BLOCKS);
    close(fd);
    return 0;
}

struct partition_entry {
    u8 active;
    u8 chs_start[3];
    u8 type;
    u8 chs_end[3];
    u32 lba_start;
    u32 nsectors;
} __attribute__((packed));

enum partition {
    PARTITION_UEFI,
    PARTITION_BOOTFS,
    PARTITION_ROOTFS,
};

#define SECTOR_OFFSET 9ULL
#define SECTOR_SIZE (1ULL << SECTOR_OFFSET)
#define BOOTFS_SIZE (12 * MB)
#define SEC_PER_TRACK 63
#define HEADS 255
#define MAX_CYL 1023

#define VOLUME_LABEL_MAX_LEN    32  /* null-terminated string */

#define partition_at(mbr, index)                                            \
    (struct partition_entry *)(u64_from_pointer(mbr) + SECTOR_SIZE - 2 -    \
            (4 - (index)) * sizeof(struct partition_entry))

#define partition_get(mbr, part)    ({  \
    u16 *mbr_sig = (u16 *)((u64)(mbr) + SECTOR_SIZE - sizeof(*mbr_sig));  \
    struct partition_entry *e;  \
    if (*mbr_sig == 0xaa55) {   \
        struct partition_entry *first = (struct partition_entry *)(((void *)mbr_sig) -  \
                4 * sizeof(struct partition_entry));    \
        boolean uefi = (first->type == 0xEF);   \
        if (((part) == PARTITION_UEFI) && !uefi)    \
            e = 0;  \
        else    \
            e = (uefi ? (first + (part)) : (first + (part) - 1));   \
    } else  \
        e = 0;  \
    e;  \
})

static inline void mbr_chs(u8 *chs, u64 offset)
{
    u64 cyl = ((offset / SECTOR_SIZE) / SEC_PER_TRACK) / HEADS;
    u64 head = ((offset / SECTOR_SIZE) / SEC_PER_TRACK) % HEADS;
    u64 sec = ((offset / SECTOR_SIZE) % SEC_PER_TRACK) + 1;
    if (cyl > MAX_CYL) {
        cyl = MAX_CYL;
    head = 254;
    sec = 63;
    }

    chs[0] = head;
    chs[1] = (cyl >> 8) | sec;
    chs[2] = cyl & 0xff;
}

static inline void partition_write(struct partition_entry *e, boolean active,
                                   u8 type, u64 offset, u64 size)
{
    e->active = active ? 0x80 : 0x00;
    e->type = type;
    mbr_chs(e->chs_start, offset);
    mbr_chs(e->chs_end, offset + size - SECTOR_SIZE);
    e->lba_start = offset / SECTOR_SIZE;
    e->nsectors = size / SECTOR_SIZE;
}

struct filesystem;

enum storage_op {
    STORAGE_OP_READ,
    STORAGE_OP_WRITE,
    STORAGE_OP_READSG,
    STORAGE_OP_WRITESG,
    STORAGE_OP_FLUSH,
};

typedef struct storage_req {
    u8 op;
    range blocks;
    void *data;
    status_handler completion;
} *storage_req;

declare_closure_struct(2, 1, void, storage_simple_req_handler,
                       block_io, read, block_io, write,
                       storage_req, req);
storage_req_handler storage_init_req_handler(closure_ref(storage_simple_req_handler, handler),
                                             block_io read, block_io write);

typedef closure_type(fs_init_complete, void, struct filesystem *, status);
typedef closure_type(fs_init_handler, void, boolean, fs_init_complete);

void init_volumes(heap h);
void storage_set_root_fs(struct filesystem *root_fs);
void storage_set_mountpoints(tuple mounts);
boolean volume_add(u8 *uuid, char *label, void *priv, fs_init_handler init_handler, int attach_id);
void storage_when_ready(status_handler complete);
void storage_sync(status_handler sh);

struct filesystem *storage_get_fs(tuple root);
u64 storage_get_mountpoint(tuple root, struct filesystem **fs);

typedef closure_type(volume_handler, void, u8 *, const char *, struct filesystem *, u64);
void storage_iterate(volume_handler vh);

void storage_detach(void *priv, thunk complete);
typedef closure_type(mount_notification_handler, void, u64);
void storage_register_mount_notify(mount_notification_handler nh);
void storage_unregister_mount_notify(mount_notification_handler nh);

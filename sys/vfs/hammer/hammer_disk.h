/*
 * Copyright (c) 2007 The DragonFly Project.  All rights reserved.
 * 
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * $DragonFly: src/sys/vfs/hammer/hammer_disk.h,v 1.34 2008/06/01 21:05:39 dillon Exp $
 */

#ifndef VFS_HAMMER_DISK_H_
#define VFS_HAMMER_DISK_H_

#ifndef _SYS_UUID_H_
#include <sys/uuid.h>
#endif

/*
 * The structures below represent the on-disk format for a HAMMER
 * filesystem.  Note that all fields for on-disk structures are naturally
 * aligned.  The host endian format is used - compatibility is possible
 * if the implementation detects reversed endian and adjusts data accordingly.
 *
 * Most of HAMMER revolves around the concept of an object identifier.  An
 * obj_id is a 64 bit quantity which uniquely identifies a filesystem object
 * FOR THE ENTIRE LIFE OF THE FILESYSTEM.  This uniqueness allows backups
 * and mirrors to retain varying amounts of filesystem history by removing
 * any possibility of conflict through identifier reuse.
 *
 * A HAMMER filesystem may span multiple volumes.
 *
 * A HAMMER filesystem uses a 16K filesystem buffer size.  All filesystem
 * I/O is done in multiples of 16K.  Most buffer-sized headers such as those
 * used by volumes, super-clusters, clusters, and basic filesystem buffers
 * use fixed-sized A-lists which are heavily dependant on HAMMER_BUFSIZE.
 *
 * Per-volume storage limit: 52 bits		4096 TB
 * Per-Zone storage limit: 59 bits		512 KTB (due to blockmap)
 * Per-filesystem storage limit: 60 bits	1 MTB
 */
#define HAMMER_BUFSIZE		16384
#define HAMMER_BUFMASK		(HAMMER_BUFSIZE - 1)
#define HAMMER_MAXDATA		(256*1024)
#define HAMMER_BUFFER_BITS	14

#if (1 << HAMMER_BUFFER_BITS) != HAMMER_BUFSIZE
#error "HAMMER_BUFFER_BITS BROKEN"
#endif

#define HAMMER_BUFSIZE64	((u_int64_t)HAMMER_BUFSIZE)
#define HAMMER_BUFMASK64	((u_int64_t)HAMMER_BUFMASK)

#define HAMMER_OFF_ZONE_MASK	0xF000000000000000ULL /* zone portion */
#define HAMMER_OFF_VOL_MASK	0x0FF0000000000000ULL /* volume portion */
#define HAMMER_OFF_SHORT_MASK	0x000FFFFFFFFFFFFFULL /* offset portion */
#define HAMMER_OFF_LONG_MASK	0x0FFFFFFFFFFFFFFFULL /* offset portion */
#define HAMMER_OFF_SHORT_REC_MASK 0x000FFFFFFF000000ULL /* recovery boundary */
#define HAMMER_OFF_LONG_REC_MASK 0x0FFFFFFFFF000000ULL /* recovery boundary */
#define HAMMER_RECOVERY_BND	0x0000000001000000ULL

/*
 * Hammer transction ids are 64 bit unsigned integers and are usually
 * synchronized with the time of day in nanoseconds.
 *
 * Hammer offsets are used for FIFO indexing and embed a cycle counter
 * and volume number in addition to the offset.  Most offsets are required
 * to be 64-byte aligned.
 */
typedef u_int64_t hammer_tid_t;
typedef u_int64_t hammer_off_t;
typedef u_int32_t hammer_seq_t;
typedef u_int32_t hammer_crc_t;

#define HAMMER_MIN_TID		0ULL			/* unsigned */
#define HAMMER_MAX_TID		0xFFFFFFFFFFFFFFFFULL	/* unsigned */
#define HAMMER_MIN_KEY		-0x8000000000000000LL	/* signed */
#define HAMMER_MAX_KEY		0x7FFFFFFFFFFFFFFFLL	/* signed */
#define HAMMER_MIN_OBJID	HAMMER_MIN_KEY		/* signed */
#define HAMMER_MAX_OBJID	HAMMER_MAX_KEY		/* signed */
#define HAMMER_MIN_RECTYPE	0x0U			/* unsigned */
#define HAMMER_MAX_RECTYPE	0xFFFFU			/* unsigned */
#define HAMMER_MIN_OFFSET	0ULL			/* unsigned */
#define HAMMER_MAX_OFFSET	0xFFFFFFFFFFFFFFFFULL	/* unsigned */

/*
 * hammer_off_t has several different encodings.  Note that not all zones
 * encode a vol_no.
 *
 * zone 0 (z,v,o):	reserved (for sanity)
 * zone 1 (z,v,o):	raw volume relative (offset 0 is the volume header)
 * zone 2 (z,v,o):	raw buffer relative (offset 0 is the first buffer)
 * zone 3 (z,o):	undo fifo	- fixed layer2 array in root vol hdr
 * zone 4 (z,v,o):	freemap		- freemap-backed self-mapping special
 *					  cased layering.
 *
 * zone 8 (z,o):	B-Tree		- blkmap-backed
 * zone 9 (z,o):	Record		- blkmap-backed
 * zone 10 (z,o):	Large-data	- blkmap-backed
 */

#define HAMMER_ZONE_RAW_VOLUME		0x1000000000000000ULL
#define HAMMER_ZONE_RAW_BUFFER		0x2000000000000000ULL
#define HAMMER_ZONE_UNDO		0x3000000000000000ULL
#define HAMMER_ZONE_FREEMAP		0x4000000000000000ULL
#define HAMMER_ZONE_RESERVED05		0x5000000000000000ULL
#define HAMMER_ZONE_RESERVED06		0x6000000000000000ULL
#define HAMMER_ZONE_RESERVED07		0x7000000000000000ULL
#define HAMMER_ZONE_BTREE		0x8000000000000000ULL
#define HAMMER_ZONE_RECORD		0x9000000000000000ULL
#define HAMMER_ZONE_LARGE_DATA		0xA000000000000000ULL
#define HAMMER_ZONE_SMALL_DATA		0xB000000000000000ULL
#define HAMMER_ZONE_RESERVED0C		0xC000000000000000ULL
#define HAMMER_ZONE_RESERVED0D		0xD000000000000000ULL
#define HAMMER_ZONE_RESERVED0E		0xE000000000000000ULL
#define HAMMER_ZONE_RESERVED0F		0xF000000000000000ULL

#define HAMMER_ZONE_RAW_VOLUME_INDEX	1
#define HAMMER_ZONE_RAW_BUFFER_INDEX	2
#define HAMMER_ZONE_UNDO_INDEX		3
#define HAMMER_ZONE_FREEMAP_INDEX	4
#define HAMMER_ZONE_BTREE_INDEX		8
#define HAMMER_ZONE_RECORD_INDEX	9
#define HAMMER_ZONE_LARGE_DATA_INDEX	10
#define HAMMER_ZONE_SMALL_DATA_INDEX	11

/*
 * Per-zone size limitation.  This just makes the iterator easier
 * to deal with by preventing an iterator overflow.
 */
#define HAMMER_ZONE_LIMIT		\
	(0x1000000000000000ULL - HAMMER_BLOCKMAP_LAYER2 * 2)

#define HAMMER_MAX_ZONES		16

#define HAMMER_VOL_ENCODE(vol_no)			\
	((hammer_off_t)((vol_no) & 255) << 52)
#define HAMMER_VOL_DECODE(ham_off)			\
	(int32_t)(((hammer_off_t)(ham_off) >> 52) & 255)
#define HAMMER_ZONE_DECODE(ham_off)			\
	(int32_t)(((hammer_off_t)(ham_off) >> 60))
#define HAMMER_ZONE_ENCODE(zone, ham_off)		\
	(((hammer_off_t)(zone) << 60) | (ham_off))
#define HAMMER_SHORT_OFF_ENCODE(offset)			\
	((hammer_off_t)(offset) & HAMMER_OFF_SHORT_MASK)
#define HAMMER_LONG_OFF_ENCODE(offset)			\
	((hammer_off_t)(offset) & HAMMER_OFF_LONG_MASK)

#define HAMMER_ENCODE_RAW_VOLUME(vol_no, offset)	\
	(HAMMER_ZONE_RAW_VOLUME |			\
	HAMMER_VOL_ENCODE(vol_no) |			\
	HAMMER_SHORT_OFF_ENCODE(offset))

#define HAMMER_ENCODE_RAW_BUFFER(vol_no, offset)	\
	(HAMMER_ZONE_RAW_BUFFER |			\
	HAMMER_VOL_ENCODE(vol_no) |			\
	HAMMER_SHORT_OFF_ENCODE(offset))

#define HAMMER_ENCODE_FREEMAP(vol_no, offset)		\
	(HAMMER_ZONE_FREEMAP |				\
	HAMMER_VOL_ENCODE(vol_no) |			\
	HAMMER_SHORT_OFF_ENCODE(offset))

/*
 * Large-Block backing store
 *
 * A blockmap is a two-level map which translates a blockmap-backed zone
 * offset into a raw zone 2 offset.  Each layer handles 18 bits.  The 8M
 * large-block size is 23 bits so two layers gives us 23+18+18 = 59 bits
 * of address space.
 */
#define HAMMER_LARGEBLOCK_SIZE		(8192 * 1024)
#define HAMMER_LARGEBLOCK_SIZE64	((u_int64_t)HAMMER_LARGEBLOCK_SIZE)
#define HAMMER_LARGEBLOCK_MASK		(HAMMER_LARGEBLOCK_SIZE - 1)
#define HAMMER_LARGEBLOCK_MASK64	((u_int64_t)HAMMER_LARGEBLOCK_SIZE - 1)
#define HAMMER_LARGEBLOCK_BITS		23
#if (1 << HAMMER_LARGEBLOCK_BITS) != HAMMER_LARGEBLOCK_SIZE
#error "HAMMER_LARGEBLOCK_BITS BROKEN"
#endif

#define HAMMER_BUFFERS_PER_LARGEBLOCK			\
	(HAMMER_LARGEBLOCK_SIZE / HAMMER_BUFSIZE)
#define HAMMER_BUFFERS_PER_LARGEBLOCK_MASK		\
	(HAMMER_BUFFERS_PER_LARGEBLOCK - 1)
#define HAMMER_BUFFERS_PER_LARGEBLOCK_MASK64		\
	((hammer_off_t)HAMMER_BUFFERS_PER_LARGEBLOCK_MASK)

/*
 * Every blockmap has this root structure in the root volume header.
 *
 * NOTE: zone 3 (the undo FIFO) does not use phys_offset.  first and next
 * offsets represent the FIFO.
 */
struct hammer_blockmap {
	hammer_off_t	phys_offset;    /* zone-2 physical offset */
	hammer_off_t	first_offset;	/* zone-X logical offset (zone 3) */
	hammer_off_t	next_offset;	/* zone-X logical offset */
	hammer_off_t	alloc_offset;	/* zone-X logical offset */
	u_int32_t	reserved01;
	hammer_crc_t	entry_crc;
};

typedef struct hammer_blockmap *hammer_blockmap_t;

#define HAMMER_BLOCKMAP_CRCSIZE	\
	offsetof(struct hammer_blockmap, entry_crc)

/*
 * The blockmap is a 2-layer entity made up of big-blocks.  The first layer
 * contains 262144 32-byte entries (18 bits), the second layer contains
 * 524288 16-byte entries (19 bits), representing 8MB (23 bit) blockmaps.
 * 18+19+23 = 60 bits.  The top four bits are the zone id.
 *
 * Layer 2 encodes the physical bigblock mapping for a blockmap.  The freemap
 * uses this field to encode the virtual blockmap offset that allocated the
 * physical block.
 *
 * NOTE:  The freemap maps the vol_no in the upper 8 bits of layer1.
 *
 * zone-4 blockmap offset: [z:4][layer1:18][layer2:19][bigblock:23]
 */
struct hammer_blockmap_layer1 {
	hammer_off_t	blocks_free;	/* big-blocks free */
	hammer_off_t	phys_offset;	/* UNAVAIL or zone-2 */
	hammer_off_t	reserved01;
	hammer_crc_t	layer2_crc;	/* xor'd crc's of HAMMER_BLOCKSIZE */
					/* (not yet used) */
	hammer_crc_t	layer1_crc;	/* MUST BE LAST FIELD OF STRUCTURE*/
};

#define HAMMER_LAYER1_CRCSIZE	\
	offsetof(struct hammer_blockmap_layer1, layer1_crc)

struct hammer_blockmap_layer2 {
	union {
		hammer_off_t	owner;		/* used by freemap */
		hammer_off_t	phys_offset;	/* used by blockmap */
	} u;
	u_int32_t	bytes_free;	/* bytes free within this bigblock */
	hammer_crc_t	entry_crc;
};

#define HAMMER_LAYER2_CRCSIZE	\
	offsetof(struct hammer_blockmap_layer2, entry_crc)

#define HAMMER_BLOCKMAP_FREE	0ULL
#define HAMMER_BLOCKMAP_UNAVAIL	((hammer_off_t)-1LL)

#define HAMMER_BLOCKMAP_RADIX1	/* 262144 (18) */	\
	(HAMMER_LARGEBLOCK_SIZE / sizeof(struct hammer_blockmap_layer1))
#define HAMMER_BLOCKMAP_RADIX2	/* 524288 (19) */	\
	(HAMMER_LARGEBLOCK_SIZE / sizeof(struct hammer_blockmap_layer2))

#define HAMMER_BLOCKMAP_RADIX1_PERBUFFER	\
	(HAMMER_BLOCKMAP_RADIX1 / (HAMMER_LARGEBLOCK_SIZE / HAMMER_BUFSIZE))
#define HAMMER_BLOCKMAP_RADIX2_PERBUFFER	\
	(HAMMER_BLOCKMAP_RADIX2 / (HAMMER_LARGEBLOCK_SIZE / HAMMER_BUFSIZE))

#define HAMMER_BLOCKMAP_LAYER1	/* 18+19+23 */		\
	(HAMMER_BLOCKMAP_RADIX1 * HAMMER_BLOCKMAP_LAYER2)
#define HAMMER_BLOCKMAP_LAYER2	/* 19+23 - 4TB */		\
	(HAMMER_BLOCKMAP_RADIX2 * HAMMER_LARGEBLOCK_SIZE64)

#define HAMMER_BLOCKMAP_LAYER1_MASK	(HAMMER_BLOCKMAP_LAYER1 - 1)
#define HAMMER_BLOCKMAP_LAYER2_MASK	(HAMMER_BLOCKMAP_LAYER2 - 1)

/*
 * byte offset within layer1 or layer2 big-block for the entry representing
 * a zone-2 physical offset. 
 */
#define HAMMER_BLOCKMAP_LAYER1_OFFSET(zone2_offset)	\
	(((zone2_offset) & HAMMER_BLOCKMAP_LAYER1_MASK) / 	\
	 HAMMER_BLOCKMAP_LAYER2 * sizeof(struct hammer_blockmap_layer1))

#define HAMMER_BLOCKMAP_LAYER2_OFFSET(zone2_offset)	\
	(((zone2_offset) & HAMMER_BLOCKMAP_LAYER2_MASK) /	\
	HAMMER_LARGEBLOCK_SIZE64 * sizeof(struct hammer_blockmap_layer2))

/*
 * HAMMER UNDO parameters.  The UNDO fifo is mapped directly in the volume
 * header with an array of layer2 structures.  A maximum of (64x8MB) = 512MB
 * may be reserved.  The size of the undo fifo is usually set a newfs time
 * but can be adjusted if the filesystem is taken offline.
 */

#define HAMMER_UNDO_LAYER2	64	/* max layer2 undo mapping entries */

/*
 * All on-disk HAMMER structures which make up elements of the UNDO FIFO
 * contain a hammer_fifo_head and hammer_fifo_tail structure.  This structure
 * contains all the information required to validate the fifo element
 * and to scan the fifo in either direction.  The head is typically embedded
 * in higher level hammer on-disk structures while the tail is typically
 * out-of-band.  hdr_size is the size of the whole mess, including the tail.
 *
 * All undo structures are guaranteed to not cross a 16K filesystem
 * buffer boundary.  Most undo structures are fairly small.  Data spaces
 * are not immediately reused by HAMMER so file data is not usually recorded
 * as part of an UNDO.
 *
 * PAD elements are allowed to take up only 8 bytes of space as a special
 * case, containing only hdr_signature, hdr_type, and hdr_size fields,
 * and with the tail overloaded onto the head structure for 8 bytes total.
 *
 * Every undo record has a sequence number.  This number is unrelated to
 * transaction ids and instead collects the undo transactions associated
 * with a single atomic operation.  A larger transactional operation, such
 * as a remove(), may consist of several smaller atomic operations
 * representing raw meta-data operations.
 */
#define HAMMER_HEAD_ONDISK_SIZE		32
#define HAMMER_HEAD_ALIGN		8
#define HAMMER_HEAD_ALIGN_MASK		(HAMMER_HEAD_ALIGN - 1)
#define HAMMER_TAIL_ONDISK_SIZE		8

struct hammer_fifo_head {
	u_int16_t hdr_signature;
	u_int16_t hdr_type;
	u_int32_t hdr_size;	/* aligned size of the whole mess */
	u_int32_t reserved01;	/* (0) reserved for future use */
	hammer_crc_t hdr_crc;	/* XOR crc up to field w/ crc after field */
};

#define HAMMER_FIFO_HEAD_CRCOFF	offsetof(struct hammer_fifo_head, hdr_crc)

struct hammer_fifo_tail {
	u_int16_t tail_signature;
	u_int16_t tail_type;
	u_int32_t tail_size;	/* aligned size of the whole mess */
};

typedef struct hammer_fifo_head *hammer_fifo_head_t;
typedef struct hammer_fifo_tail *hammer_fifo_tail_t;

/*
 * Fifo header types.
 */
#define HAMMER_HEAD_TYPE_PAD	(0x0040U|HAMMER_HEAD_FLAG_FREE)
#define HAMMER_HEAD_TYPE_VOL	0x0041U		/* Volume (dummy header) */
#define HAMMER_HEAD_TYPE_BTREE	0x0042U		/* B-Tree node */
#define HAMMER_HEAD_TYPE_UNDO	0x0043U		/* random UNDO information */
#define HAMMER_HEAD_TYPE_DELETE	0x0044U		/* record deletion */
#define HAMMER_HEAD_TYPE_RECORD	0x0045U		/* Filesystem record */

#define HAMMER_HEAD_FLAG_FREE	0x8000U		/* Indicates object freed */

#define HAMMER_HEAD_SIGNATURE	0xC84EU
#define HAMMER_TAIL_SIGNATURE	0xC74FU

#define HAMMER_HEAD_SEQ_BEG	0x80000000U
#define HAMMER_HEAD_SEQ_END	0x40000000U
#define HAMMER_HEAD_SEQ_MASK	0x3FFFFFFFU

/*
 * Misc FIFO structures.
 */
struct hammer_fifo_undo {
	struct hammer_fifo_head	head;
	hammer_off_t		undo_offset;	/* zone-1 offset */
	int32_t			undo_data_bytes;
	int32_t			undo_reserved01;
	/* followed by data */
};

typedef struct hammer_fifo_undo *hammer_fifo_undo_t;

struct hammer_fifo_buf_commit {
	hammer_off_t		undo_offset;
};

/*
 * Volume header types
 */
#define HAMMER_FSBUF_VOLUME	0xC8414D4DC5523031ULL	/* HAMMER01 */
#define HAMMER_FSBUF_VOLUME_REV	0x313052C54D4D41C8ULL	/* (reverse endian) */

/*
 * The B-Tree structures need hammer_fsbuf_head.
 */
#include "hammer_btree.h"

/*
 * HAMMER Volume header
 *
 * A HAMMER filesystem is built from any number of block devices,  Each block
 * device contains a volume header followed by however many buffers fit
 * into the volume.
 *
 * One of the volumes making up a HAMMER filesystem is the master, the
 * rest are slaves.  It does not have to be volume #0.
 *
 * The volume header takes up an entire 16K filesystem buffer and may
 * represent up to 64KTB (65536 TB) of space.
 *
 * Special field notes:
 *
 *	vol_bot_beg - offset of boot area (mem_beg - bot_beg bytes)
 *	vol_mem_beg - offset of memory log (clu_beg - mem_beg bytes)
 *	vol_buf_beg - offset of the first buffer.
 *
 *	The memory log area allows a kernel to cache new records and data
 *	in memory without allocating space in the actual filesystem to hold
 *	the records and data.  In the event that a filesystem becomes full,
 *	any records remaining in memory can be flushed to the memory log
 *	area.  This allows the kernel to immediately return success.
 */

#define HAMMER_BOOT_MINBYTES		(32*1024)
#define HAMMER_BOOT_NOMBYTES		(64LL*1024*1024)
#define HAMMER_BOOT_MAXBYTES		(256LL*1024*1024)

#define HAMMER_MEM_MINBYTES		(256*1024)
#define HAMMER_MEM_NOMBYTES		(1LL*1024*1024*1024)
#define HAMMER_MEM_MAXBYTES		(64LL*1024*1024*1024)

struct hammer_volume_ondisk {
	u_int64_t vol_signature;/* Signature */

	int64_t vol_bot_beg;	/* byte offset of boot area or 0 */
	int64_t vol_mem_beg;	/* byte offset of memory log or 0 */
	int64_t vol_buf_beg;	/* byte offset of first buffer in volume */
	int64_t vol_buf_end;	/* byte offset of volume EOF (on buf bndry) */
	int64_t vol_locked;	/* reserved clusters are >= this offset */

	uuid_t    vol_fsid;	/* identify filesystem */
	uuid_t    vol_fstype;	/* identify filesystem type */
	char	  vol_name[64];	/* Name of volume */

	int32_t vol_no;		/* volume number within filesystem */
	int32_t vol_count;	/* number of volumes making up FS */

	u_int32_t vol_version;	/* version control information */
	hammer_crc_t vol_crc;	/* header crc */
	u_int32_t vol_flags;	/* volume flags */
	u_int32_t vol_rootvol;	/* which volume is the root volume? */

	int32_t vol_reserved04;
	int32_t vol_reserved05;
	u_int32_t vol_reserved06;
	u_int32_t vol_reserved07;

	int32_t vol_blocksize;		/* for statfs only */
	int32_t vol_reserved08;
	int64_t vol_nblocks;		/* total allocatable hammer bufs */

	/*
	 * These fields are initialized and space is reserved in every
	 * volume making up a HAMMER filesytem, but only the master volume
	 * contains valid data.
	 */
	int64_t vol0_stat_bigblocks;	/* total bigblocks when fs is empty */
	int64_t vol0_stat_freebigblocks;/* number of free bigblocks */
	int64_t	vol0_stat_bytes;	/* for statfs only */
	int64_t vol0_stat_inodes;	/* for statfs only */
	int64_t vol0_stat_records;	/* total records in filesystem */
	hammer_off_t vol0_btree_root;	/* B-Tree root */
	hammer_tid_t vol0_next_tid;	/* highest synchronized TID */
	hammer_off_t vol0_zone_limit;	/* limit the zone size */

	/*
	 * Blockmaps for zones.  Not all zones use a blockmap.  Note that
	 * the entire root blockmap is cached in the hammer_mount structure.
	 */
	struct hammer_blockmap	vol0_blockmap[HAMMER_MAX_ZONES];

	/*
	 * Layer-2 array for undo fifo
	 */
	struct hammer_blockmap_layer2 vol0_undo_array[HAMMER_UNDO_LAYER2];

};

typedef struct hammer_volume_ondisk *hammer_volume_ondisk_t;

#define HAMMER_VOLF_VALID		0x0001	/* valid entry */
#define HAMMER_VOLF_OPEN		0x0002	/* volume is open */

#define HAMMER_VOL_CRCSIZE1	\
	offsetof(struct hammer_volume_ondisk, vol_crc)
#define HAMMER_VOL_CRCSIZE2	\
	(sizeof(struct hammer_volume_ondisk) - HAMMER_VOL_CRCSIZE1 -	\
	 sizeof(hammer_crc_t))

/*
 * Record types are fairly straightforward.  The B-Tree includes the record
 * type in its index sort.
 *
 * In particular please note that it is possible to create a pseudo-
 * filesystem within a HAMMER filesystem by creating a special object
 * type within a directory.  Pseudo-filesystems are used as replication
 * targets and even though they are built within a HAMMER filesystem they
 * get their own obj_id space (and thus can serve as a replication target)
 * and look like a mount point to the system.
 *
 * NOTE: hammer_ip_delete_range_all() deletes all record types greater
 * then HAMMER_RECTYPE_INODE.
 */
#define HAMMER_RECTYPE_UNKNOWN		0
#define HAMMER_RECTYPE_LOWEST		1	/* lowest record type avail */
#define HAMMER_RECTYPE_INODE		1	/* inode in obj_id space */
#define HAMMER_RECTYPE_PSEUDO_INODE	2	/* pseudo filesysem */
#define HAMMER_RECTYPE_CLUSTER		3	/* inter-cluster reference */
#define HAMMER_RECTYPE_DATA		0x0010
#define HAMMER_RECTYPE_DIRENTRY		0x0011
#define HAMMER_RECTYPE_DB		0x0012
#define HAMMER_RECTYPE_EXT		0x0013	/* ext attributes */
#define HAMMER_RECTYPE_FIX		0x0014	/* fixed attribute */
#define HAMMER_RECTYPE_MOVED		0x8000	/* special recovery flag */

#define HAMMER_FIXKEY_SYMLINK		1

#define HAMMER_OBJTYPE_UNKNOWN		0	/* (never exists on-disk) */
#define HAMMER_OBJTYPE_DIRECTORY	1
#define HAMMER_OBJTYPE_REGFILE		2
#define HAMMER_OBJTYPE_DBFILE		3
#define HAMMER_OBJTYPE_FIFO		4
#define HAMMER_OBJTYPE_CDEV		5
#define HAMMER_OBJTYPE_BDEV		6
#define HAMMER_OBJTYPE_SOFTLINK		7
#define HAMMER_OBJTYPE_PSEUDOFS		8	/* pseudo filesystem obj */

/*
 * HAMMER inode attribute data
 *
 * The data reference for a HAMMER inode points to this structure.  Any
 * modifications to the contents of this structure will result in a
 * replacement operation.
 *
 * parent_obj_id is only valid for directories (which cannot be hard-linked),
 * and specifies the parent directory obj_id.  This field will also be set
 * for non-directory inodes as a recovery aid, but can wind up specifying
 * stale information.  However, since object id's are not reused, the worse
 * that happens is that the recovery code is unable to use it.
 *
 * NOTE: atime is stored in the inode's B-Tree element and not in the inode
 * data.  This allows the atime to be updated without having to lay down a
 * new record.
 */
struct hammer_inode_data {
	u_int16_t version;	/* inode data version */
	u_int16_t mode;		/* basic unix permissions */
	u_int32_t uflags;	/* chflags */
	u_int32_t rmajor;	/* used by device nodes */
	u_int32_t rminor;	/* used by device nodes */
	u_int64_t ctime;
	u_int64_t parent_obj_id;/* parent directory obj_id */
	uuid_t	  uid;
	uuid_t	  gid;

	u_int8_t  obj_type;
	u_int8_t  reserved01;
	u_int16_t reserved02;
	u_int32_t reserved03;
	u_int64_t mtime;
	u_int64_t size;		/* filesystem object size */
	u_int64_t nlinks;	/* hard links */
	u_int64_t reserved04;
	union {
		char	reserved06[24];
		char	symlink[24];	/* HAMMER_INODE_BASESYMLEN */
	} ext;
};

#define HAMMER_INODE_DATA_VERSION	1
#define HAMMER_OBJID_ROOT		1
#define HAMMER_INODE_BASESYMLEN		24

/*
 * A directory entry specifies the HAMMER filesystem object id, a copy of
 * the file type, and file name (either embedded or as out-of-band data).
 * If the file name is short enough to fit into den_name[] (including a
 * terminating nul) then it will be embedded in the record, otherwise it
 * is stored out-of-band.  The base record's data reference always points
 * to the nul-terminated filename regardless.
 *
 * Directory entries are indexed with a 128 bit namekey rather then an
 * offset.  A portion of the namekey is an iterator or randomizer to deal
 * with collisions.
 *
 * NOTE: base.base.obj_type holds the filesystem object type of obj_id,
 *	 e.g. a den_type equivalent.
 *
 * NOTE: den_name / the filename data reference is NOT terminated with \0.
 *
 */
struct hammer_entry_data {
	u_int64_t obj_id;		/* object being referenced */
	u_int64_t reserved01;
	char	name[16];		/* name (extended) */
};

#define HAMMER_ENTRY_NAME_OFF	offsetof(struct hammer_entry_data, name[0])
#define HAMMER_ENTRY_SIZE(nlen)	offsetof(struct hammer_entry_data, name[nlen])

struct hammer_symlink_data {
	char	name[16];
};

#define HAMMER_SYMLINK_NAME_OFF	offsetof(struct hammer_symlink_data, name[0])

/*
 * Rollup various structures embedded as record data
 */
union hammer_data_ondisk {
	struct hammer_entry_data entry;
	struct hammer_inode_data inode;
	struct hammer_symlink_data symlink;
};

typedef union hammer_data_ondisk *hammer_data_ondisk_t;

#endif

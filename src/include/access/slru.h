/*-------------------------------------------------------------------------
 *
 * slru.h
 *		Simple LRU buffering for transaction status logfiles
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/access/slru.h,v 1.24 2009/01/01 17:23:56 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef SLRU_H
#define SLRU_H

#include "access/xlogdefs.h"
#include "storage/lwlock.h"

#define CLOG_DIR				"pg_clog"
#define DISTRIBUTEDLOG_DIR		"pg_distributedlog"
#define DISTRIBUTEDXIDMAP_DIR	"pg_distributedxidmap" 
#define MULTIXACT_MEMBERS_DIR	"pg_multixact/members"
#define MULTIXACT_OFFSETS_DIR	"pg_multixact/offsets"
#define SUBTRANS_DIR			"pg_subtrans" 

#define SLRU_FILENAME_LEN		4     /* SLRU filenames are 4 characters each */
#define SLRU_CHECKSUM_FILENAME 	"slru_checksum_file"
#define SLRU_MD5_BUFLEN			33     /* MD5 is 32 bytes + 1 null-terminator */

                           /* room for filename + ":" + " " + md5 hash + "\n" */
#define SLRU_CKSUM_LINE_LEN		(SLRU_FILENAME_LEN + 3 + SLRU_MD5_BUFLEN)

#define SLRU_CKSUM_LINE_DELIM	"\n"

/*
 * Page status codes.  Note that these do not include the "dirty" bit.
 * page_dirty can be TRUE only in the VALID or WRITE_IN_PROGRESS states;
 * in the latter case it implies that the page has been re-dirtied since
 * the write started.
 */
typedef enum
{
	SLRU_PAGE_EMPTY,			/* buffer is not in use */
	SLRU_PAGE_READ_IN_PROGRESS, /* page is being read in */
	SLRU_PAGE_VALID,			/* page is valid and not being written */
	SLRU_PAGE_WRITE_IN_PROGRESS /* page is being written out */
} SlruPageStatus;

/*
 * Shared-memory state
 */
typedef struct SlruSharedData
{
	LWLockId	ControlLock;

	/* Number of buffers managed by this SLRU structure */
	int			num_slots;

	/*
	 * Arrays holding info for each buffer slot.  Page number is undefined
	 * when status is EMPTY, as is page_lru_count.
	 */
	char	  **page_buffer;
	SlruPageStatus *page_status;
	bool	   *page_dirty;
	int		   *page_number;
	int		   *page_lru_count;
	LWLockId   *buffer_locks;

	/*
	 * Optional array of WAL flush LSNs associated with entries in the SLRU
	 * pages.  If not zero/NULL, we must flush WAL before writing pages (true
	 * for pg_clog, false for multixact and pg_subtrans).  group_lsn[] has
	 * lsn_groups_per_page entries per buffer slot, each containing the
	 * highest LSN known for a contiguous group of SLRU entries on that slot's
	 * page.
	 */
	XLogRecPtr *group_lsn;
	int			lsn_groups_per_page;

	/*----------
	 * We mark a page "most recently used" by setting
	 *		page_lru_count[slotno] = ++cur_lru_count;
	 * The oldest page is therefore the one with the highest value of
	 *		cur_lru_count - page_lru_count[slotno]
	 * The counts will eventually wrap around, but this calculation still
	 * works as long as no page's age exceeds INT_MAX counts.
	 *----------
	 */
	int			cur_lru_count;

	/*
	 * latest_page_number is the page number of the current end of the log;
	 * this is not critical data, since we use it only to avoid swapping out
	 * the latest page.
	 */
	int			latest_page_number;
} SlruSharedData;

typedef SlruSharedData *SlruShared;

/*
 * SlruCtlData is an unshared structure that points to the active information
 * in shared memory.
 */
typedef struct SlruCtlData
{
	SlruShared	shared;

	/*
	 * This flag tells whether to fsync writes (true for pg_clog and multixact
	 * stuff, false for pg_subtrans).
	 */
	bool		do_fsync;

	/*
	 * Decide which of two page numbers is "older" for truncation purposes. We
	 * need to use comparison of TransactionIds here in order to do the right
	 * thing with wraparound XID arithmetic.
	 */
	bool		(*PagePrecedes) (int, int);

	/*
	 * Dir is set during SimpleLruInit and does not change thereafter. Since
	 * it's always the same, it doesn't need to be in shared memory.
	 */
	char		Dir[64];
} SlruCtlData;

typedef SlruCtlData *SlruCtl;

/* Opaque struct known only in slru.c */
typedef struct SlruFlushData *SlruFlush;


extern Size SimpleLruShmemSize(int nslots, int nlsns);
extern void SimpleLruInit(SlruCtl ctl, const char *name, int nslots, int nlsns,
			  LWLockId ctllock, const char *subdir);
extern int	SimpleLruZeroPage(SlruCtl ctl, int pageno);
extern int SimpleLruReadPage(SlruCtl ctl, int pageno, bool write_ok,
				  TransactionId xid);
extern int SimpleLruReadPage_ReadOnly(SlruCtl ctl, int pageno,
				      TransactionId xid, bool *valid);
extern void SimpleLruWritePage(SlruCtl ctl, int slotno, SlruFlush fdata);
extern void SimpleLruFlush(SlruCtl ctl, bool checkpoint);
extern void SimpleLruTruncate(SlruCtl ctl, int cutoffPage);
extern void SimpleLruTruncateWithLock(SlruCtl ctl, int cutoffPage);
extern bool SlruScanDirectory(SlruCtl ctl, int cutoffPage, bool doDeletions);
extern bool SimpleLruPageExists(SlruCtl ctl, int pageno);
extern int SlruRecoverMirror(void);
extern int SlruCreateChecksumFile(const char *fullDirName);
extern int SlruMirrorVerifyDirectoryChecksum(char *dirName, char *cksumFile,
											 char *primaryMd5);

#endif   /* SLRU_H */

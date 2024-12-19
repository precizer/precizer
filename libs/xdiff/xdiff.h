#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "rational.h"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libxdiff 0.23"
#define XDF_NEED_MINIMAL (1 << 1)
#define XDL_PATCH_NORMAL '-'
#define XDL_PATCH_REVERSE '+'
#define XDL_PATCH_MODEMASK ((1 << 8) - 1)
#define XDL_PATCH_IGNOREBSPACE (1 << 8)
#define XDL_MMB_READONLY (1 << 0)
#define XDL_MMF_ATOMIC (1 << 0)
#define XDL_BDOP_INS 1
#define XDL_BDOP_CPY 2
#define XDL_BDOP_INSB 3
#define XDL_MAX_COST_MIN 256
#define XDL_HEUR_MIN_COST 256
#define XDL_LINE_MAX (long)((1UL << (8 * sizeof(long) - 1)) - 1)
#define XDL_SNAKE_CNT 20
#define XDL_K_HEUR 4
#define XDL_KPDIS_RUN 4
#define XDL_MAX_EQLIMIT 1024
#define XDL_SIMSCAN_WINDOWN 100
#define XDLT_STD_BLKSIZE (1024 * 8)
#define XDLT_MAX_LINE_SIZE 80
#define XDL_GUESS_NLINES 256

/* largest prime smaller than 65536 */
#define BASE 65521L

/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */
#define NMAX 5552

#define DO1(buf,i) \
	{ \
		s1 += buf[i]; \
		s2 += s1; \
	}
#define DO2(buf,i) \
	DO1(buf,i); \
	DO1(buf,i + 1);
#define DO4(buf,i) \
	DO2(buf,i); \
	DO2(buf,i + 2);
#define DO8(buf,i) \
	DO4(buf,i); \
	DO4(buf,i + 4);
#define DO16(buf) \
	DO8(buf,0); \
	DO8(buf,8);

#define DBL_RAND() (((double)rand()) / (1.0 + (double)RAND_MAX))

#define XDL_MIN(a,b) ((a) < (b) ? (a) : (b))
#define XDL_MAX(a,b) ((a) > (b) ? (a) : (b))
#define XDL_ABS(v) ((v) >= 0 ? (v) : -(v))
#define XDL_ISDIGIT(c) ((c) >= '0' && (c) <= '9')
#define XDL_ADDBITS(v,b) ((v) + ((v) >> (b)))
#define XDL_MASKBITS(b) ((1UL << (b)) - 1)
#define XDL_HASHLONG(v,b) \
	(XDL_ADDBITS((unsigned long)(v),b) & XDL_MASKBITS(b))
#define XDL_PTRFREE(p) \
	do \
	{ \
		if(p) \
		{ \
			xdl_free(p); \
			(p) = NULL; \
		} \
	} while(0)
#define XDL_RECMATCH(r1,r2) \
	((r1)->size == (r2)->size && memcmp((r1)->ptr,(r2)->ptr,(r1)->size) == 0)
#define XDL_LE32_PUT(p,v) \
	do \
	{ \
		unsigned char *__p = (unsigned char *)(p); \
		*__p++ = (unsigned char)(v); \
		*__p++ = (unsigned char)((v) >> 8); \
		*__p++ = (unsigned char)((v) >> 16); \
		*__p = (unsigned char)((v) >> 24); \
	} while(0)
#define XDL_LE32_GET(p,v) \
	do \
	{ \
		unsigned char const *__p = (unsigned char const *)(p); \
		(v) = (unsigned long)__p[0] | ((unsigned long)__p[1]) << 8 | \
		        ((unsigned long)__p[2]) << 16 | ((unsigned long)__p[3]) << 24; \
	} while(0)

typedef struct s_memallocator {
	void *priv;
	void *(*malloc)(void *,unsigned int);
	void (*free)(
		void *,
		void *
	);
	void *(*realloc)(void *,void *,unsigned int);
} memallocator_t;

typedef struct s_mmblock {
	struct s_mmblock *next;
	unsigned long flags;
	long size,bsize;
	char *ptr;
} mmblock_t;

typedef struct s_mmfile {
	unsigned long flags;
	mmblock_t *head,*tail;
	long bsize,fsize,rpos;
	mmblock_t *rcur,*wcur;
} mmfile_t;

typedef struct s_mmbuffer {
	char *ptr;
	long size;
} mmbuffer_t;

typedef struct s_xpparam {
	unsigned long flags;
} xpparam_t;

typedef struct s_xdemitcb {
	void *priv;
	int (*outf)(
		void *,
		mmbuffer_t *,
		int
	);
} xdemitcb_t;

typedef struct s_xdemitconf {
	long ctxlen;
} xdemitconf_t;

typedef struct s_bdiffparam {
	long bsize;
} bdiffparam_t;

int xdl_set_allocator(memallocator_t const *malt);
void *xdl_malloc(unsigned int size);
void xdl_free(void *ptr);
void *xdl_realloc(
	void         *ptr,
	unsigned int size
);

int xdl_init_mmfile(
	mmfile_t      *mmf,
	long          bsize,
	unsigned long flags
);
void xdl_free_mmfile(mmfile_t *mmf);
void *xdl_mmfile_writeallocate(
	mmfile_t *mmf,
	long     size
);
void *xdl_mmfile_first(
	mmfile_t *mmf,
	long     *size
);
void *xdl_mmfile_next(
	mmfile_t *mmf,
	long     *size
);
long xdl_mmfile_size(mmfile_t *mmf);

int xdl_diff(
	mmfile_t           *mf1,
	mmfile_t           *mf2,
	xpparam_t const    *xpp,
	xdemitconf_t const *xecfg,
	xdemitcb_t         *ecb
);

int xdl_merge3(
	mmfile_t   *mmfo,
	mmfile_t   *mmf1,
	mmfile_t   *mmf2,
	xdemitcb_t *ecb,
	xdemitcb_t *rjecb
);


typedef struct s_chanode {
	struct s_chanode *next;
	long icurr;
} chanode_t;

typedef struct s_chastore {
	chanode_t *head,*tail;
	long isize,nsize;
	chanode_t *ancur;
	chanode_t *sncur;
	long scurr;
} chastore_t;

typedef struct s_xrecord {
	struct s_xrecord *next;
	char const *ptr;
	long size;
	unsigned long ha;
} xrecord_t;

typedef struct s_xdfile {
	chastore_t rcha;
	long nrec;
	unsigned int hbits;
	xrecord_t **rhash;
	long dstart,dend;
	xrecord_t **recs;
	char *rchg;
	long *rindex;
	long nreff;
	unsigned long *ha;
} xdfile_t;

typedef struct s_diffdata {
	long nrec;
	unsigned long const *ha;
	long *rindex;
	char *rchg;
} diffdata_t;

typedef struct s_xdalgoenv {
	long mxcost;
	long snake_cnt;
	long heur_min;
} xdalgoenv_t;

typedef struct s_xdchange {
	struct s_xdchange *next;
	long i1,i2;
	long chg1,chg2;
} xdchange_t;

typedef struct s_xdfenv {
	xdfile_t xdf1,xdf2;
} xdfenv_t;

int xdl_recs_cmp(
	diffdata_t  *dd1,
	long        off1,
	long        lim1,
	diffdata_t  *dd2,
	long        off2,
	long        lim2,
	long        *kvdf,
	long        *kvdb,
	int         need_min,
	xdalgoenv_t *xenv
);
int xdl_do_diff(
	mmfile_t        *mf1,
	mmfile_t        *mf2,
	xpparam_t const *xpp,
	xdfenv_t        *xe
);
int xdl_build_script(
	xdfenv_t   *xe,
	xdchange_t **xscr
);
void xdl_free_script(xdchange_t *xscr);
int xdl_emit_diff(
	xdfenv_t           *xe,
	xdchange_t         *xscr,
	xdemitcb_t         *ecb,
	xdemitconf_t const *xecfg
);

int xdl_emit_diff(
	xdfenv_t           *xe,
	xdchange_t         *xscr,
	xdemitcb_t         *ecb,
	xdemitconf_t const *xecfg
);


int xdl_prepare_env(
	mmfile_t        *mf1,
	mmfile_t        *mf2,
	xpparam_t const *xpp,
	xdfenv_t        *xe
);
void xdl_free_env(xdfenv_t *xe);

int xdlt_load_mmfile(
	char const *fname,
	mmfile_t   *mf,
	int        binmode
);

long xdl_bogosqrt(long n);
int xdl_emit_diffrec(
	char const *rec,
	long       size,
	char const *pre,
	long       psize,
	xdemitcb_t *ecb
);
int xdl_cha_init(
	chastore_t *cha,
	long       isize,
	long       icount
);
void xdl_cha_free(chastore_t *cha);
void *xdl_cha_alloc(chastore_t *cha);
long xdl_guess_lines(mmfile_t *mf);
unsigned long xdl_hash_record(
	char const **data,
	char const *top
);
unsigned int xdl_hashbits(unsigned int size);
int xdl_num_out(
	char *out,
	long val
);
int xdl_emit_hunk_hdr(
	long       s1,
	long       c1,
	long       s2,
	long       c2,
	xdemitcb_t *ecb
);

Return compare_texts(
	char const *,
	char const *
);
#pragma once
#define _GNU_SOURCE
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
#define XDL_SIMSCAN_WINDOW 100
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
	(XDL_ADDBITS((unsigned long)(v),b) &XDL_MASKBITS(b))
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
	void *(*malloc)(
		void *,
		unsigned int);
	void (*free)(
		void *,
		void *);
	void *(*realloc)(
		void *,
		void *,
		unsigned int);
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
		int);
} xdemitcb_t;

typedef struct s_xdemitconf {
	long ctxlen;
	char str_meta;
} xdemitconf_t;

typedef struct s_bdiffparam {
	long bsize;
} bdiffparam_t;

int xdl_set_allocator(memallocator_t const *);
void *xdl_malloc(unsigned int);
void xdl_free(void *);
void *xdl_realloc(
	void *,
	unsigned int);

int xdl_init_mmfile(
	mmfile_t *,
	long,
	unsigned long);
void xdl_free_mmfile(mmfile_t *);
void *xdl_mmfile_writeallocate(
	mmfile_t *,
	long);
void *xdl_mmfile_first(
	mmfile_t *,
	long *);
void *xdl_mmfile_next(
	mmfile_t *,
	long *);
long xdl_mmfile_size(mmfile_t *) __attribute__((pure));

int xdl_diff(
	mmfile_t *,
	mmfile_t *,
	xpparam_t const *,
	xdemitconf_t const *,
	xdemitcb_t *);

int xdl_merge3(
	mmfile_t *,
	mmfile_t *,
	mmfile_t *,
	xdemitcb_t *,
	xdemitcb_t *);

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
	diffdata_t *,
	long,
	long,
	diffdata_t *,
	long,
	long,
	long *,
	long *,
	int,
	xdalgoenv_t *);
int xdl_do_diff(
	mmfile_t *,
	mmfile_t *,
	xpparam_t const *,
	xdfenv_t *);
int xdl_build_script(
	xdfenv_t *,
	xdchange_t **);
void xdl_free_script(xdchange_t *);
int xdl_emit_diff(
	xdfenv_t *,
	xdchange_t *,
	xdemitcb_t *,
	xdemitconf_t const *);

int xdl_emit_diff(
	xdfenv_t *,
	xdchange_t *,
	xdemitcb_t *,
	xdemitconf_t const *);

int xdl_prepare_env(
	mmfile_t *,
	mmfile_t *,
	xdfenv_t *);
void xdl_free_env(xdfenv_t *);

long xdl_bogosqrt(long) __attribute__((const));
int xdl_emit_diffrec(
	char const *,
	long,
	char const *,
	long,
	xdemitcb_t *);
int xdl_cha_init(
	chastore_t *,
	long,
	long);
void xdl_cha_free(chastore_t *);
void *xdl_cha_alloc(chastore_t *);
long xdl_guess_lines(mmfile_t *);
unsigned long xdl_hash_record(
	char const **,
	char const *);
unsigned int xdl_hashbits(unsigned int) __attribute__((const));
int xdl_num_out(
	char *,
	long);
int xdl_emit_hunk_hdr(
	long,
	long,
	long,
	long,
	xdemitcb_t *);

Return compare_texts(
	char const *,
	char const *);

Return compare_strings(
	char **,
	const char *,
	const char *);

void *wrap_malloc(
	void *,
	unsigned int);

void wrap_free(
	void *,
	void *);

void *wrap_realloc(
	void *,
	void *,
	unsigned int);

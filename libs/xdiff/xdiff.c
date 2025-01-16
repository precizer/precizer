#include "xdiff.h"

static memallocator_t xmalt = {
	NULL,NULL,NULL
};

/**
 * @brief Sets the memory allocator functions for the library
 *
 * @param malt Pointer to memory allocator structure containing custom allocation functions
 * @return int Returns 0 on success
 */
int xdl_set_allocator(memallocator_t const *malt){
	xmalt = *malt;
	return 0;
}

/**
 * @brief Allocates memory using the configured memory allocator
 *
 * @param size Number of bytes to allocate
 * @return void* Pointer to allocated memory or NULL if allocation fails
 */
void *xdl_malloc(unsigned int size){
	return xmalt.malloc ? xmalt.malloc(xmalt.priv,size) : NULL;
}

/**
 * @brief Frees memory using the configured memory allocator
 *
 * @param ptr Pointer to memory to free
 */
void xdl_free(void *ptr){
	if(xmalt.free)
	{
		xmalt.free(xmalt.priv,ptr);
	}
}

/**
 * @brief Reallocates memory block using the configured memory allocator
 *
 * @param ptr Pointer to existing memory block or NULL
 * @param size New size in bytes
 * @return void* Pointer to reallocated memory or NULL if reallocation fails
 */
void *xdl_realloc(
	void         *ptr,
	unsigned int size
){
	return xmalt.realloc ? xmalt.realloc(xmalt.priv,ptr,size) : NULL;
}

typedef struct s_xdpsplit {
	long i1,i2;
	int min_lo,min_hi;
} xdpsplit_t;

/**
 * @brief Splits difference algorithm search space recursively
 *
 * See "An O(ND) Difference Algorithm and its Variations", by Eugene Myers.
 * Basically considers a "box" (off1, off2, lim1, lim2) and scan from both
 * the forward diagonal starting from (off1, off2) and the backward diagonal
 * starting from (lim1, lim2). If the K values on the same diagonal crosses
 * returns the furthest point of reach. We might end up having to expensive
 * cases using this algorithm is full, so a little bit of heuristic is needed
 * to cut the search and to return a suboptimal point.
 */
static long xdl_split(
	unsigned long const *ha1,
	long                off1,
	long                lim1,
	unsigned long const *ha2,
	long                off2,
	long                lim2,
	long                *kvdf,
	long                *kvdb,
	int                 need_min,
	xdpsplit_t          *spl,
	xdalgoenv_t         *xenv
){
	long dmin = off1 - lim2,dmax = lim1 - off2;
	long fmid = off1 - off2,bmid = lim1 - lim2;
	long odd = (fmid - bmid) & 1;
	long fmin = fmid,fmax = fmid;
	long bmin = bmid,bmax = bmid;
	long ec,d,i1,i2,prev1,best,dd,v,k;

	/*
	 * Set initial diagonal values for both forward and backward path.
	 */
	kvdf[fmid] = off1;
	kvdb[bmid] = lim1;

	for(ec = 1;; ec++)
	{
		int got_snake = 0;

		/*
		 * We need to extent the diagonal "domain" by one. If the next
		 * values exits the box boundaries we need to change it in the
		 * opposite direction because (max - min) must be a power of two.
		 * Also we initialize the extenal K value to -1 so that we can
		 * avoid extra conditions check inside the core loop.
		 */
		if(fmin > dmin)
		{
			kvdf[--fmin - 1] = -1;
		} else {
			++fmin;
		}

		if(fmax < dmax)
		{
			kvdf[++fmax + 1] = -1;
		} else {
			--fmax;
		}

		for(d = fmax; d >= fmin; d -= 2)
		{
			if(kvdf[d - 1] >= kvdf[d + 1])
			{
				i1 = kvdf[d - 1] + 1;
			} else {
				i1 = kvdf[d + 1];
			}
			prev1 = i1;
			i2 = i1 - d;

			for(; i1 < lim1 && i2 < lim2 && ha1[i1] == ha2[i2]; i1++,i2++)
			{
				;
			}

			if(i1 - prev1 > xenv->snake_cnt)
			{
				got_snake = 1;
			}
			kvdf[d] = i1;

			if(odd && bmin <= d && d <= bmax && kvdb[d] <= i1)
			{
				spl->i1 = i1;
				spl->i2 = i2;
				spl->min_lo = spl->min_hi = 1;
				return ec;
			}
		}

		/*
		 * We need to extent the diagonal "domain" by one. If the next
		 * values exits the box boundaries we need to change it in the
		 * opposite direction because (max - min) must be a power of two.
		 * Also we initialize the extenal K value to -1 so that we can
		 * avoid extra conditions check inside the core loop.
		 */
		if(bmin > dmin)
		{
			kvdb[--bmin - 1] = XDL_LINE_MAX;
		} else {
			++bmin;
		}

		if(bmax < dmax)
		{
			kvdb[++bmax + 1] = XDL_LINE_MAX;
		} else {
			--bmax;
		}

		for(d = bmax; d >= bmin; d -= 2)
		{
			if(kvdb[d - 1] < kvdb[d + 1])
			{
				i1 = kvdb[d - 1];
			} else {
				i1 = kvdb[d + 1] - 1;
			}
			prev1 = i1;
			i2 = i1 - d;

			for(; i1 > off1 && i2 > off2 && ha1[i1 - 1] == ha2[i2 - 1];
			        i1--,i2--)
			{
				;
			}

			if(prev1 - i1 > xenv->snake_cnt)
			{
				got_snake = 1;
			}
			kvdb[d] = i1;

			if(!odd && fmin <= d && d <= fmax && i1 <= kvdf[d])
			{
				spl->i1 = i1;
				spl->i2 = i2;
				spl->min_lo = spl->min_hi = 1;
				return ec;
			}
		}

		if(need_min)
		{
			continue;
		}

		/*
		 * If the edit cost is above the heuristic trigger and if
		 * we got a good snake, we sample current diagonals to see
		 * if some of the, have reached an "interesting" path. Our
		 * measure is a function of the distance from the diagonal
		 * corner (i1 + i2) penalized with the distance from the
		 * mid diagonal itself. If this value is above the current
		 * edit cost times a magic factor (XDL_K_HEUR) we consider
		 * it interesting.
		 */
		if(got_snake && ec > xenv->heur_min)
		{
			for(best = 0,d = fmax; d >= fmin; d -= 2)
			{
				dd = d > fmid ? d - fmid : fmid - d;
				i1 = kvdf[d];
				i2 = i1 - d;
				v = (i1 - off1) + (i2 - off2) - dd;

				if(v > XDL_K_HEUR * ec && v > best &&
				        off1 + xenv->snake_cnt <= i1 && i1 < lim1 &&
				        off2 + xenv->snake_cnt <= i2 && i2 < lim2)
				{
					for(k = 1; ha1[i1 - k] == ha2[i2 - k]; k++)
					{
						if(k == xenv->snake_cnt)
						{
							best = v;
							spl->i1 = i1;
							spl->i2 = i2;
							break;
						}
					}
				}
			}

			if(best > 0)
			{
				spl->min_lo = 1;
				spl->min_hi = 0;
				return ec;
			}

			for(best = 0,d = bmax; d >= bmin; d -= 2)
			{
				dd = d > bmid ? d - bmid : bmid - d;
				i1 = kvdb[d];
				i2 = i1 - d;
				v = (lim1 - i1) + (lim2 - i2) - dd;

				if(v > XDL_K_HEUR * ec && v > best && off1 < i1 &&
				        i1 <= lim1 - xenv->snake_cnt && off2 < i2 &&
				        i2 <= lim2 - xenv->snake_cnt)
				{
					for(k = 0; ha1[i1 + k] == ha2[i2 + k]; k++)
					{
						if(k == xenv->snake_cnt - 1)
						{
							best = v;
							spl->i1 = i1;
							spl->i2 = i2;
							break;
						}
					}
				}
			}

			if(best > 0)
			{
				spl->min_lo = 0;
				spl->min_hi = 1;
				return ec;
			}
		}

		/*
		 * Enough is enough. We spent too much time here and now we collect
		 * the furthest reaching path using the (i1 + i2) measure.
		 */
		if(ec >= xenv->mxcost)
		{
			long fbest,fbest1,bbest,bbest1;

			fbest = -1;

			for(d = fmax; d >= fmin; d -= 2)
			{
				i1 = XDL_MIN(kvdf[d],lim1);
				i2 = i1 - d;

				if(lim2 < i2)
				{
					i1 = lim2 + d,i2 = lim2;
				}

				if(fbest < i1 + i2)
				{
					fbest = i1 + i2;
					fbest1 = i1;
				}
			}

			bbest = XDL_LINE_MAX;

			for(d = bmax; d >= bmin; d -= 2)
			{
				i1 = XDL_MAX(off1,kvdb[d]);
				i2 = i1 - d;

				if(i2 < off2)
				{
					i1 = off2 + d,i2 = off2;
				}

				if(i1 + i2 < bbest)
				{
					bbest = i1 + i2;
					bbest1 = i1;
				}
			}

			if((lim1 + lim2) - bbest < fbest - (off1 + off2))
			{
				spl->i1 = fbest1;
				spl->i2 = fbest - fbest1;
				spl->min_lo = 1;
				spl->min_hi = 0;
			} else {
				spl->i1 = bbest1;
				spl->i2 = bbest - bbest1;
				spl->min_lo = 0;
				spl->min_hi = 1;
			}
			return ec;
		}
	}

	return -1;
}

/**
 * @brief Compares records between two sequences to find differences
 *
 * Rule: "Divide et Impera". Recursively split the box in sub-boxes by calling
 * the box splitting function. Note that the real job (marking changed lines)
 * is done in the two boundary reaching checks.
 */
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
){
	unsigned long const *ha1 = dd1->ha,*ha2 = dd2->ha;

	/*
	 * Shrink the box by walking through each diagonal snake (SW and NE).
	 */
	for(; off1 < lim1 && off2 < lim2 && ha1[off1] == ha2[off2]; off1++,off2++)
	{
		;
	}

	for(; off1 < lim1 && off2 < lim2 && ha1[lim1 - 1] == ha2[lim2 - 1];
	        lim1--,lim2--)
	{
		;
	}

	/*
	 * If one dimension is empty, then all records on the other one must
	 * be obviously changed.
	 */
	if(off1 == lim1)
	{
		char *rchg2 = dd2->rchg;
		long *rindex2 = dd2->rindex;

		for(; off2 < lim2; off2++)
		{
			rchg2[rindex2[off2]] = 1;
		}
	} else if(off2 == lim2){
		char *rchg1 = dd1->rchg;
		long *rindex1 = dd1->rindex;

		for(; off1 < lim1; off1++)
		{
			rchg1[rindex1[off1]] = 1;
		}
	} else {
		xdpsplit_t spl;

		/*
		 * Divide ...
		 */
		if(xdl_split(
			ha1,
			off1,
			lim1,
			ha2,
			off2,
			lim2,
			kvdf,
			kvdb,
			need_min,
			&spl,
			xenv) < 0)
		{
			return -1;
		}

		/*
		 * ... et Impera.
		 */
		if(xdl_recs_cmp(
			dd1,
			off1,
			spl.i1,
			dd2,
			off2,
			spl.i2,
			kvdf,
			kvdb,
			spl.min_lo,
			xenv) < 0 ||
		        xdl_recs_cmp(
			dd1,
			spl.i1,
			lim1,
			dd2,
			spl.i2,
			lim2,
			kvdf,
			kvdb,
			spl.min_hi,
			xenv) < 0)
		{
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Performs the main difference algorithm between two files
 *
 * @param mf1 First mmfile structure
 * @param mf2 Second mmfile structure
 * @param xpp Algorithm parameters
 * @param xe Environment structure to store results
 * @return int Returns 0 on success, -1 on error
 */
int xdl_do_diff(
	mmfile_t        *mf1,
	mmfile_t        *mf2,
	xpparam_t const *xpp,
	xdfenv_t        *xe
){
	long ndiags;
	long *kvd,*kvdf,*kvdb;
	xdalgoenv_t xenv;
	diffdata_t dd1,dd2;

	if(xdl_prepare_env(mf1,mf2,xpp,xe) < 0)
	{
		return -1;
	}

	/*
	 * Allocate and setup K vectors to be used by the differential algorithm.
	 * One is to store the forward path and one to store the backward path.
	 */
	ndiags = xe->xdf1.nreff + xe->xdf2.nreff + 3;

	if(!(kvd = (long *)xdl_malloc((2 * ndiags + 2) * sizeof(long))))
	{

		xdl_free_env(xe);
		return -1;
	}
	kvdf = kvd;
	kvdb = kvdf + ndiags;
	kvdf += xe->xdf2.nreff + 1;
	kvdb += xe->xdf2.nreff + 1;

	xenv.mxcost = xdl_bogosqrt(ndiags);

	if(xenv.mxcost < XDL_MAX_COST_MIN)
	{
		xenv.mxcost = XDL_MAX_COST_MIN;
	}
	xenv.snake_cnt = XDL_SNAKE_CNT;
	xenv.heur_min = XDL_HEUR_MIN_COST;

	dd1.nrec = xe->xdf1.nreff;
	dd1.ha = xe->xdf1.ha;
	dd1.rchg = xe->xdf1.rchg;
	dd1.rindex = xe->xdf1.rindex;
	dd2.nrec = xe->xdf2.nreff;
	dd2.ha = xe->xdf2.ha;
	dd2.rchg = xe->xdf2.rchg;
	dd2.rindex = xe->xdf2.rindex;

	if(xdl_recs_cmp(
		&dd1,
		0,
		dd1.nrec,
		&dd2,
		0,
		dd2.nrec,
		kvdf,
		kvdb,
		(xpp->flags & XDF_NEED_MINIMAL) != 0,
		&xenv) < 0)
	{

		xdl_free(kvd);
		xdl_free_env(xe);
		return -1;
	}

	xdl_free(kvd);

	return 0;
}

/**
 * @brief Adds a change record to the change script
 *
 * @param xscr Current change script
 * @param i1 Line number in first file
 * @param i2 Line number in second file
 * @param chg1 Number of lines changed in first file
 * @param chg2 Number of lines changed in second file
 * @return xdchange_t* Returns pointer to new change record or NULL on error
 */
static xdchange_t *
xdl_add_change(
	xdchange_t *xscr,
	long       i1,
	long       i2,
	long       chg1,
	long       chg2
){
	xdchange_t *xch;

	if(!(xch = (xdchange_t *)xdl_malloc(sizeof(xdchange_t))))
	{
		return NULL;
	}

	xch->next = xscr;
	xch->i1 = i1;
	xch->i2 = i2;
	xch->chg1 = chg1;
	xch->chg2 = chg2;

	return xch;
}

/**
 * @brief Compacts and optimizes change records for better output
 *
 * @param xdf Main file diff structure
 * @param xdfo Other file diff structure for comparison
 * @return int Returns 0 on success, -1 on error
 */
static int xdl_change_compact(
	xdfile_t *xdf,
	xdfile_t *xdfo
){
	long ix,ixo,ixref,grpsiz,nrec = xdf->nrec;
	char *rchg = xdf->rchg,*rchgo = xdfo->rchg;
	xrecord_t **recs = xdf->recs;

	/*
	 * This is the same of what GNU diff does. Move back and forward
	 * change groups for a consistent and pretty diff output. This also
	 * helps in finding joineable change groups and reduce the diff size.
	 */
	for(ix = ixo = 0;;)
	{
		/*
		 * Find the first changed line in the to-be-compacted file.
		 * We need to keep track of both indexes, so if we find a
		 * changed lines group on the other file, while scanning the
		 * to-be-compacted file, we need to skip it properly. Note
		 * that loops that are testing for changed lines on rchg* do
		 * not need index bounding since the array is prepared with
		 * a zero at position -1 and N.
		 */
		for(; ix < nrec && !rchg[ix]; ix++)
		{
			while(rchgo[ixo++])
			{
				;
			}
		}

		if(ix == nrec)
		{
			break;
		}

		/*
		 * Record the start of a changed-group in the to-be-compacted file
		 * and find the end of it, on both to-be-compacted and other file
		 * indexes (ix and ixo).
		 */
		long ixs = ix;

		for(ix++; rchg[ix]; ix++)
		{
			;
		}

		for(; rchgo[ixo]; ixo++)
		{
			;
		}

		do
		{
			grpsiz = ix - ixs;

			/*
			 * If the line before the current change group, is equal to
			 * the last line of the current change group, shift backward
			 * the group.
			 */
			while(ixs > 0 && recs[ixs - 1]->ha == recs[ix - 1]->ha &&
			        XDL_RECMATCH(recs[ixs - 1],recs[ix - 1]))
			{
				rchg[--ixs] = 1;
				rchg[--ix] = 0;

				/*
				 * This change might have joined two change groups,
				 * so we try to take this scenario in account by moving
				 * the start index accordingly (and so the other-file
				 * end-of-group index).
				 */
				for(; rchg[ixs - 1]; ixs--)
				{
					;
				}

				while(rchgo[--ixo])
				{
					;
				}
			}

			/*
			 * Record the end-of-group position in case we are matched
			 * with a group of changes in the other file (that is, the
			 * change record before the enf-of-group index in the other
			 * file is set).
			 */
			ixref = rchgo[ixo - 1] ? ix : nrec;

			/*
			 * If the first line of the current change group, is equal to
			 * the line next of the current change group, shift forward
			 * the group.
			 */
			while(ix < nrec && recs[ixs]->ha == recs[ix]->ha &&
			        XDL_RECMATCH(recs[ixs],recs[ix]))
			{
				rchg[ixs++] = 0;
				rchg[ix++] = 1;

				/*
				 * This change might have joined two change groups,
				 * so we try to take this scenario in account by moving
				 * the start index accordingly (and so the other-file
				 * end-of-group index). Keep tracking the reference
				 * index in case we are shifting together with a
				 * corresponding group of changes in the other file.
				 */
				for(; rchg[ix]; ix++)
				{
					;
				}

				while(rchgo[++ixo])
				{
					ixref = ix;
				}
			}
		} while(grpsiz != ix - ixs);

		/*
		 * Try to move back the possibly merged group of changes, to match
		 * the recorded postion in the other file.
		 */
		while(ixref < ix)
		{
			rchg[--ixs] = 1;
			rchg[--ix] = 0;

			while(rchgo[--ixo])
			{
				;
			}
		}
	}

	return 0;
}

/**
 * @brief Builds the change script from comparison results
 *
 * @param xe Diff environment containing comparison data
 * @param xscr Pointer to receive the generated change script
 * @return int Returns 0 on success, -1 on error
 */
int xdl_build_script(
	xdfenv_t   *xe,
	xdchange_t **xscr
){
	xdchange_t *cscr = NULL,*xch;
	char *rchg1 = xe->xdf1.rchg,*rchg2 = xe->xdf2.rchg;
	long i1,i2,l1,l2;

	/*
	 * Trivial. Collects "groups" of changes and creates an edit script.
	 */
	for(i1 = xe->xdf1.nrec,i2 = xe->xdf2.nrec; i1 >= 0 || i2 >= 0; i1--,i2--)
	{
		if(rchg1[i1 - 1] || rchg2[i2 - 1])
		{
			for(l1 = i1; rchg1[i1 - 1]; i1--)
			{
				;
			}

			for(l2 = i2; rchg2[i2 - 1]; i2--)
			{
				;
			}

			if(!(xch = xdl_add_change(cscr,i1,i2,l1 - i1,l2 - i2)))
			{
				xdl_free_script(cscr);
				return -1;
			}
			cscr = xch;
		}
	}

	*xscr = cscr;

	return 0;
}

/**
 * @brief Frees memory used by a change script
 *
 * @param xscr Change script to free
 */
void xdl_free_script(xdchange_t *xscr){
	xdchange_t *xch;

	while((xch = xscr) != NULL)
	{
		xscr = xscr->next;
		xdl_free(xch);
	}
}

/**
 * @brief Main entry point for diff generation between two files
 *
 * @param mf1 First file
 * @param mf2 Second file
 * @param xpp Diff algorithm parameters
 * @param xecfg Output configuration
 * @param ecb Output callback functions
 * @return int Returns 0 on success, -1 on error
 */
int xdl_diff(
	mmfile_t           *mf1,
	mmfile_t           *mf2,
	xpparam_t const    *xpp,
	xdemitconf_t const *xecfg,
	xdemitcb_t         *ecb
){
	xdchange_t *xscr;
	xdfenv_t xe;

	if(xdl_do_diff(mf1,mf2,xpp,&xe) < 0)
	{
		return -1;
	}

	if(xdl_change_compact(&xe.xdf1,&xe.xdf2) < 0 ||
	        xdl_change_compact(&xe.xdf2,&xe.xdf1) < 0 ||
	        xdl_build_script(&xe,&xscr) < 0)
	{

		xdl_free_env(&xe);
		return -1;
	}

	if(xscr)
	{
		if(xdl_emit_diff(&xe,xscr,ecb,xecfg) < 0)
		{

			xdl_free_script(xscr);
			xdl_free_env(&xe);
			return -1;
		}
		xdl_free_script(xscr);
	}
	xdl_free_env(&xe);

	return 0;
}

/**
 * @brief Output callback function for writing to file
 *
 * @param priv Private data pointer (FILE*)
 * @param mb Array of buffers to write
 * @param nbuf Number of buffers
 * @return int Returns 0 on success, -1 on error
 */
static int xdlt_outf(
	void       *priv,
	mmbuffer_t *mb,
	int        nbuf
){
	int i;

	for(i = 0; i < nbuf; i++)
	{
		if(!fwrite(mb[i].ptr,mb[i].size,1,(FILE *)priv))
		{
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Wrapper for standard malloc function
 *
 * @param priv Private data pointer (unused)
 * @param size Number of bytes to allocate
 * @return void* Returns allocated memory pointer
 */
void *wrap_malloc(
	void         *priv,
	unsigned int size
){
	return malloc(size);
}

/**
 * @brief Wrapper for standard free function
 *
 * @param priv Private data pointer (unused)
 * @param ptr Pointer to memory to free
 */
void wrap_free(
	void *priv,
	void *ptr
){
	free(ptr);
}

/**
 * @brief Wrapper for standard realloc function
 *
 * @param priv Private data pointer (unused)
 * @param ptr Pointer to existing memory block
 * @param size New size in bytes
 * @return void* Returns reallocated memory pointer
 */
void *wrap_realloc(
	void         *priv,
	void         *ptr,
	unsigned int size
){

	return realloc(ptr,size);
}

/**
 * @brief Gets record data at specified index
 *
 * @param xdf Diff file structure
 * @param ri Record index
 * @param rec Pointer to receive record data
 * @return long Returns record size
 */
static long xdl_get_rec(
	xdfile_t   *xdf,
	long       ri,
	char const **rec
){

	*rec = xdf->recs[ri]->ptr;

	return xdf->recs[ri]->size;
}

/**
 * @brief Emits a single record with prefix to output
 *
 * @param xdf Diff file structure
 * @param ri Record index
 * @param pre Prefix string to prepend
 * @param ecb Output callback
 * @return int Returns 0 on success, -1 on error
 */
static int
xdl_emit_record(
	xdfile_t   *xdf,
	long       ri,
	char const *pre,
	xdemitcb_t *ecb
){
	long size,psize = strlen(pre);
	char const *rec;

	size = xdl_get_rec(xdf,ri,&rec);

	if(xdl_emit_diffrec(rec,size,pre,psize,ecb) < 0)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief Finds the latest change atom for inclusion in diff hunk
 *
 * @param xscr Current change script
 * @param xecfg Emit configuration
 * @return xdchange_t* Returns pointer to last change in hunk
 *
 * Starting at the passed change atom, find the latest change atom to be
 * included inside the differential hunk according to the specified
 * configuration.
 */
static xdchange_t *xdl_get_hunk(
	xdchange_t         *xscr,
	xdemitconf_t const *xecfg
){
	xdchange_t *xch,*xchp;

	for(xchp = xscr,xch = xscr->next; xch; xchp = xch,xch = xch->next)
	{
		if(xch->i1 - (xchp->i1 + xchp->chg1) > 2 * xecfg->ctxlen)
		{
			break;
		}
	}

	return xchp;
}

/**
 * @brief Emits the complete diff output
 *
 * @param xe Diff environment
 * @param xscr Change script
 * @param ecb Output callback
 * @param xecfg Emit configuration
 * @return int Returns 0 on success, -1 on error
 */
int xdl_emit_diff(
	xdfenv_t           *xe,
	xdchange_t         *xscr,
	xdemitcb_t         *ecb,
	xdemitconf_t const *xecfg
){
	long s1,s2,e1,e2,lctx;
	xdchange_t *xch,*xche;

	for(xch = xche = xscr; xch; xch = xche->next)
	{
		xche = xdl_get_hunk(xch,xecfg);

		s1 = XDL_MAX(xch->i1 - xecfg->ctxlen,0);
		s2 = XDL_MAX(xch->i2 - xecfg->ctxlen,0);

		lctx = xecfg->ctxlen;
		lctx = XDL_MIN(lctx,xe->xdf1.nrec - (xche->i1 + xche->chg1));
		lctx = XDL_MIN(lctx,xe->xdf2.nrec - (xche->i2 + xche->chg2));

		e1 = xche->i1 + xche->chg1 + lctx;
		e2 = xche->i2 + xche->chg2 + lctx;

		/*
		 * Emit current hunk header.
		 */
		if(xecfg->str_meta && xdl_emit_hunk_hdr(s1 + 1,e1 - s1,s2 + 1,e2 - s2,ecb) < 0)
		{
			return -1;
		}

		/*
		 * Emit pre-context.
		 */
		for(; s1 < xch->i1; s1++)
		{
			if(xdl_emit_record(&xe->xdf1,s1," ",ecb) < 0)
			{
				return -1;
			}
		}

		for(s1 = xch->i1,s2 = xch->i2;; xch = xch->next)
		{
			/*
			 * Merge previous with current change atom.
			 */
			for(; s1 < xch->i1 && s2 < xch->i2; s1++,s2++)
			{
				if(xdl_emit_record(&xe->xdf1,s1," ",ecb) < 0)
				{
					return -1;
				}
			}

			/*
			 * Removes lines from the first file.
			 */
			for(s1 = xch->i1; s1 < xch->i1 + xch->chg1; s1++)
			{
				if(xdl_emit_record(&xe->xdf1,s1,"-",ecb) < 0)
				{
					return -1;
				}
			}

			/*
			 * Adds lines from the second file.
			 */
			for(s2 = xch->i2; s2 < xch->i2 + xch->chg2; s2++)
			{
				if(xdl_emit_record(&xe->xdf2,s2,"+",ecb) < 0)
				{
					return -1;
				}
			}

			if(xch == xche)
			{
				break;
			}
			s1 = xch->i1 + xch->chg1;
			s2 = xch->i2 + xch->chg2;
		}

		/*
		 * Emit post-context.
		 */
		for(s1 = xche->i1 + xche->chg1; s1 < e1; s1++)
		{
			if(xdl_emit_record(&xe->xdf1,s1," ",ecb) < 0)
			{
				return -1;
			}
		}
	}

	return 0;
}


typedef struct s_xdlclass {
	struct s_xdlclass *next;
	unsigned long ha;
	char const *line;
	long size;
	long idx;
} xdlclass_t;

typedef struct s_xdlclassifier {
	unsigned int hbits;
	long hsize;
	xdlclass_t **rchash;
	chastore_t ncha;
	long count;
} xdlclassifier_t;

static int xdl_init_classifier(
	xdlclassifier_t *cf,
	long            size
){
	long i;

	cf->hbits = xdl_hashbits((unsigned int)size);
	cf->hsize = 1 << cf->hbits;

	if(xdl_cha_init(&cf->ncha,sizeof(xdlclass_t),size / 4 + 1) < 0)
	{
		return -1;
	}

	if(!(cf->rchash =
	        (xdlclass_t **)xdl_malloc(cf->hsize * sizeof(xdlclass_t *))))
	{

		xdl_cha_free(&cf->ncha);
		return -1;
	}

	for(i = 0; i < cf->hsize; i++)
	{
		cf->rchash[i] = NULL;
	}

	cf->count = 0;

	return 0;
}

static void xdl_free_classifier(xdlclassifier_t *cf){
	xdl_free(cf->rchash);
	xdl_cha_free(&cf->ncha);
}

static int xdl_classify_record(
	xdlclassifier_t *cf,
	xrecord_t       **rhash,
	unsigned int    hbits,
	xrecord_t       *rec
){
	long hi;
	char const *line;
	xdlclass_t *rcrec;

	line = rec->ptr;
	hi = (long)XDL_HASHLONG(rec->ha,cf->hbits);

	for(rcrec = cf->rchash[hi]; rcrec; rcrec = rcrec->next)
	{
		if(rcrec->ha == rec->ha && rcrec->size == rec->size &&
		        !memcmp(line,rcrec->line,rec->size))
		{
			break;
		}
	}

	if(!rcrec)
	{
		if(!(rcrec = xdl_cha_alloc(&cf->ncha)))
		{
			return -1;
		}
		rcrec->idx = cf->count++;
		rcrec->line = line;
		rcrec->size = rec->size;
		rcrec->ha = rec->ha;
		rcrec->next = cf->rchash[hi];
		cf->rchash[hi] = rcrec;
	}

	rec->ha = (unsigned long)rcrec->idx;

	hi = (long)XDL_HASHLONG(rec->ha,hbits);
	rec->next = rhash[hi];
	rhash[hi] = rec;

	return 0;
}

static int xdl_prepare_ctx(
	mmfile_t        *mf,
	long            narec,
	xpparam_t const *xpp,
	xdlclassifier_t *cf,
	xdfile_t        *xdf
){
	unsigned int hbits;
	long i,nrec,hsize,bsize;
	unsigned long hav;
	char const *blk,*cur,*top,*prev;
	xrecord_t *crec;
	xrecord_t **recs,**rrecs;
	xrecord_t **rhash;
	unsigned long *ha;
	char *rchg;
	long *rindex;

	if(xdl_cha_init(&xdf->rcha,sizeof(xrecord_t),narec / 4 + 1) < 0)
	{
		return -1;
	}

	if(!(recs = (xrecord_t **)xdl_malloc(narec * sizeof(xrecord_t *))))
	{

		xdl_cha_free(&xdf->rcha);
		return -1;
	}

	hbits = xdl_hashbits((unsigned int)narec);
	hsize = 1 << hbits;

	if(!(rhash = (xrecord_t **)xdl_malloc(hsize * sizeof(xrecord_t *))))
	{

		xdl_free(recs);
		xdl_cha_free(&xdf->rcha);
		return -1;
	}

	for(i = 0; i < hsize; i++)
	{
		rhash[i] = NULL;
	}

	nrec = 0;

	if((cur = blk = xdl_mmfile_first(mf,&bsize)) != NULL)
	{
		for(top = blk + bsize;;)
		{
			if(cur >= top)
			{
				if(!(cur = blk = xdl_mmfile_next(mf,&bsize)))
				{
					break;
				}
				top = blk + bsize;
			}
			prev = cur;
			hav = xdl_hash_record(&cur,top);

			if(nrec >= narec)
			{
				narec *= 2;

				if(!(rrecs = (xrecord_t **)xdl_realloc(
					recs,narec * sizeof(xrecord_t *))))
				{

					xdl_free(rhash);
					xdl_free(recs);
					xdl_cha_free(&xdf->rcha);
					return -1;
				}
				recs = rrecs;
			}

			if(!(crec = xdl_cha_alloc(&xdf->rcha)))
			{

				xdl_free(rhash);
				xdl_free(recs);
				xdl_cha_free(&xdf->rcha);
				return -1;
			}
			crec->ptr = prev;
			crec->size = (long)(cur - prev);
			crec->ha = hav;
			recs[nrec++] = crec;

			if(xdl_classify_record(cf,rhash,hbits,crec) < 0)
			{

				xdl_free(rhash);
				xdl_free(recs);
				xdl_cha_free(&xdf->rcha);
				return -1;
			}
		}
	}

	if(!(rchg = (char *)xdl_malloc(nrec + 2)))
	{

		xdl_free(rhash);
		xdl_free(recs);
		xdl_cha_free(&xdf->rcha);
		return -1;
	}
	memset(rchg,0,nrec + 2);

	if(!(rindex = (long *)xdl_malloc((nrec + 1) * sizeof(long))))
	{

		xdl_free(rchg);
		xdl_free(rhash);
		xdl_free(recs);
		xdl_cha_free(&xdf->rcha);
		return -1;
	}

	if(!(ha = (unsigned long *)xdl_malloc((nrec + 1) * sizeof(unsigned long))))
	{

		xdl_free(rindex);
		xdl_free(rchg);
		xdl_free(rhash);
		xdl_free(recs);
		xdl_cha_free(&xdf->rcha);
		return -1;
	}

	xdf->nrec = nrec;
	xdf->recs = recs;
	xdf->hbits = hbits;
	xdf->rhash = rhash;
	xdf->rchg = rchg + 1;
	xdf->rindex = rindex;
	xdf->nreff = 0;
	xdf->ha = ha;
	xdf->dstart = 0;
	xdf->dend = nrec - 1;

	return 0;
}

static void xdl_free_ctx(xdfile_t *xdf){
	xdl_free(xdf->rhash);
	xdl_free(xdf->rindex);
	xdl_free(xdf->rchg - 1);
	xdl_free(xdf->ha);
	xdl_free(xdf->recs);
	xdl_cha_free(&xdf->rcha);
}

static int xdl_clean_mmatch(
	char const *dis,
	long       i,
	long       s,
	long       e
){
	long r,rdis0,rpdis0,rdis1,rpdis1;

	/*
	 * Limits the window the is examined during the similar-lines
	 * scan. The loops below stops when dis[i - r] == 1 (line that
	 * has no match), but there are corner cases where the loop
	 * proceed all the way to the extremities by causing huge
	 * performance penalties in case of big files.
	 */
	if(i - s > XDL_SIMSCAN_WINDOWN)
	{
		s = i - XDL_SIMSCAN_WINDOWN;
	}

	if(e - i > XDL_SIMSCAN_WINDOWN)
	{
		e = i + XDL_SIMSCAN_WINDOWN;
	}

	/*
	 * Scans the lines before 'i' to find a run of lines that either
	 * have no match (dis[j] == 0) or have multiple matches (dis[j] > 1).
	 * Note that we always call this function with dis[i] > 1, so the
	 * current line (i) is already a multimatch line.
	 */
	for(r = 1,rdis0 = 0,rpdis0 = 1; (i - r) >= s; r++)
	{
		if(!dis[i - r])
		{
			rdis0++;
		} else if(dis[i - r] == 2){
			rpdis0++;
		} else {
			break;
		}
	}

	/*
	 * If the run before the line 'i' found only multimatch lines, we
	 * return 0 and hence we don't make the current line (i) discarded.
	 * We want to discard multimatch lines only when they appear in the
	 * middle of runs with nomatch lines (dis[j] == 0).
	 */
	if(rdis0 == 0)
	{
		return 0;
	}

	for(r = 1,rdis1 = 0,rpdis1 = 1; (i + r) <= e; r++)
	{
		if(!dis[i + r])
		{
			rdis1++;
		} else if(dis[i + r] == 2){
			rpdis1++;
		} else {
			break;
		}
	}

	/*
	 * If the run after the line 'i' found only multimatch lines, we
	 * return 0 and hence we don't make the current line (i) discarded.
	 */
	if(rdis1 == 0)
	{
		return 0;
	}
	rdis1 += rdis0;
	rpdis1 += rpdis0;

	return rpdis1 * XDL_KPDIS_RUN < (rpdis1 + rdis1);
}

/*
 * Try to reduce the problem complexity, discard records that have no
 * matches on the other file. Also, lines that have multiple matches
 * might be potentially discarded if they happear in a run of discardable.
 */
static int xdl_cleanup_records(
	xdfile_t *xdf1,
	xdfile_t *xdf2
){
	long i,nm,rhi,nreff,mlim;
	unsigned long hav;
	xrecord_t **recs;
	xrecord_t *rec;
	char *dis,*dis1,*dis2;

	if(!(dis = (char *)xdl_malloc(xdf1->nrec + xdf2->nrec + 2)))
	{
		return -1;
	}
	memset(dis,0,xdf1->nrec + xdf2->nrec + 2);
	dis1 = dis;
	dis2 = dis1 + xdf1->nrec + 1;

	if((mlim = xdl_bogosqrt(xdf1->nrec)) > XDL_MAX_EQLIMIT)
	{
		mlim = XDL_MAX_EQLIMIT;
	}

	for(i = xdf1->dstart,recs = &xdf1->recs[xdf1->dstart]; i <= xdf1->dend;
	        i++,recs++)
	{
		hav = (*recs)->ha;
		rhi = (long)XDL_HASHLONG(hav,xdf2->hbits);

		for(nm = 0,rec = xdf2->rhash[rhi]; rec; rec = rec->next)
		{
			if(rec->ha == hav && ++nm == mlim)
			{
				break;
			}
		}
		dis1[i] = (nm == 0) ? 0 : (nm >= mlim) ? 2 : 1;
	}

	if((mlim = xdl_bogosqrt(xdf2->nrec)) > XDL_MAX_EQLIMIT)
	{
		mlim = XDL_MAX_EQLIMIT;
	}

	for(i = xdf2->dstart,recs = &xdf2->recs[xdf2->dstart]; i <= xdf2->dend;
	        i++,recs++)
	{
		hav = (*recs)->ha;
		rhi = (long)XDL_HASHLONG(hav,xdf1->hbits);

		for(nm = 0,rec = xdf1->rhash[rhi]; rec; rec = rec->next)
		{
			if(rec->ha == hav && ++nm == mlim)
			{
				break;
			}
		}
		dis2[i] = (nm == 0) ? 0 : (nm >= mlim) ? 2 : 1;
	}

	for(nreff = 0,i = xdf1->dstart,recs = &xdf1->recs[xdf1->dstart];
	        i <= xdf1->dend;
	        i++,recs++)
	{
		if(dis1[i] == 1 ||
		        (dis1[i] == 2 &&
		        !xdl_clean_mmatch(dis1,i,xdf1->dstart,xdf1->dend)))
		{
			xdf1->rindex[nreff] = i;
			xdf1->ha[nreff] = (*recs)->ha;
			nreff++;
		} else {
			xdf1->rchg[i] = 1;
		}
	}
	xdf1->nreff = nreff;

	for(nreff = 0,i = xdf2->dstart,recs = &xdf2->recs[xdf2->dstart];
	        i <= xdf2->dend;
	        i++,recs++)
	{
		if(dis2[i] == 1 ||
		        (dis2[i] == 2 &&
		        !xdl_clean_mmatch(dis2,i,xdf2->dstart,xdf2->dend)))
		{
			xdf2->rindex[nreff] = i;
			xdf2->ha[nreff] = (*recs)->ha;
			nreff++;
		} else {
			xdf2->rchg[i] = 1;
		}
	}
	xdf2->nreff = nreff;

	xdl_free(dis);

	return 0;
}

/*
 * Early trim initial and terminal matching records.
 */
static int xdl_trim_ends(
	xdfile_t *xdf1,
	xdfile_t *xdf2
){
	long i,lim;
	xrecord_t **recs1,**recs2;

	recs1 = xdf1->recs;
	recs2 = xdf2->recs;

	for(i = 0,lim = XDL_MIN(xdf1->nrec,xdf2->nrec); i < lim;
	        i++,recs1++,recs2++)
	{
		if((*recs1)->ha != (*recs2)->ha)
		{
			break;
		}
	}

	xdf1->dstart = xdf2->dstart = i;

	recs1 = xdf1->recs + xdf1->nrec - 1;
	recs2 = xdf2->recs + xdf2->nrec - 1;

	for(lim -= i,i = 0; i < lim; i++,recs1--,recs2--)
	{
		if((*recs1)->ha != (*recs2)->ha)
		{
			break;
		}
	}

	xdf1->dend = xdf1->nrec - i - 1;
	xdf2->dend = xdf2->nrec - i - 1;

	return 0;
}

static int xdl_optimize_ctxs(
	xdfile_t *xdf1,
	xdfile_t *xdf2
){
	if(xdl_trim_ends(xdf1,xdf2) < 0 || xdl_cleanup_records(xdf1,xdf2) < 0)
	{
		return -1;
	}

	return 0;
}

int xdl_prepare_env(
	mmfile_t        *mf1,
	mmfile_t        *mf2,
	xpparam_t const *xpp,
	xdfenv_t        *xe
){
	long enl1,enl2;
	xdlclassifier_t cf;

	enl1 = xdl_guess_lines(mf1) + 1;
	enl2 = xdl_guess_lines(mf2) + 1;

	if(xdl_init_classifier(&cf,enl1 + enl2 + 1) < 0)
	{
		return -1;
	}

	if(xdl_prepare_ctx(mf1,enl1,xpp,&cf,&xe->xdf1) < 0)
	{

		xdl_free_classifier(&cf);
		return -1;
	}

	if(xdl_prepare_ctx(mf2,enl2,xpp,&cf,&xe->xdf2) < 0)
	{

		xdl_free_ctx(&xe->xdf1);
		xdl_free_classifier(&cf);
		return -1;
	}

	xdl_free_classifier(&cf);

	if(xdl_optimize_ctxs(&xe->xdf1,&xe->xdf2) < 0)
	{

		xdl_free_ctx(&xe->xdf2);
		xdl_free_ctx(&xe->xdf1);
		return -1;
	}

	return 0;
}

void xdl_free_env(xdfenv_t *xe){
	xdl_free_ctx(&xe->xdf2);
	xdl_free_ctx(&xe->xdf1);
}


int xdlt_load_mmfile(
	char const *fname,
	mmfile_t   *mf,
	int        binmode
){
	int fd;
	long size;
	char *blk;

	if(xdl_init_mmfile(mf,XDLT_STD_BLKSIZE,XDL_MMF_ATOMIC) < 0)
	{
		return -1;
	}

	if((fd = open(fname,O_RDONLY)) == -1)
	{
		perror(fname);
		xdl_free_mmfile(mf);
		return -1;
	}
	size = lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);

	if(!(blk = (char *)xdl_mmfile_writeallocate(mf,size)))
	{
		xdl_free_mmfile(mf);
		close(fd);
		return -1;
	}

	if(read(fd,blk,(size_t)size) != (size_t)size)
	{
		perror(fname);
		xdl_free_mmfile(mf);
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
}

long xdl_bogosqrt(long n){
	long i;

	/*
	 * Classical integer square root approximation using shifts.
	 */
	for(i = 1; n > 0; n >>= 2)
	{
		i <<= 1;
	}

	return i;
}

int xdl_emit_diffrec(
	char const *rec,
	long       size,
	char const *pre,
	long       psize,
	xdemitcb_t *ecb
){
	int i = 2;
	mmbuffer_t mb[3];

	mb[0].ptr = (char *)pre;
	mb[0].size = psize;
	mb[1].ptr = (char *)rec;
	mb[1].size = size;

	if(size > 0 && rec[size - 1] != '\n')
	{
		mb[2].ptr = (char *)"\n\\ No newline at end of file\n";
		mb[2].size = strlen(mb[2].ptr);
		i++;
	}

	if(ecb->outf(ecb->priv,mb,i) < 0)
	{
		return -1;
	}

	return 0;
}

int xdl_init_mmfile(
	mmfile_t      *mmf,
	long          bsize,
	unsigned long flags
){

	mmf->flags = flags;
	mmf->head = mmf->tail = NULL;
	mmf->bsize = bsize;
	mmf->fsize = 0;
	mmf->rcur = mmf->wcur = NULL;
	mmf->rpos = 0;

	return 0;
}

void xdl_free_mmfile(mmfile_t *mmf){
	mmblock_t *cur,*tmp;

	for(cur = mmf->head; (tmp = cur) != NULL;)
	{
		cur = cur->next;
		xdl_free(tmp);
	}
}

void *xdl_mmfile_writeallocate(
	mmfile_t *mmf,
	long     size
){
	long bsize;
	mmblock_t *wcur;
	char *blk;

	if(!(wcur = mmf->wcur) || wcur->size + size > wcur->bsize)
	{
		bsize = XDL_MAX(mmf->bsize,size);

		if(!(wcur = (mmblock_t *)xdl_malloc(sizeof(mmblock_t) + bsize)))
		{
			return NULL;
		}
		wcur->flags = 0;
		wcur->ptr = (char *)wcur + sizeof(mmblock_t);
		wcur->size = 0;
		wcur->bsize = bsize;
		wcur->next = NULL;

		if(!mmf->head)
		{
			mmf->head = wcur;
		}

		if(mmf->tail)
		{
			mmf->tail->next = wcur;
		}
		mmf->tail = wcur;
		mmf->wcur = wcur;
	}

	blk = wcur->ptr + wcur->size;
	wcur->size += size;
	mmf->fsize += size;

	return blk;
}

void *xdl_mmfile_first(
	mmfile_t *mmf,
	long     *size
){

	if(!(mmf->rcur = mmf->head))
	{
		return NULL;
	}

	*size = mmf->rcur->size;

	return mmf->rcur->ptr;
}

void *xdl_mmfile_next(
	mmfile_t *mmf,
	long     *size
){

	if(!mmf->rcur || !(mmf->rcur = mmf->rcur->next))
	{
		return NULL;
	}

	*size = mmf->rcur->size;

	return mmf->rcur->ptr;
}

long xdl_mmfile_size(mmfile_t *mmf){
	return mmf->fsize;
}

int xdl_cha_init(
	chastore_t *cha,
	long       isize,
	long       icount
){

	cha->head = cha->tail = NULL;
	cha->isize = isize;
	cha->nsize = icount * isize;
	cha->ancur = cha->sncur = NULL;
	cha->scurr = 0;

	return 0;
}

void xdl_cha_free(chastore_t *cha){
	chanode_t *cur,*tmp;

	for(cur = cha->head; (tmp = cur) != NULL;)
	{
		cur = cur->next;
		xdl_free(tmp);
	}
}

void *xdl_cha_alloc(chastore_t *cha){
	chanode_t *ancur;
	void *data;

	if(!(ancur = cha->ancur) || ancur->icurr == cha->nsize)
	{
		if(!(ancur = (chanode_t *)xdl_malloc(sizeof(chanode_t) + cha->nsize)))
		{
			return NULL;
		}
		ancur->icurr = 0;
		ancur->next = NULL;

		if(cha->tail)
		{
			cha->tail->next = ancur;
		}

		if(!cha->head)
		{
			cha->head = ancur;
		}
		cha->tail = ancur;
		cha->ancur = ancur;
	}

	data = (char *)ancur + sizeof(chanode_t) + ancur->icurr;
	ancur->icurr += cha->isize;

	return data;
}

long xdl_guess_lines(mmfile_t *mf){
	long nl = 0,size,tsize = 0;
	char const *data,*cur,*top;

	if((cur = data = xdl_mmfile_first(mf,&size)) != NULL)
	{
		for(top = data + size; nl < XDL_GUESS_NLINES;)
		{
			if(cur >= top)
			{
				tsize += (long)(cur - data);

				if(!(cur = data = xdl_mmfile_next(mf,&size)))
				{
					break;
				}
				top = data + size;
			}
			nl++;

			if(!(cur = memchr(cur,'\n',top - cur)))
			{
				cur = top;
			} else {
				cur++;
			}
		}
		tsize += (long)(cur - data);
	}

	if(nl && tsize)
	{
		nl = xdl_mmfile_size(mf) / (tsize / nl);
	}

	return nl + 1;
}

unsigned long xdl_hash_record(
	char const **data,
	char const *top
){
	unsigned long ha = 5381;
	char const *ptr = *data;

	for(; ptr < top && *ptr != '\n'; ptr++)
	{
		ha += (ha << 5);
		ha ^= (unsigned long)*ptr;
	}
	*data = ptr < top ? ptr + 1 : ptr;

	return ha;
}

unsigned int xdl_hashbits(unsigned int size){
	unsigned int val = 1,bits = 0;

	for(; val < size && bits < CHAR_BIT * sizeof(unsigned int);
	        val <<= 1,bits++)
	{
		;
	}
	return bits ? bits : 1;
}

int xdl_num_out(
	char *out,
	long val
){
	char *ptr,*str = out;
	char buf[32];

	ptr = buf + sizeof(buf) - 1;
	*ptr = '\0';

	if(val < 0)
	{
		*--ptr = '-';
		val = -val;
	}

	for(; val && ptr > buf; val /= 10)
	{
		*--ptr = "0123456789"[val % 10];
	}

	if(*ptr)
	{
		for(; *ptr; ptr++,str++)
		{
			*str = *ptr;
		}
	} else {
		*str++ = '0';
	}
	*str = '\0';

	return str - out;
}

int xdl_emit_hunk_hdr(
	long       s1,
	long       c1,
	long       s2,
	long       c2,
	xdemitcb_t *ecb
){
	int nb = 0;
	mmbuffer_t mb;
	char buf[128];

	memcpy(buf,"@@ -",4);
	nb += 4;

	nb += xdl_num_out(buf + nb,c1 ? s1 : s1 - 1);

	memcpy(buf + nb,",",1);
	nb += 1;

	nb += xdl_num_out(buf + nb,c1);

	memcpy(buf + nb," +",2);
	nb += 2;

	nb += xdl_num_out(buf + nb,c2 ? s2 : s2 - 1);

	memcpy(buf + nb,",",1);
	nb += 1;

	nb += xdl_num_out(buf + nb,c2);

	memcpy(buf + nb," @@\n",4);
	nb += 4;

	mb.ptr = buf;
	mb.size = nb;

	if(ecb->outf(ecb->priv,&mb,1) < 0)
	{
		return -1;
	}

	return 0;
}

#if 0
int main(
	int  argc,
	char *argv[]
){
	int i = 1,ctxlen = 3,do_bdiff,do_bpatch;
	memallocator_t malt;
	mmfile_t mf1,mf2;
	xpparam_t xpp;
	xdemitconf_t xecfg;
	xdemitcb_t ecb;

	malt.priv = NULL;
	malt.malloc = wrap_malloc;
	malt.free = wrap_free;
	malt.realloc = wrap_realloc;
	xdl_set_allocator(&malt);

	do_bdiff = do_bpatch = 0;

	if(!strcmp(argv[i],"--diff"))
	{
		i++;

		for(; i < argc; i++)
		{
			if(strcmp(argv[i],"-C") == 0)
			{
				if(++i < argc)
				{
					ctxlen = atoi(argv[i]);
				}
			} else {
				break;
			}
		}
	}

	xpp.flags = 0;
	xecfg.ctxlen = ctxlen;

	if(xdlt_load_mmfile(argv[i],&mf1,do_bdiff || do_bpatch) < 0)
	{
		return 2;
	}

	if(xdlt_load_mmfile(argv[i + 1],&mf2,do_bdiff || do_bpatch) < 0)
	{
		xdl_free_mmfile(&mf1);
		return 2;
	}

	ecb.priv = stdout;
	ecb.outf = xdlt_outf;

	if(xdl_diff(&mf1,&mf2,&xpp,&xecfg,&ecb) < 0)
	{
		xdl_free_mmfile(&mf2);
		xdl_free_mmfile(&mf1);
		return 3;
	}

	xdl_free_mmfile(&mf2);
	xdl_free_mmfile(&mf1);

	return 0;
}
#endif

/**
 * Copyright (C) 2006 Sigbj�rn Skj�ret <cisc@c2i.net>
 * Copyright (C) 2006, 2010 Mark Olsen <bigfoot@private.dk>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * @file mos_libnix_so.c
 * @brief Handles the C library initialization of the shared objects
 * on MorphOS.
 */

#include <proto/exec.h>
#include <stdlib.h>

struct ExecBase   *SysBase;
struct DosLibrary *DOSBase;

struct CTDT
{
	int (*fp)(void);
	long priority;
};

extern const void(*__ctrslist[]) (void);
extern const void(*__dtrslist[]) (void);
extern const struct CTDT __ctdtlist[];
static struct CTDT       *sort_ctdt(struct CTDT **last);
static struct CTDT       *ctdt, *last_ctdt;

struct FuncSeg
{
	ULONG size;
	struct FuncSeg *next;
};

int              __nocommandline;
char             *__commandline;
unsigned long    __commandlen;
int              __argc;
char             **__argv;
struct WBStartup *_WBenchMsg;
char             *_ProgramName                   = "";
void             *libnix_mempool                 = 0L;
int              ThisRequiresConstructorHandling = 1;

/* ANSI de/constructor handler */
static void CallFuncArray(const void(*FuncArray[]) (void))
{
	struct FuncSeg *seg;
	int            i, num;

	seg = (struct FuncSeg *)(((IPTR)FuncArray) - sizeof(struct FuncSeg));
	num = (seg->size - sizeof(struct FuncSeg)) / sizeof(APTR);

	for (i = 0; (i < num) && FuncArray[i]; i++)
	{
		if (FuncArray[i] != ((const void (*)(void)) - 1))
		{
			(*FuncArray[i])();
		}
	}
}

static int comp_ctdt(struct CTDT *a, struct CTDT *b)
{
	if (a->priority == b->priority)
	{
		return (0);
	}
	if ((unsigned long)a->priority < (unsigned long) b->priority)
	{
		return (-1);
	}

	return (1);
}

static struct CTDT *sort_ctdt(struct CTDT **last)
{
	struct FuncSeg *seg;
	struct CTDT    *last_ctdt;

	seg       = (struct FuncSeg *)(((IPTR)__ctdtlist) - sizeof(struct FuncSeg));
	last_ctdt = (struct CTDT *)(((IPTR)seg) + seg->size);

	qsort((struct CTDT *)__ctdtlist, (IPTR)(last_ctdt - __ctdtlist), sizeof(*__ctdtlist), (int (*)(const void *, const void *))comp_ctdt);

	*last = last_ctdt;

	return ((struct CTDT *) __ctdtlist);
}

int morphos_so_init(void)
{
	SysBase = *(struct ExecBase **)4;

	DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
	if (DOSBase)
	{
		/* Sort SAS/C de/constructor list */
		ctdt = sort_ctdt(&last_ctdt);

		/* Call SAS/C constructors */
		while (ctdt < last_ctdt)
		{
			if (ctdt->priority >= 0)
			{
				if (ctdt->fp() != 0)
				{
					break;
				}
			}

			ctdt++;
		}

		if (ctdt >= last_ctdt)
		{
			/* Preinit memory-system */
			Com_Allocate(0);

			/* Call ANSI constructors */
			CallFuncArray(__ctrslist);

			return -1;
		}

		CloseLibrary((struct Library *)DOSBase);
	}

	return 0;
}

void morphos_so_deinit(void)
{
	/* Call ANSI destructors */
	if (ctdt == last_ctdt)
	{
		CallFuncArray(__dtrslist);
	}

	/* Call SAS/C destructors */
	ctdt = (struct CTDT *)__ctdtlist;
	while (ctdt < last_ctdt)
	{
		if (ctdt->priority < 0)
		{
			if (ctdt->fp != (int (*)(void)) - 1)
			{
				ctdt->fp();
			}
		}

		ctdt++;
	}
}

/* Disable CTRL-C check in libc routines */
void __chkabort(void)
{
}

/* We can't allow abort() or exit()! */
void abort(void)
{
	for (;; )
		Wait(0);
}
void exit(int i)
{
	for (;; )
		Wait(0);
}

__asm("\n.long __nocommandline\n");
/* De/constructor section-placeholders (MUST be last in the source (don't compile this with -O3)!) */
__asm("\n.section \".ctdt\",\"a\",@progbits\n__ctdtlist:\n.long -1,-1\n");
__asm("\n.section \".ctors\",\"a\",@progbits\n__ctrslist:\n.long -1\n");
__asm("\n.section \".dtors\",\"a\",@progbits\n__dtrslist:\n.long -1\n");

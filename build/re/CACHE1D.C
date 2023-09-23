// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
// This file has been modified from Ken Silverman's original release

#include <dos.h>
#include <stdio.h>
#include "pragmas.h"

//   This module keeps track of a standard linear cacheing system.
//   To use this module, here's all you need to do:
//
//   Step 1: Allocate a nice BIG buffer, like from 1MB-4MB and
//           Call initcache(long cachestart, long cachesize) where
//
//              cachestart = (long)(pointer to start of BIG buffer)
//              cachesize = length of BIG buffer
//
//   Step 2: Call allocache(long *bufptr, long bufsiz, char *lockptr)
//              whenever you need to allocate a buffer, where:
//
//              *bufptr = pointer to 4-byte pointer to buffer
//                 Confused?  Using this method, cache2d can remove
//                 previously allocated things from the cache safely by
//                 setting the 4-byte pointer to 0.
//              bufsiz = number of bytes to allocate
//              *lockptr = pointer to locking char which tells whether
//                 the region can be removed or not.  If *lockptr = 0 then
//                 the region is not locked else its locked.
//
//   Step 3: If you need to remove everything from the cache, or every
//           unlocked item from the cache, you can call uninitcache();
//              Call uninitcache(0) to remove all unlocked items, or
//              Call uninitcache(1) to remove everything.
//           After calling uninitcache, it is still ok to call allocache
//           without first calling initcache.

#if (LIBVER_BUILDREV < 19960427L)
#define MAXCACHEOBJECTS 4096
#elif (LIBVER_BUILDREV < 20000614L)
#define MAXCACHEOBJECTS 6144
#else
#define MAXCACHEOBJECTS 9216
#endif

#if (LIBVER_BUILDREV < 19960427L) // VERSIONS RESTORATION - Convenience macros
#define CAC_HAND(i) handptr[i]
#define CAC_LENG(i) cacleng[i]
#define CAC_LOCK(i) lockptr[i]
#else
#define CAC_HAND(i) cac[i].hand
#define CAC_LENG(i) cac[i].leng
#define CAC_LOCK(i) cac[i].lock
#endif
#if (LIBVER_BUILDREV < 19960427L)
long cachecount = 0;
static char zerochar = 0;
static long cachestart = 0, cachesize = 0, cacnum = 0, agecount = 0;
static long *handptr[MAXCACHEOBJECTS];
static long cacleng[MAXCACHEOBJECTS];
static char *lockptr[MAXCACHEOBJECTS];
#else
static long cachesize = 0;
long cachecount = 0;
char zerochar = 0;
long cachestart = 0, cacnum = 0, agecount = 0;
typedef struct { long *hand, leng; char *lock; } cactype;
cactype cac[MAXCACHEOBJECTS];
static long lockrecip[200];
#endif
#if (LIBVER_BUILDREV < 20000614L) // VERSIONS RESTORATION - HACK
#define reportandexit_wrapper(errormessage) do { printf(errormessage "\n"); reportandexit(); } while (0)
#else
#define reportandexit_wrapper(errormessage) reportandexit(errormessage)
#endif

initcache(long dacachestart, long dacachesize)
{
#if (LIBVER_BUILDREV >= 19960427L)
	long i;

	for(i=1;i<200;i++) lockrecip[i] = (1<<28)/(200-i);

#endif
	cachestart = dacachestart;
	cachesize = dacachesize;

	CAC_LENG(0) = cachesize;
	CAC_LOCK(0) = &zerochar;
	cacnum = 1;
}

allocache (long *newhandle, long newbytes, char *newlockptr)
{
	long i, j, z, zz, bestz, daval, bestval, besto, o1, o2, sucklen, suckz;

#if (LIBVER_BUILDREV >= 19960319L)
	newbytes = ((newbytes+15)&0xfffffff0);
#endif

#if (LIBVER_BUILDREV < 19960925L)
	if (newbytes > cachesize)
#else
	if ((unsigned)newbytes > (unsigned)cachesize)
#endif
	{
		printf("Cachesize: %ld\n",cachesize);
		printf("*Newhandle: 0x%x, Newbytes: %ld, *Newlock: %d\n",newhandle,newbytes,*newlockptr);
		reportandexit_wrapper("BUFFER TOO BIG TO FIT IN CACHE!");
	}

	if (*newlockptr == 0)
	{
		reportandexit_wrapper("ALLOCACHE CALLED WITH LOCK OF 0!");
	}

		//Find best place
#if (LIBVER_BUILDREV < 19960427L)
	bestval = 0x7fffffff; o1 = 0;
	for(z=0;z<cacnum;z++)
#else
	bestval = 0x7fffffff; o1 = cachesize;
	for(z=cacnum-1;z>=0;z--)
#endif
	{
#if (LIBVER_BUILDREV < 19960427L)
		o2 = o1+newbytes; if (o2 > cachesize) break;
#else
		o1 -= CAC_LENG(z);
		o2 = o1+newbytes; if (o2 > cachesize) continue;
#endif

		daval = 0;
#if (LIBVER_BUILDREV < 19960427L)
		i=o1;zz=z;
		while(i<o2)
		{
			if (*CAC_LOCK(zz) >= 200) { daval = 0x7fffffff; break; }
			if (*CAC_LOCK(zz) > 0)
				daval += divscale4((CAC_LENG(zz)>>10)+64,200-*CAC_LOCK(zz));
			i+=CAC_LENG(zz);zz++;
		}
#else
		for(i=o1,zz=z;i<o2;i+=CAC_LENG(zz++))
		{
			if (*CAC_LOCK(zz) == 0) continue;
			if (*CAC_LOCK(zz) >= 200) { daval = 0x7fffffff; break; }
			daval += mulscale32(CAC_LENG(zz)+65536,lockrecip[*CAC_LOCK(zz)]);
			if (daval >= bestval) break;
		}
#endif
		if (daval < bestval)
		{
			bestval = daval; besto = o1; bestz = z;
#if (LIBVER_BUILDREV >= 19960427L)
			if (bestval == 0) break;
#endif
		}
#if (LIBVER_BUILDREV < 19960427L)
		o1 += CAC_LENG(z);
#endif
	}

	//printf("%ld %ld %ld\n",besto,newbytes,*newlockptr);

	if (bestval == 0x7fffffff)
		reportandexit_wrapper("CACHE SPACE ALL LOCKED UP!");

		//Suck things out
	for(sucklen=-newbytes,suckz=bestz;sucklen<0;sucklen+=CAC_LENG(suckz++))
		if (*CAC_LOCK(suckz)) *CAC_HAND(suckz) = 0;

		//Remove all blocks except 1
	suckz -= (bestz+1); cacnum -= suckz;
#if (LIBVER_BUILDREV < 19960427L)
	for(z=bestz;z<cacnum;z++)
	{
		CAC_HAND(z) = CAC_HAND(z+suckz);
		CAC_LENG(z) = CAC_LENG(z+suckz);
		CAC_LOCK(z) = CAC_LOCK(z+suckz);
	}
#elif (LIBVER_BUILDREV < 19961012L) // VERSIONS RESTORATION - See BUILD2.TXT, 12/13/96 (Plutonium Pak note)
	copybuf(&cac[bestz+suckz],&cac[bestz],(cacnum-bestz)*sizeof(cactype));
#else
	copybufbyte(&cac[bestz+suckz],&cac[bestz],(cacnum-bestz)*sizeof(cactype));
#endif
	CAC_HAND(bestz) = newhandle; *newhandle = cachestart+besto;
	CAC_LENG(bestz) = newbytes;
	CAC_LOCK(bestz) = newlockptr;
	cachecount++;

		//Add new empty block if necessary
	if (sucklen <= 0) return;

	bestz++;
	if (bestz == cacnum)
	{
#if (LIBVER_BUILDREV < 20000614L)
		cacnum++;
#else
		cacnum++; if (cacnum > MAXCACHEOBJECTS) reportandexit("Too many objects in cache! (cacnum > MAXCACHEOBJECTS)");
#endif
		CAC_LENG(bestz) = sucklen;
		CAC_LOCK(bestz) = &zerochar;
		return;
	}

	if (*CAC_LOCK(bestz) == 0) { CAC_LENG(bestz) += sucklen; return; }

#if (LIBVER_BUILDREV < 20000614L)
	cacnum++;
#else
	cacnum++; if (cacnum > MAXCACHEOBJECTS) reportandexit("Too many objects in cache! (cacnum > MAXCACHEOBJECTS)");
#endif
#if (LIBVER_BUILDREV < 19960925L)
	for(z=cacnum-1;z>bestz;z--) { CAC_HAND(z) = CAC_HAND(z-1); CAC_LENG(z) = CAC_LENG(z-1); CAC_LOCK(z) = CAC_LOCK(z-1); }
#else
	for(z=cacnum-1;z>bestz;z--) cac[z] = cac[z-1];
#endif
	CAC_LENG(bestz) = sucklen;
	CAC_LOCK(bestz) = &zerochar;
}

#if (LIBVER_BUILDREV >= 19960925L)
suckcache (long *suckptr)
{
	long i;

		//Can't exit early, because invalid pointer might be same even though lock = 0
	for(i=0;i<cacnum;i++)
		if ((long)(*CAC_HAND(i)) == (long)suckptr)
		{
			if (*CAC_LOCK(i)) *CAC_HAND(i) = 0;
			CAC_LOCK(i) = &zerochar;
			CAC_HAND(i) = 0;

				//Combine empty blocks
			if ((i > 0) && (*CAC_LOCK(i-1) == 0))
			{
				CAC_LENG(i-1) += CAC_LENG(i);
				cacnum--; copybuf(&cac[i+1],&cac[i],(cacnum-i)*sizeof(cactype));
			}
			else if ((i < cacnum-1) && (*CAC_LOCK(i+1) == 0))
			{
				CAC_LENG(i+1) += CAC_LENG(i);
				cacnum--; copybuf(&cac[i+1],&cac[i],(cacnum-i)*sizeof(cactype));
			}
		}
}
#endif // LIBVER_BUILDREV

agecache()
{
	long cnt;
	char ch;

	if (agecount >= cacnum) agecount = cacnum-1;
	for(cnt=(cacnum>>4);cnt>=0;cnt--)
	{
		ch = (*CAC_LOCK(agecount));
		if (((ch-2)&255) < 198)
			(*CAC_LOCK(agecount)) = ch-1;

		agecount--; if (agecount < 0) agecount = cacnum-1;
	}
}

#if (LIBVER_BUILDREV < 20000614L) // VERSIONS RESTORATION - HACK
reportandexit()
#else
reportandexit(char *errormessage)
#endif
{
	long i, j;

	setvmode(0x3);
#if (LIBVER_BUILDREV < 20000614L)
	printf("Cachesize = %ld\n",cachesize);
	printf("Cacnum = %ld\n",cacnum);
#endif
	j = 0;
	for(i=0;i<cacnum;i++)
	{
		printf("%ld- ",i);
		printf("ptr: 0x%x, ",*CAC_HAND(i));
		printf("leng: %ld, ",CAC_LENG(i));
		printf("lock: %ld\n",*CAC_LOCK(i));
		j += CAC_LENG(i);
	}
#if (LIBVER_BUILDREV >= 20000614L)
	printf("Cachesize = %ld\n",cachesize);
	printf("Cacnum = %ld\n",cacnum);
#endif
	printf("Cache length sum = %ld\n",j);
#if (LIBVER_BUILDREV >= 20000614L)
	printf("ERROR: %s",errormessage);
#endif
	exit(0);
}




#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

#if (LIBVER_BUILDREV >= 19960427L)
#define MAXGROUPFILES 4     //Warning: Fix groupfil if this is changed
#endif
#define MAXOPENFILES 64     //Warning: Fix filehan if this is changed

#if (LIBVER_BUILDREV < 19960427L)
static long numfiles;
static long groupfil = -1;
static long groupfilpos;
static char *filelist;
static long *fileoffs;
#else
static char toupperlookup[256] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
};

static long numgroupfiles = 0;
static long gnumfiles[MAXGROUPFILES];
static long groupfil[MAXGROUPFILES] = {-1,-1,-1,-1};
static long groupfilpos[MAXGROUPFILES];
static char *gfilelist[MAXGROUPFILES];
static long *gfileoffs[MAXGROUPFILES];
#endif // LIBVER_BUILDREV

#if (LIBVER_BUILDREV < 19960427L)
#define filehan filehandle // VERSIONS RESTORATION - Convenience macro
#else
static char filegrp[MAXOPENFILES];
#endif
static long filepos[MAXOPENFILES];
static long filehan[MAXOPENFILES] =
{
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

initgroupfile(char *filename)
{
	char buf[16];
	long i, j, k;

#if (LIBVER_BUILDREV < 19960427L)
	uninitgroupfile();

	groupfil = open(filename,O_BINARY|O_RDWR,S_IREAD);
	if (groupfil != -1)
#else
	if (numgroupfiles >= MAXGROUPFILES) return(-1);

	groupfil[numgroupfiles] = open(filename,O_BINARY|O_RDWR,S_IREAD);
	if (groupfil[numgroupfiles] != -1)
#endif
	{
#if (LIBVER_BUILDREV < 19960427L)
		groupfilpos = 0;
		read(groupfil,buf,16);
#else
		groupfilpos[numgroupfiles] = 0;
		read(groupfil[numgroupfiles],buf,16);
#endif
		if ((buf[0] != 'K') || (buf[1] != 'e') || (buf[2] != 'n') ||
			 (buf[3] != 'S') || (buf[4] != 'i') || (buf[5] != 'l') ||
			 (buf[6] != 'v') || (buf[7] != 'e') || (buf[8] != 'r') ||
			 (buf[9] != 'm') || (buf[10] != 'a') || (buf[11] != 'n'))
		{
#if (LIBVER_BUILDREV < 19960427L)
			close(groupfil);
			groupfil = -1;
#else
			close(groupfil[numgroupfiles]);
			groupfil[numgroupfiles] = -1;
#endif
			return(-1);
		}
#if (LIBVER_BUILDREV < 19960427L)
		numfiles = (buf[12])+(buf[13]<<8)+(buf[14]<<16)+(buf[15]<<24);

		if ((filelist = (char *)kmalloc(numfiles<<4)) == 0)
			{ printf("Not enough memory for file grouping system\n"); exit(0); }
		if ((fileoffs = (long *)kmalloc((numfiles+1)<<2)) == 0)
			{ printf("Not enough memory for file grouping system\n"); exit(0); }

		read(groupfil,filelist,numfiles<<4);
#else
		gnumfiles[numgroupfiles] = *((long *)&buf[12]);

		if ((gfilelist[numgroupfiles] = (char *)kmalloc(gnumfiles[numgroupfiles]<<4)) == 0)
			{ printf("Not enough memory for file grouping system\n"); exit(0); }
		if ((gfileoffs[numgroupfiles] = (long *)kmalloc((gnumfiles[numgroupfiles]+1)<<2)) == 0)
			{ printf("Not enough memory for file grouping system\n"); exit(0); }

		read(groupfil[numgroupfiles],gfilelist[numgroupfiles],gnumfiles[numgroupfiles]<<4);
#endif

		j = 0;
#if (LIBVER_BUILDREV < 19960427L)
		for(i=0;i<numfiles;i++)
		{
			k = (filelist[(i<<4)+12])
			  + (filelist[(i<<4)+13]<<8)
			  + (filelist[(i<<4)+14]<<16)
			  + (filelist[(i<<4)+15]<<24);
			filelist[(i<<4)+12] = 0;
			fileoffs[i] = j;
			j += k;
		}
		fileoffs[numfiles] = j;
#else
		for(i=0;i<gnumfiles[numgroupfiles];i++)
		{
			k = *((long *)&gfilelist[numgroupfiles][(i<<4)+12]);
			gfilelist[numgroupfiles][(i<<4)+12] = 0;
			gfileoffs[numgroupfiles][i] = j;
			j += k;
		}
		gfileoffs[numgroupfiles][gnumfiles[numgroupfiles]] = j;
#endif
	}
#if (LIBVER_BUILDREV < 19960427L)
	return(groupfil);
#else
	numgroupfiles++;
	return(groupfil[numgroupfiles-1]);
#endif
}

uninitgroupfile()
{
#if (LIBVER_BUILDREV < 19960427L)
	if (groupfil != -1)
	{
		kfree(filelist);
		kfree(fileoffs);
		close(groupfil);
		groupfil = -1;
	}
#else
	long i;

	for(i=numgroupfiles-1;i>=0;i--)
		if (groupfil[i] != -1)
		{
			kfree(gfilelist[i]);
			kfree(gfileoffs[i]);
			close(groupfil[i]);
			groupfil[i] = -1;
		}
#endif
}

kopen4load(char *filename, char searchfirst)
{
#if (LIBVER_BUILDREV < 19960427L)
	long i, j, fil, newhandle;
	char ch1, ch2, bad;
#else
	long i, j, k, fil, newhandle;
	char bad, *gfileptr;
#endif

	newhandle = MAXOPENFILES-1;
	while (filehan[newhandle] != -1)
	{
		newhandle--;
		if (newhandle < 0)
		{
			printf("TOO MANY FILES OPEN IN FILE GROUPING SYSTEM!");
			exit(0);
		}
	}

	if (searchfirst == 0)
		if ((fil = open(filename,O_BINARY|O_RDONLY)) != -1)
		{
#if (LIBVER_BUILDREV < 19960427L)
			filehan[newhandle] = fil;
			filepos[newhandle] = -1;
#else
			filegrp[newhandle] = 255;
			filehan[newhandle] = fil;
			filepos[newhandle] = 0;
#endif
			return(newhandle);
		}
#if (LIBVER_BUILDREV < 19960427L)
	if (groupfil != -1)
	{
		for(i=numfiles-1;i>=0;i--)
		{
			j = 0; bad = 0;
			while (filename[j]!=0&&j<13)
			{
				ch1 = filename[j];
				if (ch1 >= 'a' && ch1 <= 'z'+1)
					ch1 -= 32;
				ch2 = filelist[(i<<4)+j];
				if (ch2 >= 'a' && ch2 <= 'z'+1)
					ch2 -= 32;
				if (ch1 != ch2)
					{ bad = 1; break; }
				j++;
			}
			if (bad) continue;

			filehan[newhandle] = i;
			filepos[newhandle] = 0;
			return(newhandle);
		}
	}
#else // LIBVER_BUILDREV
	for(k=numgroupfiles-1;k>=0;k--)
	{
		if (searchfirst != 0) k = 0;
		if (groupfil[k] != -1)
		{
			for(i=gnumfiles[k]-1;i>=0;i--)
			{
				gfileptr = (char *)&gfilelist[k][i<<4];

				bad = 0;
				for(j=0;j<13;j++)
				{
					if (!filename[j]) break;
					if (toupperlookup[filename[j]] != toupperlookup[gfileptr[j]])
						{ bad = 1; break; }
				}
				if (bad) continue;

				filegrp[newhandle] = k;
				filehan[newhandle] = i;
				filepos[newhandle] = 0;
				return(newhandle);
			}
		}
	}
#endif // LIBVER_BUILDREV
	return(-1);
}

kread(long handle, void *buffer, long leng)
{
#if (LIBVER_BUILDREV < 19960427L)
	long i, j, filenum;

	filenum = filehan[handle];
	if (filepos[handle] < 0) return(read(filenum,buffer,leng));

	if (groupfil != -1)
	{
		i = fileoffs[filenum]+filepos[handle];
		if (i != groupfilpos)
		{
			lseek(groupfil,i+((numfiles+1)<<4),SEEK_SET);
			groupfilpos = i;
		}
		leng = min(leng,(fileoffs[filenum+1]-fileoffs[filenum])-filepos[handle]);
		leng = read(groupfil,buffer,leng);
		filepos[handle] += leng;
		groupfilpos += leng;
		return(leng);
	}
#else // LIBVER_BUILDREV
	long i, j, filenum, groupnum;

	filenum = filehan[handle];
	groupnum = filegrp[handle];
	if (groupnum == 255) return(read(filenum,buffer,leng));

	if (groupfil[groupnum] != -1)
	{
		i = gfileoffs[groupnum][filenum]+filepos[handle];
		if (i != groupfilpos[groupnum])
		{
			lseek(groupfil[groupnum],i+((gnumfiles[groupnum]+1)<<4),SEEK_SET);
			groupfilpos[groupnum] = i;
		}
		leng = min(leng,(gfileoffs[groupnum][filenum+1]-gfileoffs[groupnum][filenum])-filepos[handle]);
		leng = read(groupfil[groupnum],buffer,leng);
		filepos[handle] += leng;
		groupfilpos[groupnum] += leng;
		return(leng);
	}
#endif // LIBVER_BUILDREV

	return(0);
}

klseek(long handle, long offset, long whence)
{
#if (LIBVER_BUILDREV < 19960427L)
	long i;

	if (filepos[handle] < 0) return(lseek(filehan[handle],offset,whence));
	if (groupfil != -1)
#else
	long i, groupnum;

	groupnum = filegrp[handle];

	if (groupnum == 255) return(lseek(filehan[handle],offset,whence));
	if (groupfil[groupnum] != -1)
#endif
	{
		switch(whence)
		{
			case SEEK_SET: filepos[handle] = offset; break;
			case SEEK_END: i = filehan[handle];
#if (LIBVER_BUILDREV < 19960427L)
								filepos[handle] = (fileoffs[i+1]-fileoffs[i])+offset;
#else
								filepos[handle] = (gfileoffs[groupnum][i+1]-gfileoffs[groupnum][i])+offset;
#endif
								break;
			case SEEK_CUR: filepos[handle] += offset; break;
		}
		return(filepos[handle]);
	}
	return(-1);
}

kfilelength(long handle)
{
#if (LIBVER_BUILDREV < 19960427L)
	long i;

	if (filepos[handle] < 0) return(filelength(filehan[handle]));
	i = filehan[handle];
	return(fileoffs[i+1]-fileoffs[i]);
#else
	long i, groupnum;

	groupnum = filegrp[handle];
	if (groupnum == 255) return(filelength(filehan[handle]));
	i = filehan[handle];
	return(gfileoffs[groupnum][i+1]-gfileoffs[groupnum][i]);
#endif
}

kclose(long handle)
{
	if (handle < 0) return;
#if (LIBVER_BUILDREV < 19960427L)
	if (filepos[handle] < 0) close(filehan[handle]);
#else
	if (filegrp[handle] == 255) close(filehan[handle]);
#endif
	filehan[handle] = -1;
}




	//Internal LZW variables
#define LZWSIZE 16384           //Watch out for shorts!
#if (LIBVER_BUILDREV < 19960427L)
static char *lzwbuf1;
static short *lzwbuf2, *lzwbuf3;
static char *lzwbuf4, *lzwbuf5, lzwbuflock[5];
#else
static char *lzwbuf1, *lzwbuf4, *lzwbuf5, lzwbuflock[5];
static short *lzwbuf2, *lzwbuf3;
#endif

kdfread(void *buffer, size_t dasizeof, size_t count, long fil)
{
	long i, j, k, kgoal;
	short leng;
	char *ptr;

	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 200;
	if (lzwbuf1 == NULL) allocache((long *)&lzwbuf1,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[0]);
	if (lzwbuf2 == NULL) allocache((long *)&lzwbuf2,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[1]);
	if (lzwbuf3 == NULL) allocache((long *)&lzwbuf3,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[2]);
	if (lzwbuf4 == NULL) allocache((long *)&lzwbuf4,LZWSIZE,&lzwbuflock[3]);
	if (lzwbuf5 == NULL) allocache((long *)&lzwbuf5,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[4]);

	if (dasizeof > LZWSIZE) { count *= dasizeof; dasizeof = 1; }
	ptr = (char *)buffer;

	kread(fil,&leng,2); kread(fil,lzwbuf5,(long)leng);
	k = 0; kgoal = uncompress(lzwbuf5,(long)leng,lzwbuf4);

	copybufbyte(lzwbuf4,ptr,(long)dasizeof);
	k += (long)dasizeof;

	for(i=1;i<count;i++)
	{
		if (k >= kgoal)
		{
			kread(fil,&leng,2); kread(fil,lzwbuf5,(long)leng);
			k = 0; kgoal = uncompress(lzwbuf5,(long)leng,lzwbuf4);
		}
		for(j=0;j<dasizeof;j++) ptr[j+dasizeof] = ((ptr[j]+lzwbuf4[j+k])&255);
		k += dasizeof;
		ptr += dasizeof;
	}
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 1;
}

dfread(void *buffer, size_t dasizeof, size_t count, FILE *fil)
{
	long i, j, k, kgoal;
	short leng;
	char *ptr;

	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 200;
	if (lzwbuf1 == NULL) allocache((long *)&lzwbuf1,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[0]);
	if (lzwbuf2 == NULL) allocache((long *)&lzwbuf2,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[1]);
	if (lzwbuf3 == NULL) allocache((long *)&lzwbuf3,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock[2]);
	if (lzwbuf4 == NULL) allocache((long *)&lzwbuf4,LZWSIZE,&lzwbuflock[3]);
	if (lzwbuf5 == NULL) allocache((long *)&lzwbuf5,LZWSIZE+(LZWSIZE>>4),&lzwbuflock[4]);

	if (dasizeof > LZWSIZE) { count *= dasizeof; dasizeof = 1; }
	ptr = (char *)buffer;

	fread(&leng,2,1,fil); fread(lzwbuf5,(long)leng,1,fil);
	k = 0; kgoal = uncompress(lzwbuf5,(long)leng,lzwbuf4);

	copybufbyte(lzwbuf4,ptr,(long)dasizeof);
	k += (long)dasizeof;

	for(i=1;i<count;i++)
	{
		if (k >= kgoal)
		{
			fread(&leng,2,1,fil); fread(lzwbuf5,(long)leng,1,fil);
			k = 0; kgoal = uncompress(lzwbuf5,(long)leng,lzwbuf4);
		}
		for(j=0;j<dasizeof;j++) ptr[j+dasizeof] = ((ptr[j]+lzwbuf4[j+k])&255);
		k += dasizeof;
		ptr += dasizeof;
	}
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 1;
}

dfwrite(void *buffer, size_t dasizeof, size_t count, FILE *fil)
{
	long i, j, k;
	short leng;
	char *ptr;

	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 200;
	if (lzwbuf1 == NULL) allocache((long *)&lzwbuf1,LZWSIZE+(LZWSIZE>>4),&lzwbuflock);
	if (lzwbuf2 == NULL) allocache((long *)&lzwbuf2,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock);
	if (lzwbuf3 == NULL) allocache((long *)&lzwbuf3,(LZWSIZE+(LZWSIZE>>4))*2,&lzwbuflock);
	if (lzwbuf4 == NULL) allocache((long *)&lzwbuf4,LZWSIZE,&lzwbuflock);
	if (lzwbuf5 == NULL) allocache((long *)&lzwbuf5,LZWSIZE+(LZWSIZE>>4),&lzwbuflock);

	if (dasizeof > LZWSIZE) { count *= dasizeof; dasizeof = 1; }
	ptr = (char *)buffer;

	copybufbyte(ptr,lzwbuf4,(long)dasizeof);
	k = dasizeof;

	if (k > LZWSIZE-dasizeof)
	{
		leng = (short)compress(lzwbuf4,k,lzwbuf5); k = 0;
		fwrite(&leng,2,1,fil); fwrite(lzwbuf5,(long)leng,1,fil);
	}

	for(i=1;i<count;i++)
	{
		for(j=0;j<dasizeof;j++) lzwbuf4[j+k] = ((ptr[j+dasizeof]-ptr[j])&255);
		k += dasizeof;
		if (k > LZWSIZE-dasizeof)
		{
			leng = (short)compress(lzwbuf4,k,lzwbuf5); k = 0;
			fwrite(&leng,2,1,fil); fwrite(lzwbuf5,(long)leng,1,fil);
		}
		ptr += dasizeof;
	}
	if (k > 0)
	{
		leng = (short)compress(lzwbuf4,k,lzwbuf5);
		fwrite(&leng,2,1,fil); fwrite(lzwbuf5,(long)leng,1,fil);
	}
	lzwbuflock[0] = lzwbuflock[1] = lzwbuflock[2] = lzwbuflock[3] = lzwbuflock[4] = 1;
}

compress(char *lzwinbuf, long uncompleng, char *lzwoutbuf)
{
	long i, addr, newaddr, addrcnt, zx, *longptr;
	long bytecnt1, bitcnt, numbits, oneupnumbits;
	short *shortptr;
#if (LIBVER_BUILDREV < 19960427L)
	char bitcnt2 = 0;
#endif

	for(i=255;i>=0;i--) { lzwbuf1[i] = i; lzwbuf3[i] = (i+1)&255; }
	clearbuf(FP_OFF(lzwbuf2),256>>1,0xffffffff);
	clearbuf(FP_OFF(lzwoutbuf),((uncompleng+15)+3)>>2,0L);

#if (LIBVER_BUILDREV < 19960427L)
	addrcnt = 256; bytecnt1 = 0; bitcnt = 4;
	numbits = 9; bitcnt2 = 0; oneupnumbits = (1<<9);
#else
	addrcnt = 256; bytecnt1 = 0; bitcnt = (4<<3);
	numbits = 8; oneupnumbits = (1<<8);
#endif
	do
	{
		addr = lzwinbuf[bytecnt1];
		do
		{
			bytecnt1++;
			if (bytecnt1 == uncompleng) break;
			if (lzwbuf2[addr] < 0) {lzwbuf2[addr] = addrcnt; break;}
			newaddr = lzwbuf2[addr];
			while (lzwbuf1[newaddr] != lzwinbuf[bytecnt1])
			{
				zx = lzwbuf3[newaddr];
				if (zx < 0) {lzwbuf3[newaddr] = addrcnt; break;}
				newaddr = zx;
			}
			if (lzwbuf3[newaddr] == addrcnt) break;
			addr = newaddr;
		} while (addr >= 0);
		lzwbuf1[addrcnt] = lzwinbuf[bytecnt1];
		lzwbuf2[addrcnt] = -1;
		lzwbuf3[addrcnt] = -1;

#if (LIBVER_BUILDREV < 19960427L)
		longptr = (long *)&lzwoutbuf[bitcnt];
		longptr[0] |= (addr<<bitcnt2);
		bitcnt2 += numbits;
		bitcnt += (bitcnt2>>3);
		bitcnt2 &= 7;
#else
		longptr = (long *)&lzwoutbuf[bitcnt>>3];
		longptr[0] |= (addr<<(bitcnt&7));
		bitcnt += numbits;
		if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
			bitcnt--;
#endif

		addrcnt++;
#if (LIBVER_BUILDREV < 19960427L)
		if (addrcnt == oneupnumbits) { numbits++; oneupnumbits <<= 1; }
	} while ((bytecnt1 < uncompleng) && (bitcnt < uncompleng));
#else
		if (addrcnt > oneupnumbits) { numbits++; oneupnumbits <<= 1; }
	} while ((bytecnt1 < uncompleng) && (bitcnt < (uncompleng<<3)));
#endif

#if (LIBVER_BUILDREV < 19960427L)
	longptr = (long *)&lzwoutbuf[bitcnt];
	longptr[0] |= (addr<<bitcnt2);
	bitcnt2 += numbits;
	bitcnt += (bitcnt2>>3);
#else
	longptr = (long *)&lzwoutbuf[bitcnt>>3];
	longptr[0] |= (addr<<(bitcnt&7));
	bitcnt += numbits;
	if ((addr&((oneupnumbits>>1)-1)) > ((addrcnt-1)&((oneupnumbits>>1)-1)))
		bitcnt--;
#endif

	shortptr = (short *)lzwoutbuf;
	shortptr[0] = (short)uncompleng;
#if (LIBVER_BUILDREV < 19960427L)
	if (bitcnt < uncompleng)
	{
		shortptr[1] = (short)addrcnt;
		return(bitcnt);
	}
#else
	if (((bitcnt+7)>>3) < uncompleng)
	{
		shortptr[1] = (short)addrcnt;
		return((bitcnt+7)>>3);
	}
#endif
	shortptr[1] = (short)0;
	for(i=0;i<uncompleng;i++) lzwoutbuf[i+4] = lzwinbuf[i];
	return(uncompleng+4);
}

uncompress(char *lzwinbuf, long compleng, char *lzwoutbuf)
{
	long strtot, currstr, numbits, oneupnumbits;
	long i, dat, leng, bitcnt, outbytecnt, *longptr;
	short *shortptr;
#if (LIBVER_BUILDREV < 19960427L)
	char bitcnt2;
#endif

	shortptr = (short *)lzwinbuf;
	strtot = (long)shortptr[1];
	if (strtot == 0)
	{
		copybuf(FP_OFF(lzwinbuf)+4,FP_OFF(lzwoutbuf),((compleng-4)+3)>>2);
		return((long)shortptr[0]); //uncompleng
	}
	for(i=255;i>=0;i--) { lzwbuf2[i] = i; lzwbuf3[i] = i; }
#if (LIBVER_BUILDREV < 19960427L)
	bitcnt2 = 0; currstr = 256; bitcnt = 4; outbytecnt = 0;
	numbits = 9; oneupnumbits = (1<<9);
#else
	currstr = 256; bitcnt = (4<<3); outbytecnt = 0;
	numbits = 8; oneupnumbits = (1<<8);
#endif
	do
	{
#if (LIBVER_BUILDREV < 19960427L)
		longptr = (long *)&lzwinbuf[bitcnt];
		dat = ((longptr[0]>>bitcnt2) & (oneupnumbits-1));
		bitcnt2 += numbits;
		bitcnt += (bitcnt2>>3);
		bitcnt2 &= 7;
#else
		longptr = (long *)&lzwinbuf[bitcnt>>3];
		dat = ((longptr[0]>>(bitcnt&7)) & (oneupnumbits-1));
		bitcnt += numbits;
		if ((dat&((oneupnumbits>>1)-1)) > ((currstr-1)&((oneupnumbits>>1)-1)))
			{ dat &= ((oneupnumbits>>1)-1); bitcnt--; }
#endif

		lzwbuf3[currstr] = dat;

		for(leng=0;dat>=256;leng++,dat=lzwbuf3[dat])
			lzwbuf1[leng] = lzwbuf2[dat];

		lzwoutbuf[outbytecnt++] = dat;
		for(i=leng-1;i>=0;i--) lzwoutbuf[outbytecnt++] = lzwbuf1[i];

		lzwbuf2[currstr-1] = dat; lzwbuf2[currstr] = dat;
		currstr++;
#if (LIBVER_BUILDREV < 19960427L)
		if (currstr >= oneupnumbits) { numbits++; oneupnumbits <<= 1; }
#else
		if (currstr > oneupnumbits) { numbits++; oneupnumbits <<= 1; }
#endif
	} while (currstr < strtot);
	return((long)shortptr[0]); //uncompleng
}

/*
 * geoip.c
 *
 * Description: This is a modified version of GeoIP API (simplified to use in our case)
 *              - query performance about ~2.6 Sec for 1 million requests p4 2.0 Ghz
 *
 * Based on GeoIP by mcwf/ETPub
 *
 *
 */
#include "g_local.h"

GeoIP *gidb = NULL;

unsigned long GeoIP_addr_to_num(const char *addr)
{
	char          tok[4];
	int           i, octet, j = 0, k = 0;
	unsigned long ipnum = 0;
	char          c     = 0;

	for (i = 0; i < 4; i++)
	{
		for (;; )
		{
			c = addr[k++];
			if (c == '.' || c == ':' || c == '\0')
			{
				tok[j] = '\0';
				octet  = atoi(tok);
				if (octet > 255)
				{
					return 0;
				}
				ipnum += (octet << ((3 - i) * 8));
				j      = 0;
				break;
			}
			else if (c >= '0' && c <= '9')
			{
				if (j > 2)
				{
					return 0;
				}
				tok[j++] = c;
			}
			else
			{
				return 0;
			}
		}
		if (c == '\0' && i < 3)
		{
			return 0;
		}
	}
	return ipnum;
}

unsigned int GeoIP_seek_record(GeoIP *gi, unsigned long ipnum)
{
	int                 depth;
	unsigned int        x    = 0;
	unsigned int        step = 0;
	const unsigned char *buf = NULL;

	for (depth = 31; depth >= 0; depth--)
	{
		step = 6 * x;

		if (step + 6 >= gi->memsize)
		{
			G_Printf("GeoIP: Error Traversing Database for ipnum = %lu - Perhaps database is corrupt?\n", ipnum);
			return 255;
		}

		buf = gi->cache + step;

		if (ipnum & (1 << depth))
		{
			x = (buf[3] << 0) + (buf[4] << 8) + (buf[5] << 16);
		}
		else
		{
			x = (buf[0] << 0) + (buf[1] << 8) + (buf[2] << 16);
		}

		if (x >= 16776960)
		{
			return x - 16776960;
		}
	}

	G_Printf("GeoIP: Error Traversing Database for ipnum = %lu - Perhaps database is corrupt?\n", ipnum);
	return 255;
}

void GeoIP_open(void)
{
	// basically not necessary but to get sure we close a maybe existing database
	GeoIP_close();

	if (!g_countryflags.integer)
	{
		G_Printf("GeoIP is disabled\n");
		return;
	}

	gidb = (GeoIP *)malloc(sizeof(GeoIP));

	if (gidb == NULL)
	{
		G_Printf("GeoIP: Memory allocation error for GeoIP struct\n");
		return;
	}

	gidb->memsize = trap_FS_FOpenFile("GeoIP.dat", &gidb->GeoIPDatabase, FS_READ);

	if ((int)gidb->memsize < 0)
	{
		G_Printf("GeoIP: Error opening database GeoIP.dat\n");
		free(gidb);
		gidb = NULL;
		return;

	}
	else if (gidb->memsize == 0)
	{
		G_Printf("GeoIP: Error zero-sized database file\n");
		trap_FS_FCloseFile(gidb->GeoIPDatabase);
		free(gidb);
		gidb = NULL;
		return;
	}
	else
	{
		gidb->cache = (unsigned char *) calloc(gidb->memsize + 1, sizeof(unsigned char));
		if (gidb->cache != NULL)
		{
			trap_FS_Read(gidb->cache, gidb->memsize, gidb->GeoIPDatabase);
			trap_FS_FCloseFile(gidb->GeoIPDatabase);
			G_Printf("GeoIP is enabled. Database memory size: %.2f kb\n", gidb->memsize / 1024.f);
			return;
		}

		G_Printf("GeoIP: Memory allocation error for GeoIP cache\n");
		trap_FS_FCloseFile(gidb->GeoIPDatabase);
		free(gidb);
		gidb = NULL;
		return;
	}
}

void GeoIP_close(void)
{
	if (gidb != NULL)
	{
		free(gidb->cache);
		gidb->cache = NULL;
		free(gidb);
		gidb = NULL;
	}
}

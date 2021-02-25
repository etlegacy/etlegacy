/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
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
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file g_geoip.c
 * @brief This is a modified version of GeoIP API (simplified to use in our case)
 *        - query performance about ~2.6 Sec for 1 million requests p4 2.0 Ghz
 *
 * @copyright mcwf/ETPub
 */

#include "g_local.h"

GeoIP *gidb = NULL;

const char *country_name[MAX_COUNTRY_NUM] = { "N/A",                                  "Asia/Pacific Region",             "Europe",                         "Andorra",                                      "the United Arab Emirates",                 "Afghanistan",                       "Antigua and Barbuda",          "Anguilla",              "Albania",                                   "Armenia",
	                                          "Curacao",                              "Angola",                          "Antarctica",                     "Argentina",                                    "American Samoa",                           "Austria",                           "Australia",                    "Aruba",                 "Azerbaijan",                                "Bosnia and Herzegovina",
	                                          "Barbados",                             "Bangladesh",                      "Belgium",                        "Burkina Faso",                                 "Bulgaria",                                 "Bahrain",                           "Burundi",                      "Benin",                 "Bermuda",                                   "Brunei Darussalam",
	                                          "Plurinational State of Bolivia",       "Brazil",                          "the Bahamas",                    "Bhutan",                                       "Bouvet Island",                            "Botswana",                          "Belarus",                      "Belize",                "Canada",                                    "the Cocos (Keeling) Islands",
	                                          "the Democratic Republic of the Congo", "the Central African Republic",    "the Congo",                      "Switzerland",                                  "Cote d'Ivoire",                            "the Cook Islands",                  "Chile",                        "Cameroon",              "China",                                     "Colombia",
	                                          "Costa Rica",                           "Cuba",                            "Cabo Verde",                     "Christmas Island",                             "Cyprus",                                   "Czechia",                           "Germany",                      "Djibouti",              "Denmark",                                   "Dominica",
	                                          "the Dominican Republic",               "Algeria",                         "Ecuador",                        "Estonia",                                      "Egypt",                                    "Western Sahara",                    "Eritrea",                      "Spain",                 "Ethiopia",                                  "Finland",
	                                          "Fiji",                                 "the Falkland Islands (Malvinas)", "Federated States of Micronesia", "the Faroe Islands",                            "France",                                   "Sint Maarten (Dutch part)",         "Gabon",                        "the United Kingdom",    "Grenada",                                   "Georgia",
	                                          "French Guiana",                        "Ghana",                           "Gibraltar",                      "Greenland",                                    "the Gambia",                               "Guinea",                            "Guadeloupe",                   "Equatorial Guinea",     "Greece",                                    "South Georgia and the South Sandwich Islands",
	                                          "Guatemala",                            "Guam",                            "Guinea-Bissau",                  "Guyana",                                       "Hong Kong",                                "Heard Island and McDonald Islands", "Honduras",                     "Croatia",               "Haiti",                                     "Hungary",
	                                          "Indonesia",                            "Ireland",                         "Israel",                         "India",                                        "the British Indian Ocean Territory",       "Iraq",                              "Islamic Republic of Iran",     "Iceland",               "Italy",                                     "Jamaica",
	                                          "Jordan",                               "Japan",                           "Kenya",                          "Kyrgyzstan",                                   "Cambodia",                                 "Kiribati",                          "the Comoros",                  "Saint Kitts and Nevis", "the Democratic People's Republic of Korea", "the Republic of Korea",
	                                          "Kuwait",                               "the Cayman Islands",              "Kazakhstan",                     "the Lao People's Democratic Republic",         "Lebanon",                                  "Saint Lucia",                       "Liechtenstein",                "Sri Lanka",             "Liberia",                                   "Lesotho",
	                                          "Lithuania",                            "Luxembourg",                      "Latvia",                         "Libya",                                        "Morocco",                                  "Monaco",                            "the Republic of Moldova",      "Madagascar",            "the Marshall Islands",                      "North Macedonia",
	                                          "Mali",                                 "Myanmar",                         "Mongolia",                       "Macao",                                        "the Northern Mariana Islands",             "Martinique",                        "Mauritania",                   "Montserrat",            "Malta",                                     "Mauritius",
	                                          "Maldives",                             "Malawi",                          "Mexico",                         "Malaysia",                                     "Mozambique",                               "Namibia",                           "New Caledonia",                "the Niger",             "Norfolk Island",                            "Nigeria",
	                                          "Nicaragua",                            "the Netherlands",                 "Norway",                         "Nepal",                                        "Nauru",                                    "Niue",                              "New Zealand",                  "Oman",                  "Panamai",                                   "Peru",
	                                          "French Polynesia",                     "Papua New Guinea",                "the Philippines",                "Pakistan",                                     "Poland",                                   "Saint Pierre and Miquelon",         "Pitcairn",                     "Puerto Rico",           "State of Palestine",                        "Portugal",
	                                          "Palau",                                "Paraguay",                        "Qatar",                          "Reunion",                                      "Romania",                                  "the Russian Federation",            "Rwanda",                       "Saudi Arabia",          "Solomon Islands",                           "Seychelles",
	                                          "the Sudan",                            "Sweden",                          "Singapore",                      "Saint Helena, Ascension and Tristan da Cunha", "Slovenia",                                 "Svalbard and Jan Mayen",            "Slovakia",                     "Sierra Leone",          "San Marino",                                "Senegal",
	                                          "Somalia",                              "Suriname",                        "Sao Tome and Principe",          "El Salvador",                                  "the Syrian Arab Republic",                 "Eswatini",                          "the Turks and Caicos Islands", "Chad",                  "the French Southern Territories",           "Togo",
	                                          "Thailand",                             "Tajikistan",                      "Tokelau",                        "Turkmenistan",                                 "Tunisia",                                  "Tonga",                             "Timor-Leste",                  "Turkey",                "Trinidad and Tobago",                       "Tuvalu",
	                                          "Taiwan (Province of China)",           "the United Republic of Tanzania", "Ukraine",                        "Uganda",                                       "the United States Minor Outlying Islands", "the United States",                 "Uruguay",                      "Uzbekistan",            "the Holy See",                              "Saint Vincent and the Grenadines",
	                                          "Bolivarian Republic of Venezuela",     "British Virgin Islands",          "U.S. Virgin Islands",            "Viet Nam",                                     "Vanuatu",                                  "Wallis and Futuna",                 "Samoa",                        "Yemen",                 "Mayotte",                                   "Serbia",
	                                          "South Africa",                         "Zambia",                          "Montenegro",                     "Zimbabwe",                                     "Anonymous Proxy",                          "Satellite Provider",                "Other",                        "Aland Islands",         "Guernsey",                                  "Isle of Man",
	                                          "Jersey",                               "Saint Barthelemy",                "Saint Martin (French part)",     "Bonaire, Sint Eustatius and Saba",             "South Sudan",                              "Other" };

/**
 * @brief GeoIP_addr_to_num
 * @param[in] addr
 * @return
 */
unsigned long GeoIP_addr_to_num(const char *addr)
{
	char          tok[4];
	int           i, octet, j = 0, k = 0;
	unsigned long ipnum = 0;
	char          c     = 0;

	for (i = 0; i < 4; i++)
	{
		for (;;)
		{
			c = addr[k++];
			if (c == '.' || c == ':' || c == '\0')
			{
				tok[j] = '\0';
				octet  = Q_atoi(tok);
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

/**
 * @brief GeoIP_seek_record
 * @param[in] gi
 * @param[in] ipnum
 * @return
 */
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

/**
 * @brief GeoIP_open
 */
void GeoIP_open(void)
{
	// basically not necessary but to get sure we close a maybe existing database
	GeoIP_close();

	if (!g_countryflags.integer)
	{
		G_Printf("GeoIP is disabled\n");
		return;
	}

	gidb = (GeoIP *)Com_Allocate(sizeof(GeoIP));

	if (gidb == NULL)
	{
		G_Printf("GeoIP: Memory allocation error for GeoIP struct\n");
		return;
	}

	gidb->memsize = trap_FS_FOpenFile("GeoIP.dat", &gidb->GeoIPDatabase, FS_READ);

	if ((int)gidb->memsize < 0)
	{
		G_Printf("GeoIP: Error opening database GeoIP.dat\n");
		Com_Dealloc(gidb);
		gidb = NULL;
		return;

	}
	else if (gidb->memsize == 0)
	{
		G_Printf("GeoIP: Error zero-sized database file\n");
		trap_FS_FCloseFile(gidb->GeoIPDatabase);
		Com_Dealloc(gidb);
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
			G_Printf("GeoIP is enabled. Database memory size: %.2f kb\n", gidb->memsize / 1024.0);
			return;
		}

		G_Printf("GeoIP: Memory allocation error for GeoIP cache\n");
		trap_FS_FCloseFile(gidb->GeoIPDatabase);
		Com_Dealloc(gidb);
		gidb = NULL;
		return;
	}
}

/**
 * @brief GeoIP_close
 */
void GeoIP_close(void)
{
	if (gidb != NULL)
	{
		Com_Dealloc(gidb->cache);
		gidb->cache = NULL;
		Com_Dealloc(gidb);
		gidb = NULL;
	}
}

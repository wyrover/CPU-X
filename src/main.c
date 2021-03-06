/****************************************************************************
*    Copyright © 2014-2017 Xorg
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/

/*
* PROJECT CPU-X
* FILE main.c
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <locale.h>
#include <libintl.h>
#include "cpu-x.h"
#include "label_names.h"

#if HAS_LIBCURL
# include <curl/curl.h>
#endif

#if PORTABLE_BINARY
# include <sys/stat.h>
# include <archive.h>
# include <archive_entry.h>
# if HAS_GETTEXT
#  include "../po/mo.h"
# endif
#endif

char *binary_name, *new_version[2] = { NULL, NULL };
Options *opts;


/************************* Arrays management functions *************************/

/* Set labels name */
static void labels_setname(Labels *data)
{
	int i, j;

	Arrays arrays[] =
	{
		{ data->objects,               NULL,                         objects_names,         LASTOBJ         },
		{ data->tab_cpu[NAME],         data->tab_cpu[VALUE],         tab_cpu_names,         LASTCPU         },
		{ data->tab_caches[NAME],      data->tab_caches[VALUE],      tab_caches_names,      LASTCACHES      },
		{ data->tab_motherboard[NAME], data->tab_motherboard[VALUE], tab_motherboard_names, LASTMOTHERBOARD },
		{ data->tab_memory[NAME],      data->tab_memory[VALUE],      tab_memory_names,      LASTMEMORY      },
		{ data->tab_system[NAME],      data->tab_system[VALUE],      tab_system_names,      LASTSYSTEM      },
		{ data->tab_graphics[NAME],    data->tab_graphics[VALUE],    tab_graphics_names,    LASTGRAPHICS    },
		{ data->tab_bench[NAME],       data->tab_bench[VALUE],       tab_bench_names,       LASTBENCH       },
		{ data->tab_about,             NULL,                         tab_about_names,       LASTABOUT       },
		{ NULL,                        NULL,                         NULL,                  0               }
	};

	MSG_VERBOSE(_("Setting label names"));
	for(i = 0; arrays[i].dim_names != NULL; i++)
	{
		for(j = 0; j < arrays[i].last; j++)
		{
			asprintf(&arrays[i].dim_names[arrays[i].names[j].index], _(arrays[i].names[j].name));
			if(arrays[i].dim_values != NULL)
				casprintf(&arrays[i].dim_values[j], false, "%c", '\0');
		}
	}
}

/* Dump all data in stdout */
static void dump_data(Labels *data)
{
	int i, j, k = 0;
	const char *col_start = opts->color ? BOLD_BLUE : "";
	const char *col_end   = opts->color ? DEFAULT   : "";
	const Arrays arrays[] =
	{
		{ data->tab_cpu[NAME],         data->tab_cpu[VALUE],         NULL, LASTCPU                                 },
		{ data->tab_caches[NAME],      data->tab_caches[VALUE],      NULL, data->cache_count * CACHEFIELDS         },
		{ data->tab_motherboard[NAME], data->tab_motherboard[VALUE], NULL, LASTMOTHERBOARD                         },
		{ data->tab_memory[NAME],      data->tab_memory[VALUE],      NULL, data->dimm_count                        },
		{ data->tab_system[NAME],      data->tab_system[VALUE],      NULL, LASTSYSTEM                              },
		{ data->tab_graphics[NAME],    data->tab_graphics[VALUE],    NULL, data->gpu_count * GPUFIELDS             },
		{ NULL,                        NULL,                         NULL, 0                                       }
	};
	const struct { int tab_nb, lab_nb, frame_nb; } frames[] =
	{
		{ NO_CPU,         VENDOR,       FRAMPROCESSOR       },
		{ NO_CPU,         CORESPEED,    FRAMCLOCKS          },
		{ NO_CPU,         LEVEL1D,      FRAMCACHE           },
		{ NO_CPU,         SOCKETS,      -1                  },
		{ NO_CACHES,      L1SIZE,       FRAML1CACHE         },
		{ NO_CACHES,      L2SIZE,       FRAML2CACHE         },
		{ NO_CACHES,      L3SIZE,       FRAML3CACHE         },
		{ NO_CACHES,      L4SIZE,       FRAML4CACHE         },
		{ NO_MOTHERBOARD, MANUFACTURER, FRAMMOTHERBOARD     },
		{ NO_MOTHERBOARD, BRAND,        FRAMBIOS            },
		{ NO_MOTHERBOARD, CHIPVENDOR,   FRAMCHIPSET         },
		{ NO_MEMORY,      BANK0,        FRAMBANK0           },
		{ NO_MEMORY,      BANK1,        FRAMBANK1           },
		{ NO_MEMORY,      BANK2,        FRAMBANK2           },
		{ NO_MEMORY,      BANK3,        FRAMBANK3           },
		{ NO_MEMORY,      BANK4,        FRAMBANK4           },
		{ NO_MEMORY,      BANK5,        FRAMBANK5           },
		{ NO_MEMORY,      BANK6,        FRAMBANK6           },
		{ NO_MEMORY,      BANK7,        FRAMBANK7           },
		{ NO_SYSTEM,      KERNEL,       FRAMOPERATINGSYSTEM },
		{ NO_SYSTEM,      USED,         FRAMMEMORY          },
		{ NO_GRAPHICS,    GPU1VENDOR,   FRAMGPU1            },
		{ NO_GRAPHICS,    GPU2VENDOR,   FRAMGPU2            },
		{ NO_GRAPHICS,    GPU3VENDOR,   FRAMGPU3            },
		{ NO_GRAPHICS,    GPU4VENDOR,   FRAMGPU4            }
	};

	MSG_VERBOSE(_("Dumping data..."));
	for(i = 0; arrays[i].dim_names != NULL; i++)
	{
		MSG_STDOUT("  %s>>>>>>>>>> %s <<<<<<<<<<%s", col_start, data->objects[i], col_end);
		while(frames[k].tab_nb != i)
			k++;

		for(j = 0; j < arrays[i].last; j++)
		{
			if(frames[k].tab_nb == i && frames[k].lab_nb == j)
			{
				MSG_STDOUT("\n\t%s***** %s *****%s", col_start,
					(frames[k].frame_nb >= 0) ? data->objects[frames[k].frame_nb] : "*", col_end);
				k++;
			}
			MSG_STDOUT("%16s: %s", arrays[i].dim_names[j], arrays[i].dim_values[j]);
		}
		MSG_STDOUT("\n");
	}

	labels_free(data);
}

/* Free memory after display labels */
void labels_free(Labels *data)
{
	int i, j;
	Arrays arrays[] =
	{
		{ data->objects,               NULL,                         NULL, LASTOBJ                         },
		{ data->tab_cpu[NAME],         data->tab_cpu[VALUE],         NULL, LASTCPU                         },
		{ data->tab_caches[NAME],      data->tab_caches[VALUE],      NULL, data->cache_count * CACHEFIELDS },
		{ data->w_data->test_name,     NULL,                         NULL, data->w_data->test_count        },
		{ data->tab_motherboard[NAME], data->tab_motherboard[VALUE], NULL, LASTMOTHERBOARD                 },
		{ data->tab_memory[NAME],      data->tab_memory[VALUE],      NULL, data->dimm_count                },
		{ data->tab_system[NAME],      data->tab_system[VALUE],      NULL, LASTSYSTEM                      },
		{ data->tab_graphics[NAME],    data->tab_graphics[VALUE],    NULL, data->gpu_count * GPUFIELDS     },
		{ data->tab_bench[NAME],       data->tab_bench[VALUE],       NULL, LASTBENCH                       },
		{ data->tab_about,             NULL,                         NULL, LASTABOUT                       },
		{ NULL,                        NULL,                         NULL, 0                               }
	};

	MSG_VERBOSE(_("Freeing memory"));
	for(i = 0; arrays[i].dim_names != NULL; i++)
	{
		for(j = 0; j < arrays[i].last; j++)
		{
			free(arrays[i].dim_names[j]);
			arrays[i].dim_names[j] = NULL;
			if(arrays[i].dim_values != NULL)
			{
				free(arrays[i].dim_values[j]);
				arrays[i].dim_values[j] = NULL;
			}
		}
	}
}


/************************* Update-related functions *************************/

#if HAS_LIBCURL
/* Write function for Curl */
static size_t writefunc(void *ptr, size_t size, size_t nmemb, void **stream)
{
	char **buff = (char **) stream;

	asprintf(buff, ptr);
	(*buff)[nmemb - 1] = '\0';

	return size * nmemb;
}

/* Check if running version is latest */
static bool check_new_version(void)
{
	CURL *curl;
	CURLcode code;

	if(!opts->use_network)
	{
		asprintf(&new_version[1], "%c", '\0');
		return false;
	}

	MSG_VERBOSE(_("Checking on Internet for a new version..."));
	curl = curl_easy_init();
	if(!curl)
	{
		MSG_ERROR(_("failed to open a Curl session"));
		return 1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, UPDURL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &new_version[0]);
	code = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if((code != CURLE_OK) || (new_version[0] == NULL))
	{
		MSG_ERROR(_("failed to perform the Curl transfer (%s)"),
			(new_version[0] != NULL) ? curl_easy_strerror(code) : _("wrong write data"));
		opts->use_network = false;
		asprintf(&new_version[1], "%c", '\0');
	}
	else if(strcmp(new_version[0], PRGVER))
	{
		MSG_VERBOSE(_("A new version of %s is available!"), PRGNAME);
		asprintf(&new_version[1], _("(version %s is available)"), new_version[0]);
		return true;
	}
	else
	{
		MSG_VERBOSE(_("No new version available"));
		asprintf(&new_version[1], _("(up-to-date)"));
	}

	free(new_version[0]);
	new_version[0] = NULL;
	return false;
}

# if PORTABLE_BINARY
/* Copy function for libarchive */
static int copy_data(struct archive *ar, struct archive *aw)
{
	int ret;
	const void *buff;
	size_t size;
	off_t offset;

	while(true)
	{
		ret = archive_read_data_block(ar, &buff, &size, &offset);
		if(ret == ARCHIVE_EOF)
			return ARCHIVE_OK;
		if(ret < ARCHIVE_OK)
			return ret;

		if((ret = archive_write_data_block(aw, buff, size, offset)) < ARCHIVE_OK)
			return ret;
	}
}

/* Extract a .tar.gz archive */
static int extract_archive(const char *filename, const char *needed)
{
	int ret;
	const int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM;
	struct archive *archive, *ext;
	struct archive_entry *entry;

	archive = archive_read_new();
	archive_read_support_format_tar(archive);
	archive_read_support_filter_gzip(archive);

	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);

	if((ret = archive_read_open_filename(archive, filename, 10240)))
		goto error_archive;

	do {
		if((ret = archive_read_next_header(archive, &entry)) != ARCHIVE_OK)
			goto error_archive;
	} while(strcmp(archive_entry_pathname(entry), needed));

	if((ret = archive_write_header(ext, entry)) != ARCHIVE_OK)
		goto error_ext;
	if((ret = copy_data(archive, ext)) != ARCHIVE_OK)
		goto error_ext;
	if((ret = archive_write_finish_entry(ext)) != ARCHIVE_OK)
		goto error_ext;

	archive_read_close(archive);
	archive_read_free(archive);
	archive_write_close(ext);
	archive_write_free(ext);
	return 0;

error_archive:
	MSG_ERROR(_("an error occurred while extracting %s archive (%s)"), filename, archive_error_string(archive));
	return 1;

error_ext:
	MSG_ERROR(_("an error occurred while extracting %s archive (%s)"), filename, archive_error_string(ext));
	return 1;
}

/* Apply new portable version if available */
static int update_prg(void)
{
	int err;
	char *archive = NULL, *new_binary = NULL;
	CURL *curl;
	CURLcode code;
	FILE *file_descr = NULL;

	if(!opts->use_network)
	{
		MSG_WARNING(_("Network access is disabled by environment variable"
		              " (set CPUX_NETWORK with a positive value to enable it)"));
		return 1;
	}

	if(new_version[0] == NULL)
	{
		MSG_WARNING(_("No new version available"));
		return 2;
	}

	curl = curl_easy_init();
	if(!curl)
	{
		MSG_ERROR(_("failed to open a Curl session"));
		return 3;
	}

	asprintf(&archive, "%s_v%s_portable%s.tar.gz", PRGNAME, new_version[0], HAS_GTK ? "" : "_noGTK");
	file_descr = fopen(archive, "wb");
	if(file_descr == NULL)
	{
		MSG_ERRNO(_("failed to open %s archive for writing"), archive);
		free(archive);
		return 4;
	}

	/* Download archive */
	curl_easy_setopt(curl, CURLOPT_URL, format("%s/v%s/%s", TARBALL, new_version[0], archive));
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2 * 60L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file_descr);
	code = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(file_descr);
	if(code != CURLE_OK)
	{
		MSG_ERROR(_("failed to download %s archive (%s)"), archive, curl_easy_strerror(code));
		free(archive);
		return 5;
	}

	/* Extract archive */
	MSG_VERBOSE(_("Extracting new version..."));
	asprintf(&new_binary, "%s_v%s_portable%s.%s", PRGNAME, new_version[0], HAS_GTK ? "" : "_noGTK", OS);
	err = extract_archive(archive, new_binary);
	if(err)
	{
		remove(archive);
		return err;
	}

	/* Rename new binary */
	MSG_VERBOSE(_("Applying new version..."));
	if(strstr(binary_name, PRGVER) != NULL) // If binary name contains version
	{
		err  = remove(binary_name); // Delete old version and keep new version
		err += rename(new_binary, format("%s_v%s", PRGNAME, new_version[0]));
	}
	else
		err = rename(new_binary, binary_name); // Erase old version by new version

	err += remove(archive);
	if(err)
		MSG_ERROR(_("an error occurred while removing/renaming files"));
	else
		MSG_VERBOSE(_("Update successful!"));

	return err;
}
# endif /* PORTABLE_BINARY */
#endif /* HAS_LIBCURL */


/************************* Options-related functions *************************/

static const struct
{
	const bool has_mod;
	const char short_opt;
	const char *long_opt;
	const int  need_arg;
	char       *description;
} cpux_options[] =
{
	{ HAS_GTK,         'g', "gtk",       no_argument,       N_("Start graphical user interface (GUI) (default)")           },
	{ HAS_NCURSES,     'n', "ncurses",   no_argument,       N_("Start text-based user interface (TUI)")                    },
	{ true,            'd', "dump",      no_argument,       N_("Dump all data on standard output and exit")                },
	{ HAS_DMIDECODE,   'D', "dmidecode", no_argument,       N_("Run embedded command dmidecode and exit")                  },
	{ HAS_BANDWIDTH,   'B', "bandwidth", no_argument,       N_("Run embedded command bandwidth and exit")                  },
	{ true,            'a', "tab",       required_argument, N_("Set default tab (integer)")                                },
	{ HAS_LIBCPUID,    'c', "core",      required_argument, N_("Select CPU core to monitor (integer)")                     },
	{ HAS_BANDWIDTH,   't', "cachetest", required_argument, N_("Set custom bandwidth test for CPU caches speed (integer)") },
	{ true,            'r', "refresh",   required_argument, N_("Set custom time between two refreshes (in seconds)")       },
	{ true,            'o', "nocolor",   no_argument,       N_("Disable colored output")                                   },
	{ true,            'i', "issue-fmt", no_argument,       N_("Print required informations to paste in an issue")         },
	{ true,            'v', "verbose",   no_argument,       N_("Verbose output")                                           },
	{ PORTABLE_BINARY, 'u', "update",    no_argument,       N_("Update portable version if a new version is available")    },
	{ true,            'h', "help",      no_argument,       N_("Print help and exit")                                      },
	{ true,            'V', "version",   no_argument,       N_("Print version and exit")                                   },
	{ true,            '0', NULL,        0,                 NULL                                                           }
};

static const struct
{
	const bool has_mod;
	const char *var_name;
	char       *description;
} cpux_env_vars[] =
{
	{ HAS_LIBCURL,  "CPUX_NETWORK",   N_("Temporarily disable network support")   },
	{ true,         "CPUX_BCLK",      N_("Enforce the bus clock") },
	{ HAS_LIBCPUID, "CPUX_CPUID_RAW", N_("Read CPUID raw data from a given file") },
	{ true,         NULL,             NULL                                        }
};

/* This is help display with --help option */
static void help(void)
{
	int i;

	MSG_STDOUT(_("Usage: %s DISPLAY [OPTIONS]\n"), binary_name);
	MSG_STDOUT(_("Available DISPLAY:"));
	for(i = 0; cpux_options[i].long_opt != NULL; i++)
	{
		if(cpux_options[i].short_opt == 'a')
			MSG_STDOUT(_("\nAvailable OPTIONS:"));
		if(cpux_options[i].has_mod)
			MSG_STDOUT("  -%c, --%-10s %s", cpux_options[i].short_opt, cpux_options[i].long_opt, _(cpux_options[i].description));
	}

	MSG_STDOUT(_("\nInfluenceable environment variables:"));
	for(i = 0; cpux_env_vars[i].var_name != NULL; i++)
	{
		if(cpux_env_vars[i].has_mod)
			MSG_STDOUT("  %-16s %s", cpux_env_vars[i].var_name, _(cpux_env_vars[i].description));
	}
}

/* This is the --version option */
static void version(void)
{
	int i;
	const struct { const bool has_mod; const char *lib, *version; } libs_ver[] =
	{
		{ HAS_GTK,         "GTK",         GTK_VERSION         },
		{ HAS_NCURSES,     "NCURSES",     NCURSES_VERSION     },
		{ HAS_LIBCURL,     "LIBCURL",     LIBCURL_VERSION     },
		{ HAS_LIBCPUID,    "LIBCPUID",    LIBCPUID_VERSION    },
		{ HAS_LIBPCI,      "LIBPCI",      LIBPCI_VERSION      },
		{ HAS_LIBPROCPS,   "LIBPROCPS",   LIBPROCPS_VERSION   },
		{ HAS_LIBSTATGRAB, "LIBSTATGRAB", LIBSTATGRAB_VERSION },
		{ HAS_DMIDECODE,   "DMIDECODE",   DMIDECODE_VERSION   },
		{ HAS_BANDWIDTH,   "BANDWIDTH",   BANDWIDTH_VERSION   },
		{ false,           NULL,          NULL                }
	};

	if(HAS_LIBCURL)
		check_new_version();

	MSG_STDOUT("%s %s %s", PRGNAME, PRGVER, new_version[1]);
	MSG_STDOUT("%s\n", PRGCPRGHT);
	MSG_STDOUT(_("This is free software: you are free to change and redistribute it."));
	MSG_STDOUT(_("This program comes with ABSOLUTELY NO WARRANTY"));
	MSG_STDOUT(_("See the %s license: <%s>\n"), PRGLCNS, LCNSURL);
	MSG_STDOUT(_("Built on %s, %s (with %s %s on %s)."), __DATE__, __TIME__, CC, __VERSION__, OS);
	free(new_version[1]);

	/* Print features version */
	for(i = 0; libs_ver[i].lib != NULL; i++)
	{
		if(libs_ver[i].has_mod)
			MSG_STDOUT(_("-- %-9s version: %s"), libs_ver[i].lib, libs_ver[i].version);
	}
}

/* Parse arguments and set some flags */
#define OPTIONS_COUNT (sizeof(cpux_options) / sizeof(cpux_options[0]) - 1)
static void parse_arguments(int argc, char *argv[])
{
	int i, j = 0, c, tmp_arg = -1;
	char shortopts[OPTIONS_COUNT * 2] = "";
	struct option longopts[OPTIONS_COUNT];

	/* Filling longopts structure */
	for(i = 0; cpux_options[i].long_opt != NULL; i++)
	{
		while(!cpux_options[i].has_mod)
			i++;
		longopts[j++] = (struct option) { .name = cpux_options[i].long_opt, .has_arg = cpux_options[i].need_arg, .flag = 0, .val = cpux_options[i].short_opt };
		strcat(shortopts, format("%c%s", cpux_options[i].short_opt, cpux_options[i].need_arg ? ":" : ""));
	}

	/* Set the default mode */
	if(HAS_GTK && (getenv("DISPLAY") != NULL || getenv("WAYLAND_DISPLAY") != NULL))
		opts->output_type = OUT_GTK;
	else if(HAS_NCURSES)
		opts->output_type = OUT_NCURSES;
	else
		opts->output_type = OUT_DUMP;

	/* Parse options */
	while((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
	{
		switch(c)
		{
			case 'g':
				opts->output_type = OUT_GTK;
				break;
			case 'n':
				opts->output_type = OUT_NCURSES;
				break;
			case 'd':
				opts->output_type = OUT_DUMP;
				break;
			case 'r':
				tmp_arg = atoi(optarg);
				if(tmp_arg >= 1)
					opts->refr_time = tmp_arg;
				break;
			case 'a':
				tmp_arg = atoi(optarg);
				if(NO_CPU < tmp_arg && tmp_arg <= NO_ABOUT)
					opts->selected_page = tmp_arg;
				break;
			case 'c':
				tmp_arg = atoi(optarg);
				if(tmp_arg >= 0)
					opts->selected_core = tmp_arg;
				break;
			case 't':
				tmp_arg = atoi(optarg);
				if(tmp_arg >= 0)
					opts->bw_test = atoi(optarg);
				break;
			case 'D':
				opts->output_type = OUT_DMIDECODE;
				break;
			case 'B':
				opts->output_type = OUT_BANDWIDTH;
				break;
			case 'o':
				opts->color = false;
				break;
			case 'i':
				opts->color       = false;
				opts->verbose     = true;
				opts->issue       = true;
				opts->use_network = 0;
				opts->output_type = OUT_DUMP;
				setlocale(LC_ALL, "C");
				version();
				break;
			case 'v':
				opts->verbose = true;
				break;
			case 'u':
				opts->update = true;
				break;
			case 'h':
				help();
				exit(EXIT_SUCCESS);
			case 'V':
				version();
				exit(EXIT_SUCCESS);
			case '?':
			default:
				help();
				exit(EXIT_FAILURE);
		}
	}
}

/* Check for influenceable environment variables */
static void check_environment_variables(Labels *data)
{
	if(getenv("CPUX_NETWORK"))
		opts->use_network = ((atoi(getenv("CPUX_NETWORK"))) > 0);
	if(getenv("CPUX_BCLK"))
		data->bus_freq = atof(getenv("CPUX_BCLK"));
	if(getenv("CPUX_CPUID_RAW"))
		data->l_data->cpuid_raw_file = getenv("CPUX_CPUID_RAW");
}


/************************* Main-related functions *************************/

/* Action on SIGSEV/SIGFPE */
static void sighandler(int signum)
{
	int bt_size, i;
	char **bt_syms, *buff = NULL;
	void *bt[16];

	/* Get the backtrace */
	bt_size = backtrace(bt, 16);
	bt_syms = backtrace_symbols(bt, bt_size);

	/* Print the backtrace */
	MSG_STDERR(_("\n%sOops, something was wrong! %s has received signal %d (%s) and has crashed.%s"),
	           BOLD_RED, PRGNAME, signum, strsignal(signum), DEFAULT);
	MSG_STDERR("========================= Backtrace =========================");
	MSG_STDERR("%s %s (%s, %s)", PRGNAME, PRGVER, CC, OS);
	for(i = 1; i < bt_size; i++)
	{
		popen_to_str(&buff, "addr2line %s -e /proc/%d/exe", strtok(strrchr(bt_syms[i], '[') + 1, "]"), getpid());
		if(strstr(buff, "??") == NULL)
			MSG_STDERR("#%2i %s %s", i, strrchr(buff, '/') + 1, bt_syms[i]);
		else
			MSG_STDERR("#%2i %s", i, bt_syms[i]);
		free(buff);
	}
	MSG_STDERR("======================== End Backtrace =======================\n");
	MSG_STDERR(_("You can paste this backtrace by opening a new issue here:"));
	MSG_STDERR("https://github.com/X0rg/CPU-X/issues/new\n");

	/* Stop program */
	free(bt_syms);
	signal(signum, SIG_DFL);
	kill(getpid(), signum);
}

 /* Enable internationalization support */
static int set_locales(void)
{
	int err;
#if PORTABLE_BINARY && HAS_GETTEXT
	int i;
	FILE *mofile = NULL;

	/* Write .mo files in temporary directory */
	err = mkdir(LOCALEDIR, ACCESSPERMS);
	for(i = 0; ptrlen[i] != NULL; i++)
	{
		err   += mkdir(format("%s/%s/",             LOCALEDIR, lang[i]), ACCESSPERMS);
		err   += mkdir(format("%s/%s/LC_MESSAGES/", LOCALEDIR, lang[i]), ACCESSPERMS);
		mofile = fopen(format("%s/%s/LC_MESSAGES/%s.mo", LOCALEDIR, lang[i], GETTEXT_PACKAGE), "w");

		if(mofile != NULL)
		{
			err += fwrite(ptrlang[i], sizeof(unsigned char), *(ptrlen)[i], mofile) > 0 ? 0 : 1;
			err += fclose(mofile);
		}
		else
			err++;
	}

	if(err)
		MSG_ERROR("an error occurred while extracting translations");
#endif /* PORTABLE_BINARY && HAS_GETTEXT */

	/* Apply locale */
	setlocale(LC_ALL, "");
	err  = bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR)        ? 0 : 1;
	err += bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8") ? 0 : 1;
	err += textdomain(GETTEXT_PACKAGE)                       ? 0 : 1;

	/* Check if something is wrong */
	if(err)
	{
		MSG_ERROR(_("an error occurred while setting locale"));
		return 1;
	}
	else
		return 0;
}

int main(int argc, char *argv[])
{
	/* Init variables */
	binary_name  = argv[0];
	Labels *data = &(Labels)
	{
		.tab_cpu         = { { NULL } },
		.tab_caches      = { { NULL } },
		.tab_motherboard = { { NULL } },
		.tab_memory      = { { NULL } },
		.tab_system      = { { NULL } },
		.tab_graphics    = { { NULL } },
		.tab_bench       = { { NULL } },
		.cpu_freq        = 0,
		.cpu_count       = 0,
		.cache_count     = 0,
		.dimm_count      = 0,
		.gpu_count       = 0,
		.bus_freq        = 0.0,
		.cpu_min_mult    = 0.0,
		.cpu_max_mult    = 0.0
	};
	data->l_data = &(LibcpuidData)
	{
		.cpu_vendor_id  = -1,
		.cpu_model      = -1,
		.cpu_ext_model  = -1,
		.cpu_ext_family = -1,
		.cpuid_raw_file = NULL
	};
	data->w_data = &(BandwidthData)
	{
		.test_count = 0,
		.size       = { 0 },
		.speed      = { 0 },
		.test_name  = NULL
	};
	data->m_data = &(MemoryData)
	{
		.mem_usage  = { 0 },
		.mem_total  = 0,
		.swap_total = 0
	};
	data->b_data = &(BenchData)
	{
		.run      = false,
		.duration = 1,
		.threads  = 1,
		.primes   = 0,
		.start    = 0,
		.elapsed  = 0,
		.num      = 0
	};
	opts = &(Options)
	{
		.color         = true,
		.verbose       = false,
		.issue         = false,
		.use_network   = true,
		.update        = false,
		.selected_page = 0,
		.selected_core = 0,
		.bw_test       = 0,
		.refr_time     = 1
	};

	set_locales();
	signal(SIGSEGV, sighandler);
	signal(SIGFPE,  sighandler);

	/* Parse options */
	parse_arguments(argc, argv);
	check_environment_variables(data);
	if(opts->output_type > OUT_NO_CPUX)
		goto skip_init;

	/* Retrieve data */
	if(getuid())
	{
		MSG_WARNING(_("Root privileges are required to work properly"));
		MSG_WARNING(_("Some informations will not be retrievable"));
	}
	labels_setname(data);
	fill_labels   (data);

	if(HAS_LIBCURL)
		check_new_version();

	/* Show data */
	if(HAS_GTK && (opts->output_type == OUT_GTK))
		start_gui_gtk(&argc, &argv, data);
	if(HAS_NCURSES && (opts->output_type == OUT_NCURSES))
		start_tui_ncurses(data);
	if(opts->output_type == OUT_DUMP)
		dump_data(data);
skip_init:
	if(HAS_DMIDECODE && (opts->output_type == OUT_DMIDECODE))
		return run_dmidecode();
	if(HAS_BANDWIDTH && (opts->output_type == OUT_BANDWIDTH))
		return run_bandwidth();

#if PORTABLE_BINARY
	if(PORTABLE_BINARY && HAS_LIBCURL && opts->update)
		update_prg();
#endif /* PORTABLE_BINARY */

	return EXIT_SUCCESS;
}

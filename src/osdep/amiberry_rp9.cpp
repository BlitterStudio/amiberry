#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "disk.h"
#include "fsdb.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "filesys.h"
#include "autoconf.h"
#include "zfile.h"
#include "archivers/zip/unzip.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <vector>


#define RP9_MANIFEST _T("rp9-manifest.xml")
#define MAX_MANIFEST_ENTRY 256

static char rp9tmp_path[MAX_DPATH];
static std::vector<std::string> lstTmpRP9Files;
static int add_HDF_DHnum = 0;
static bool clip_no_hires = false;


void rp9_init()
{
	fetch_rp9path(rp9tmp_path, MAX_DPATH);
	strncat(rp9tmp_path, _T("tmp/"), MAX_DPATH);
	lstTmpRP9Files.clear();
	LIBXML_TEST_VERSION
}


static void del_tmpFiles()
{
	for (auto & lstTmpRP9File : lstTmpRP9Files)
		remove(lstTmpRP9File.c_str());
	lstTmpRP9Files.clear();
}


void rp9_cleanup()
{
	del_tmpFiles();
	xmlCleanupParser();
	xmlMemoryDump();
}


static xmlNode* get_node(xmlNode* node, const char* name)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), name) == 0)
			return curr_node->children;
	}
	return nullptr;
}


static bool get_value(xmlNode* node, const char* key, char* value, int max_size)
{
	auto result = false;

	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), key) == 0)
		{
			const auto content = xmlNodeGetContent(curr_node);
			if (content != nullptr)
			{
				strncpy(value, reinterpret_cast<char *>(content), max_size);
				xmlFree(content);
				result = true;
			}
			break;
		}
	}

	return result;
}


static void set_default_system(struct uae_prefs* p, const char* system, int rom)
{
	default_prefs(p, true, 0);
	del_tmpFiles();

	if (strcmp(system, "a-500") == 0)
		bip_a500(p, rom);
	else if (strcmp(system, "a-500plus") == 0)
		bip_a500plus(p, rom);
	else if (strcmp(system, "a-1200") == 0)
		bip_a1200(p, rom);
	else if (strcmp(system, "a-2000") == 0)
		bip_a2000(p, rom);
	else if (strcmp(system, "a-4000") == 0)
	{
		bip_a4000(p, rom);
	}
}


static void parse_compatibility(struct uae_prefs* p, xmlNode* node)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), _T("compatibility")) == 0)
		{
			const auto content = xmlNodeGetContent(curr_node);
			if (content != nullptr)
			{
				if (strcmp(reinterpret_cast<const char *>(content), "flexible-blitter-immediate") == 0)
					p->immediate_blits = true;
				else if (strcmp(reinterpret_cast<const char *>(content), "turbo-floppy") == 0)
					p->floppy_speed = 400;
				else if (strcmp(reinterpret_cast<const char *>(content), "flexible-sprite-collisions-spritesplayfield") == 0)
					p->collision_level = 2;
				else if (strcmp(reinterpret_cast<const char *>(content), "flexible-sprite-collisions-spritesonly") == 0)
					p->collision_level = 1;
				else if (strcmp(reinterpret_cast<const char *>(content), "flexible-sound") == 0)
					p->produce_sound = 2;
				else if (strcmp(reinterpret_cast<const char *>(content), "flexible-maxhorizontal-nohires") == 0)
					clip_no_hires = true;
				else if (strcmp(reinterpret_cast<const char *>(content), "jit") == 0)
				{
					p->cachesize = MAX_JIT_CACHE;
					p->address_space_24 = false;
          p->compfpu = true;
				}
				else if (strcmp(reinterpret_cast<const char *>(content), "flexible-cpu-cycles") == 0)
					p->cpu_compatible = false;
				else if (strcmp(reinterpret_cast<const char *>(content), "flexible-maxhorizontal-nosuperhires") == 0); /* nothing to change */
				else if (strcmp(reinterpret_cast<const char *>(content), "flexible-maxvertical-nointerlace") == 0); /* nothing to change */
				else
					printf("rp9: unknown compatibility: %p\n", content);
				xmlFree(content);
			}
		}
	}
}


static void parse_ram(struct uae_prefs* p, xmlNode* node)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), _T("ram")) == 0)
		{
			const auto content = xmlNodeGetContent(curr_node);
			if (content != nullptr)
			{
				const auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("type"));
				if (attr != nullptr)
				{
					auto size = atoi(reinterpret_cast<const char *>(content));
					if (strcmp(reinterpret_cast<const char *>(attr), "fast") == 0)
						p->fastmem[0].size = size;
					else if (strcmp(reinterpret_cast<const char *>(attr), "z3") == 0)
						p->z3fastmem[0].size = size;
					else if (strcmp(reinterpret_cast<const char *>(attr), "chip") == 0)
						p->chipmem_size = size;
					xmlFree(attr);
				}

				xmlFree(content);
			}
		}
	}
}


static void parse_clip(struct uae_prefs* p, xmlNode* node)
{
	auto left = 0, top = 0, width = 320, height = 240;

	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), _T("clip")) == 0)
		{
			auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("left"));
			if (attr != nullptr)
			{
				left = atoi(reinterpret_cast<const char *>(attr));
				xmlFree(attr);
			}
			attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("top"));
			if (attr != nullptr)
			{
				top = atoi(reinterpret_cast<const char *>(attr)) / 2;
				p->vertical_offset = top - 41 + OFFSET_Y_ADJUST;
				xmlFree(attr);
			}
			attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("width"));
			if (attr != nullptr)
			{
				width = atoi(reinterpret_cast<const char *>(attr));
				if (p->chipset_mask & CSMASK_AGA && !clip_no_hires)
					width = width / 2; // Use Hires in AGA mode
				else
					width = width / 4; // Use Lores in OCS/ECS
				if (width <= 320)
					p->gfx_size.width = 320;
				else if (width <= 352)
					p->gfx_size.width = 352;
				else if (width <= 384)
					p->gfx_size.width = 384;
				else if (width <= 640)
					p->gfx_size.width = 640;
				else if (width <= 704)
					p->gfx_size.width = 704;
				else
					p->gfx_size.width = 768;
				xmlFree(attr);
			}
			attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("height"));
			if (attr != nullptr)
			{
				height = atoi(reinterpret_cast<const char *>(attr)) / 2;
				if (height <= 200)
					p->gfx_size.height = 200;
				else if (height <= 216)
					p->gfx_size.height = 216;
				else if (height <= 240)
					p->gfx_size.height = 240;
				else if (height <= 256)
					p->gfx_size.height = 256;
				else if (height <= 262)
					p->gfx_size.height = 262;
				else
					p->gfx_size.height = 270;
				xmlFree(attr);
			}
			break;
		}
	}
}


static void parse_peripheral(struct uae_prefs* p, xmlNode* node)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), _T("peripheral")) == 0)
		{
			const auto content = xmlNodeGetContent(curr_node);
			if (content != nullptr)
			{
				if (strcmp(reinterpret_cast<const char *>(content), "floppy") == 0)
				{
					int type = DRV_35_DD;
					auto unit = -1;

					auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("type"));
					if (attr != nullptr)
					{
						if (strcmp(reinterpret_cast<const char *>(attr), "dd") == 0)
							type = DRV_35_DD;
						else
							type = DRV_35_HD;
						xmlFree(attr);
					}

					attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("unit"));
					if (attr != nullptr)
					{
						unit = atoi(reinterpret_cast<const char *>(attr));
						xmlFree(attr);
					}

					if (unit >= 0)
					{
						if (unit + 1 > p->nr_floppies)
							p->nr_floppies = unit + 1;
						p->floppyslots[unit].dfxtype = type;
					}
				}
				else if (strcmp(reinterpret_cast<const char *>(content), "a-501") == 0)
					p->bogomem_size = 0x00080000;
				else if (strcmp(reinterpret_cast<const char *>(content), "cpu") == 0)
				{
					auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("type"));
					if (attr != nullptr)
					{
						p->cpu_model = atoi(reinterpret_cast<const char *>(attr));
						if (p->cpu_model > 68020)
							p->address_space_24 = false;
						if (p->cpu_model == 68040)
							p->fpu_model = 68040;
						xmlFree(attr);
					}
					attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("speed"));
					if (attr != nullptr)
					{
						if (strcmp(reinterpret_cast<const char *>(attr), "max") == 0)
							p->m68k_speed = -1;
						xmlFree(attr);
					}
				}
				else if (strcmp(reinterpret_cast<const char *>(content), "fpu") == 0)
				{
					const auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("type"));
					if (attr != nullptr)
					{
						if (strcmp(reinterpret_cast<const char *>(attr), "68881") == 0)
							p->fpu_model = 68881;
						else if (strcmp(reinterpret_cast<const char *>(attr), "68882") == 0)
							p->fpu_model = 68882;
						xmlFree(attr);
					}
				}
				else if (strcmp(reinterpret_cast<const char *>(content), "jit") == 0)
				{
					auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("memory"));
					if (attr != nullptr)
					{
						p->cachesize = atoi(reinterpret_cast<const char *>(attr)) / 1024;
						xmlFree(attr);
					}
					attr = xmlGetProp(curr_node, (const xmlChar *)_T("fpu"));
					if (attr != NULL)
					{
						if (strcmp((const char *)attr, "false") == 0)
							p->compfpu = false;
					}
				}
				xmlFree(content);
			}
		}
	}
}


static void parse_boot(struct uae_prefs* p, xmlNode* node)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), _T("boot")) == 0)
		{
			const auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("type"));
			if (attr != nullptr)
			{
				if (strcmp(reinterpret_cast<const char *>(attr), "hdf") == 0)
				{
					// Built-in hdf required
					const auto content = xmlNodeGetContent(curr_node);
					if (content != nullptr)
					{
						char target_file[MAX_DPATH];
						fetch_rp9path(target_file, MAX_DPATH);
						strncat(target_file, "workbench-", MAX_DPATH - 1);
						strncat(target_file, reinterpret_cast<const char *>(content), MAX_DPATH - 1);
						strncat(target_file, ".hdf", MAX_DPATH - 1);
						const auto f = fopen(target_file, "rbe");
						if (f != nullptr)
						{
							struct uaedev_config_info ci{};

							fclose(f);

							if (hardfile_testrdb(target_file))
							{
								ci.physical_geometry = true;
								uci_set_defaults(&ci, true);
							}
							else
							{
								ci.physical_geometry = false;
								uci_set_defaults(&ci, false);
							}

							ci.type = UAEDEV_HDF;
							sprintf(ci.devname, "DH%d", add_HDF_DHnum);
							++add_HDF_DHnum;
							strncpy(ci.rootdir, target_file, MAX_DPATH);

							const auto ro = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("readonly"));
							if (ro != nullptr)
							{
								if (strcmp(reinterpret_cast<const char *>(ro), "true") == 0)
									ci.readonly = true;
								xmlFree(ro);
							}
							ci.bootpri = 127;

							const auto uci = add_filesys_config(p, -1, &ci);
							if (uci)
							{
								const auto hfd = get_hardfile_data(uci->configoffset);
								hardfile_media_change(hfd, &ci, true, false);
							}
						}
						xmlFree(content);
					}
				}
				xmlFree(attr);
			}
		}
	}
}


static void extract_media(struct uae_prefs* p, unzFile uz, xmlNode* node)
{
	const auto tmp = get_node(node, "media");
	if (tmp != nullptr)
	{
		for (auto curr_node = tmp; curr_node; curr_node = curr_node->next)
		{
			auto mediatype = -1;
			if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), _T("floppy")) == 0)
				mediatype = 0;
			else if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), _T("harddrive")) == 0)
				mediatype = 1;
			if (mediatype >= 0)
			{
				auto content = xmlNodeGetContent(curr_node);
				if (content != nullptr)
				{
					auto priority = 0;
					const auto attr = xmlGetProp(curr_node, reinterpret_cast<const xmlChar *>("priority"));
					if (attr != nullptr)
					{
						priority = atoi(reinterpret_cast<const char *>(attr));
						xmlFree(attr);
					}

					if (unzLocateFile(uz, reinterpret_cast<char *>(content), 1) == UNZ_OK)
					{
						unz_file_info file_info;
						if (unzGetCurrentFileInfo(uz, &file_info, nullptr, 0, nullptr, 0, nullptr, 0) == UNZ_OK)
						{
							void* buffer = malloc(file_info.uncompressed_size);
							if (buffer != nullptr)
							{
								if (unzOpenCurrentFile(uz) == UNZ_OK)
								{
									auto readsize = unzReadCurrentFile(uz, buffer, file_info.uncompressed_size);
									unzCloseCurrentFile(uz);
									if (readsize == file_info.uncompressed_size)
									{
										char target_file[MAX_DPATH];
										if (!my_existsdir(rp9tmp_path))
											my_mkdir(rp9tmp_path);
										snprintf(target_file, MAX_DPATH, "%s%s", rp9tmp_path, content);
										auto f = fopen(target_file, "wbe");
										if (f != nullptr)
										{
											fwrite(buffer, 1, readsize, f);
											fclose(f);
											if (mediatype == 0)
											{
												// Add floppy
												if (priority < 2)
												{
													strncpy(p->floppyslots[0].df, target_file, sizeof(p->floppyslots[0].df));
													disk_insert(0, p->floppyslots[0].df);
												}
												else if (priority == 2 && p->nr_floppies > 1)
												{
													strncpy(p->floppyslots[1].df, target_file, sizeof(p->floppyslots[1].df));
													disk_insert(1, p->floppyslots[1].df);
												}
												else if (priority == 3 && p->nr_floppies > 2)
												{
													strncpy(p->floppyslots[2].df, target_file, sizeof(p->floppyslots[2].df));
													disk_insert(2, p->floppyslots[2].df);
												}
												else if (priority == 4 && p->nr_floppies > 3)
												{
													strncpy(p->floppyslots[3].df, target_file, sizeof(p->floppyslots[3].df));
													disk_insert(3, p->floppyslots[3].df);
												}
												AddFileToDiskList(target_file, 1);
											}
											else
											{
												struct uaedev_config_info ci{};

												if (hardfile_testrdb(target_file))
												{
													ci.physical_geometry = true;
													uci_set_defaults(&ci, true);
												}
												else
												{
													ci.physical_geometry = false;
													uci_set_defaults(&ci, false);
												}

												ci.type = UAEDEV_HDF;
												sprintf(ci.devname, "DH%d", add_HDF_DHnum);
												++add_HDF_DHnum;
												strncpy(ci.rootdir, target_file, MAX_DPATH);

												ci.bootpri = 0;

												const auto uci = add_filesys_config(p, -1, &ci);
												if (uci)
												{
													const auto hfd = get_hardfile_data(uci->configoffset);
													hardfile_media_change(hfd, &ci, true, false);
												}
											}
											lstTmpRP9Files.emplace_back(target_file);
										}
									}
								}
								free(buffer);
							}
						}
					}

					xmlFree(content);
				}
			}
		}
	}
}


static bool parse_manifest(struct uae_prefs* p, unzFile uz, const char* manifest)
{
	auto result = false;
	char buffer[MAX_MANIFEST_ENTRY];

	const auto doc = xmlReadMemory(manifest, strlen(manifest), nullptr, nullptr, 0);
	if (doc != nullptr)
	{
		const auto root_element = xmlDocGetRootElement(doc);
		const auto rp9 = get_node(root_element, "rp9");
		if (rp9 != nullptr)
		{
			const auto app = get_node(rp9, "application");
			if (app != nullptr)
			{
				auto rom = -1;
				auto tmp = get_node(app, "description");
				if (tmp != nullptr && get_value(tmp, "systemrom", buffer, MAX_MANIFEST_ENTRY))
					rom = atoi(buffer);

				tmp = get_node(app, "configuration");
				if (tmp != nullptr && get_value(tmp, "system", buffer, MAX_MANIFEST_ENTRY))
				{
					set_default_system(p, buffer, rom);

					parse_compatibility(p, tmp);
					parse_ram(p, tmp);
					parse_clip(p, tmp);
					parse_peripheral(p, tmp);
					parse_boot(p, tmp);
					extract_media(p, uz, app);
					result = true;
				}
			}
		}
		xmlFreeDoc(doc);
	}

	return result;
}


bool rp9_parse_file(struct uae_prefs* p, const char* filename)
{
	auto result = false;
	unz_file_info file_info;

	add_HDF_DHnum = 0;
	clip_no_hires = false;

	const auto zf = zfile_fopen(filename, _T("rb"));
	if (zf != nullptr)
	{
		auto uz = unzOpen(zf);
		if (uz != nullptr)
		{
			if (unzLocateFile(uz, RP9_MANIFEST, 1) == UNZ_OK)
			{
				if (unzGetCurrentFileInfo(uz, &file_info, nullptr, 0, nullptr, 0, nullptr, 0) == UNZ_OK)
				{
					auto * manifest = static_cast<char *>(malloc(file_info.uncompressed_size + 1));
					if (manifest != nullptr)
					{
						if (unzOpenCurrentFile(uz) == UNZ_OK)
						{
							const auto readsize = unzReadCurrentFile(uz, manifest, file_info.uncompressed_size);
							unzCloseCurrentFile(uz);

							if (readsize == file_info.uncompressed_size)
							{
								manifest[readsize] = '\0';
								result = parse_manifest(p, uz, manifest);

								if (result)
								{
									// Fixup some prefs...
									if (p->m68k_speed >= 0)
										p->cachesize = 0; // Use JIT only if max. speed selected
									p->input_joymouse_multiplier = 5; // Most games need slower mouse movement...
								}
							}
						}
						free(manifest);
					}
				}
			}

			unzClose(uz);
		}
		zfile_fclose(zf);
	}

	return result;
}

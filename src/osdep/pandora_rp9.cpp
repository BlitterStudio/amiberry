#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "autoconf.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "disk.h"
#include "fsdb.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "filesys.h"
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


void rp9_init(void)
{
  fetch_rp9path(rp9tmp_path, MAX_DPATH);
  strncat(rp9tmp_path, _T("tmp/"), MAX_DPATH);
  lstTmpRP9Files.clear();
  LIBXML_TEST_VERSION
}


static void del_tmpFiles(void)
{
  int i;
  
  for(i=0; i<lstTmpRP9Files.size(); ++i)
    remove(lstTmpRP9Files[i].c_str());
  lstTmpRP9Files.clear();
}


void rp9_cleanup(void)
{
  del_tmpFiles();
  xmlCleanupParser();
  xmlMemoryDump();
}


static xmlNode *get_node(xmlNode *node, const char *name)
{
  for(xmlNode *curr_node = node; curr_node; curr_node = curr_node->next) {
    if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, name) == 0)
      return curr_node->children;
  }
  return NULL;
}


static bool get_value(xmlNode *node, const char *key, char *value, int max_size)
{
  bool bResult = false;

  for(xmlNode *curr_node = node; curr_node; curr_node = curr_node->next) {
    if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, key) == 0) {
      xmlChar *content = xmlNodeGetContent(curr_node);
      if(content != NULL) {
        strncpy(value, (char *)content, max_size);
        xmlFree(content);
        bResult = true;
      }
      break;
    }
  }

  return bResult;
}


static void set_default_system(struct uae_prefs *p, const char *system, int rom)
{
  default_prefs(p, 0);
  del_tmpFiles();
  
  if(strcmp(system, "a-500") == 0)
    bip_a500(p, rom);
  else if(strcmp(system, "a-500plus") == 0)
    bip_a500plus(p, rom);
  else if(strcmp(system, "a-1200") == 0)
    bip_a1200(p, rom);
  else if(strcmp(system, "a-2000") == 0)
    bip_a2000(p, rom);
  else if(strcmp(system, "a-4000") == 0)
    bip_a4000(p, rom);
}


static void parse_compatibility(struct uae_prefs *p, xmlNode *node)
{
  for(xmlNode *curr_node = node; curr_node; curr_node = curr_node->next) {
    if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, _T("compatibility")) == 0) {
      xmlChar *content = xmlNodeGetContent(curr_node);
      if(content != NULL) {
        if(strcmp((const char *) content, "flexible-blitter-immediate") == 0)
          p->immediate_blits = 1;
        else if(strcmp((const char *) content, "turbo-floppy") == 0)
          p->floppy_speed = 400;
        else if(strcmp((const char *) content, "flexible-sprite-collisions-spritesplayfield") == 0)
          p->collision_level = 2;
        else if(strcmp((const char *) content, "flexible-sprite-collisions-spritesonly") == 0)
          p->collision_level = 1;
        else if(strcmp((const char *) content, "flexible-sound") == 0)
          p->produce_sound = 2;
        else if(strcmp((const char *) content, "flexible-maxhorizontal-nohires") == 0)
          clip_no_hires = true;
        else if(strcmp((const char *) content, "jit") == 0)
        {
          p->cachesize = 8192;
          p->address_space_24 = 0;
        }
        else if(strcmp((const char *) content, "flexible-cpu-cycles") == 0)
          p->cpu_compatible = 0;
        else if(strcmp((const char *) content, "flexible-maxhorizontal-nosuperhires") == 0)
          ; /* nothing to change */
        else if(strcmp((const char *) content, "flexible-maxvertical-nointerlace") == 0)
          ; /* nothing to change */
        else
          printf("rp9: unknown compatibility: %s\n", content);
        xmlFree(content);
      }      
    }
  }
}


static void parse_ram(struct uae_prefs *p, xmlNode *node)
{
  for(xmlNode *curr_node = node; curr_node; curr_node = curr_node->next) {
    if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, _T("ram")) == 0) {
      xmlChar *content = xmlNodeGetContent(curr_node);
      if(content != NULL) {
        xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("type"));
        if(attr != NULL) 
        {
          int size = atoi((const char *)content);
          if(strcmp((const char *) attr, "fast") == 0)
            p->fastmem_size = size;
          else if(strcmp((const char *) attr, "z3") == 0)
            p->z3fastmem_size = size;
          else if(strcmp((const char *) attr, "chip") == 0)
            p->chipmem_size = size;
          xmlFree(attr);
        }

        xmlFree(content);
      }      
    }
  }
}


static void parse_clip(struct uae_prefs *p, xmlNode *node)
{
  int left = 0, top = 0, width = 320, height = 240;
  
  for(xmlNode *curr_node = node; curr_node; curr_node = curr_node->next) 
  {
    if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, _T("clip")) == 0) 
    {
      xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("left"));
      if(attr != NULL)
      {
        left = atoi((const char *)attr);
        xmlFree(attr);
      }
      attr = xmlGetProp(curr_node, (const xmlChar *) _T("top"));
      if(attr != NULL)
      {
        top = atoi((const char *)attr) / 2;
        p->pandora_vertical_offset = top - 41; // VBLANK_ENDLINE_PAL + OFFSET_Y_ADJUST
        xmlFree(attr);
      }
      attr = xmlGetProp(curr_node, (const xmlChar *) _T("width"));
      if(attr != NULL)
      {
        width = atoi((const char *)attr);
        if(p->chipset_mask & CSMASK_AGA && clip_no_hires == false)
          width = width / 2; // Use Hires in AGA mode
        else
          width = width / 4; // Use Lores in OCS/ECS
        if(width <= 320)
          p->gfx_size.width = 320;
        else if(width <= 352)
          p->gfx_size.width = 352;
        else if(width <= 384)
          p->gfx_size.width = 384;
        else if(width <= 640)
          p->gfx_size.width = 640;
        else if(width <= 704)
          p->gfx_size.width = 704;
        else
          p->gfx_size.width = 768;
        xmlFree(attr);
      }
      attr = xmlGetProp(curr_node, (const xmlChar *) _T("height"));
      if(attr != NULL)
      {
        height = atoi((const char *)attr) / 2;
        if(height <= 200)
          p->gfx_size.height = 200;
        else if(height <= 216)
          p->gfx_size.height = 216;
        else if(height <= 240)
          p->gfx_size.height = 240;
        else if(height <= 256)
          p->gfx_size.height = 256;
        else if(height <= 262)
          p->gfx_size.height = 262;
        else
          p->gfx_size.height = 270;
        xmlFree(attr);
      }
      break;
    }
  }
}


static void parse_peripheral(struct uae_prefs *p, xmlNode *node)
{
  for(xmlNode *curr_node = node; curr_node; curr_node = curr_node->next) 
  {
    if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, _T("peripheral")) == 0) 
    {
      xmlChar *content = xmlNodeGetContent(curr_node);
      if(content != NULL)
      {
        if(strcmp((const char *)content, "floppy") == 0) 
        {
          int type = DRV_35_DD;
          int unit = -1;
          
          xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("type"));
          if(attr != NULL) 
          {
            if(strcmp((const char *) attr, "dd") == 0)
              type = DRV_35_DD;
            else
              type = DRV_35_HD;
            xmlFree(attr);
          }
          
          attr = xmlGetProp(curr_node, (const xmlChar *) _T("unit"));
          if(attr != NULL) 
          {
            unit = atoi((const char *) attr);
            xmlFree(attr);
          }
          
          if(unit >= 0)
          {
            if(unit + 1 > p->nr_floppies)
              p->nr_floppies = unit + 1;
            p->floppyslots[unit].dfxtype = type;
          }
        }
        else if(strcmp((const char *)content, "a-501") == 0)
          p->bogomem_size = 0x00080000;
        else if(strcmp((const char *)content, "cpu") == 0)
        {
          xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("type"));
          if(attr != NULL) 
          {
            p->cpu_model = atoi((const char *) attr);
            if(p->cpu_model > 68020)
              p->address_space_24 = 0;
            if(p->cpu_model == 68040)
              p->fpu_model = 68040;
            xmlFree(attr);
          }
          attr = xmlGetProp(curr_node, (const xmlChar *) _T("speed"));
          if(attr != NULL) 
          {
            if(strcmp((const char *) attr, "max") == 0)
              p->m68k_speed = -1;
            xmlFree(attr);
          }
        }
        else if(strcmp((const char *)content, "fpu") == 0)
        {
          xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("type"));
          if(attr != NULL) 
          {
            if(strcmp((const char *) attr, "68881") == 0)
              p->fpu_model = 68881;
            else if(strcmp((const char *) attr, "68882") == 0)
              p->fpu_model = 68882;
            xmlFree(attr);
          }
        }
        else if(strcmp((const char *)content, "jit") == 0)
        {
          xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("memory"));
          if(attr != NULL) 
          {
            p->cachesize = atoi((const char *) attr) / 1024;
            xmlFree(attr);
          }
        }
        xmlFree(content);
      }      
    }
  }
}


static void parse_boot(struct uae_prefs *p, xmlNode *node)
{
  for(xmlNode *curr_node = node; curr_node; curr_node = curr_node->next) 
  {
    if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, _T("boot")) == 0) 
    {
      xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("type"));
      if(attr != NULL) 
      {
        if(strcmp((const char *) attr, "hdf") == 0)
        {
          // Built-in hdf required
          xmlChar *content = xmlNodeGetContent(curr_node);
          if(content != NULL)
          {
            char target_file[MAX_DPATH];
            fetch_rp9path(target_file, MAX_DPATH);
            strncat(target_file, "workbench-", MAX_DPATH);
            strncat(target_file, (const char *)content, MAX_DPATH);
            strncat(target_file, ".hdf", MAX_DPATH);
            FILE *f = fopen(target_file, "rb");
            if(f != NULL)
            {
              struct uaedev_config_data *uci;
            	struct uaedev_config_info ci;
              
              fclose(f);

              if(hardfile_testrdb (target_file))                        
                uci_set_defaults(&ci, true);
              else
                uci_set_defaults(&ci, false);
              
              ci.type = UAEDEV_HDF;
              sprintf(ci.devname, "DH%d", add_HDF_DHnum);
              ++add_HDF_DHnum;
              strcpy(ci.rootdir, target_file);
              
              xmlChar *ro = xmlGetProp(curr_node, (const xmlChar *) _T("readonly"));
              if(ro != NULL)
              {
                if(strcmp((const char *) ro, "true") == 0)
                  ci.readonly = 1;
                xmlFree(ro);
              }
              ci.bootpri = 127;
              
              uci = add_filesys_config(p, -1, &ci);
              if (uci) {
            		struct hardfiledata *hfd = get_hardfile_data (uci->configoffset);
                hardfile_media_change (hfd, &ci, true, false);
              }
              gui_force_rtarea_hdchange();
            }
            xmlFree(content);
          }
        }
        xmlFree(attr);
      }
    }
  }
}


static void extract_media(struct uae_prefs *p, unzFile uz, xmlNode *node)
{
  xmlNode *tmp = get_node(node, "media");
  if(tmp != NULL) 
  {
    for(xmlNode *curr_node = tmp; curr_node; curr_node = curr_node->next) 
    {
      int mediatype = -1;
      if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, _T("floppy")) == 0)
        mediatype = 0;
      else if (curr_node->type == XML_ELEMENT_NODE && strcmp((const char *)curr_node->name, _T("harddrive")) == 0)
        mediatype = 1;
      if(mediatype >= 0)
      {
        xmlChar *content = xmlNodeGetContent(curr_node);
        if(content != NULL) 
        {
          int priority = 0;
          xmlChar *attr = xmlGetProp(curr_node, (const xmlChar *) _T("priority"));
          if(attr != NULL) 
          {
            priority = atoi((const char *)attr);
            xmlFree(attr);
          }
        	
        	if (unzLocateFile (uz, (char *)content, 1) == UNZ_OK)
          {
            unz_file_info file_info;
            if (unzGetCurrentFileInfo (uz, &file_info, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK)
            {
              void *buffer = malloc(file_info.uncompressed_size);
              if(buffer != NULL)
              {
                if (unzOpenCurrentFile (uz) == UNZ_OK)
                {
                  int readsize = unzReadCurrentFile(uz, buffer, file_info.uncompressed_size);
                  unzCloseCurrentFile(uz);
                  if(readsize == file_info.uncompressed_size)
                  {
                    char target_file[MAX_DPATH];
                    if(!my_existsdir(rp9tmp_path))
                      my_mkdir(rp9tmp_path);
                    snprintf(target_file, MAX_DPATH, "%s%s", rp9tmp_path, content);
                    FILE *f = fopen(target_file, "wb");
                    if(f != NULL)
                    {
                      fwrite(buffer, 1, readsize, f);
                      fclose(f);
                      if(mediatype == 0)
                      {
                        // Add floppy
                        if(priority < 2)
                        {
                  	      strncpy(p->floppyslots[0].df, target_file, sizeof(p->floppyslots[0].df));
                  	      disk_insert(0, p->floppyslots[0].df);
                  	    }
                  	    else if(priority == 2 && p->nr_floppies > 1)
                	      {
                  	      strncpy(p->floppyslots[1].df, target_file, sizeof(p->floppyslots[1].df));
                  	      disk_insert(1, p->floppyslots[1].df);
                	      }
                  	    else if(priority == 3 && p->nr_floppies > 2)
                	      {
                  	      strncpy(p->floppyslots[2].df, target_file, sizeof(p->floppyslots[2].df));
                  	      disk_insert(2, p->floppyslots[2].df);
                	      }
                  	    else if(priority == 4 && p->nr_floppies > 3)
                	      {
                  	      strncpy(p->floppyslots[3].df, target_file, sizeof(p->floppyslots[3].df));
                  	      disk_insert(3, p->floppyslots[3].df);
                	      }
                        AddFileToDiskList(target_file, 1);
                      }
                      else
                      {
                        // Add hardfile
                        struct uaedev_config_data *uci;
                      	struct uaedev_config_info ci;
          
                        if(hardfile_testrdb (target_file))                        
                          uci_set_defaults(&ci, true);
                        else
                          uci_set_defaults(&ci, false);
                        
                        ci.type = UAEDEV_HDF;
                        sprintf(ci.devname, "DH%d", add_HDF_DHnum);
                        ++add_HDF_DHnum;
                        strcpy(ci.rootdir, target_file);
                        
                        ci.bootpri = 0;
                        
                        uci = add_filesys_config(p, -1, &ci);
                        if (uci) {
                      		struct hardfiledata *hfd = get_hardfile_data (uci->configoffset);
                          hardfile_media_change (hfd, &ci, true, false);
                        }

    	                  gui_force_rtarea_hdchange();
                      }
                      lstTmpRP9Files.push_back(target_file);
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


static bool parse_manifest(struct uae_prefs *p, unzFile uz, const char *manifest)
{
  bool bResult = false;
  char buffer[MAX_MANIFEST_ENTRY];
  
  xmlDocPtr doc = xmlReadMemory(manifest, strlen(manifest), NULL, NULL, 0);;
  if(doc != NULL) 
  {
    xmlNode *root_element = xmlDocGetRootElement(doc);
    xmlNode *rp9 = get_node(root_element, "rp9");
    if(rp9 != NULL)
    {
      xmlNode *app = get_node(rp9, "application");
      if(app != NULL)
      {
        int rom = -1;
        xmlNode *tmp = get_node(app, "description");
        if(tmp != NULL && get_value(tmp, "systemrom", buffer, MAX_MANIFEST_ENTRY))
          rom = atoi(buffer);
        
        tmp = get_node(app, "configuration");
        if(tmp != NULL && get_value(tmp, "system", buffer, MAX_MANIFEST_ENTRY))
        {
          set_default_system(p, buffer, rom);
          
          parse_compatibility(p, tmp);
          parse_ram(p, tmp);
          parse_clip(p, tmp);
          parse_peripheral(p, tmp);
          parse_boot(p, tmp);
          extract_media(p, uz, app);
          bResult = true;
        }
      }
    }
    xmlFreeDoc(doc);
  }
  
  return bResult;
}


bool rp9_parse_file(struct uae_prefs *p, const char *filename)
{
  bool bResult = false;
	struct zfile *zf;
	unzFile uz;
  unz_file_info file_info;
  char *manifest;
  
  add_HDF_DHnum = 0;
  clip_no_hires = false;
  
  zf = zfile_fopen(filename, _T("rb"));
  if(zf != NULL)
  {
    uz = unzOpen(zf);
  	if (uz != NULL)
	  {
    	if (unzLocateFile (uz, RP9_MANIFEST, 1) == UNZ_OK)
      {
      	if (unzGetCurrentFileInfo (uz, &file_info, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK)
      	{
          manifest = (char *)malloc(file_info.uncompressed_size + 1);
          if(manifest != NULL)
          {
            if (unzOpenCurrentFile (uz) == UNZ_OK)
            {
              int readsize = unzReadCurrentFile(uz, manifest, file_info.uncompressed_size);
              unzCloseCurrentFile(uz);

              if(readsize == file_info.uncompressed_size)
              {
                manifest[readsize] = '\0';
                bResult = parse_manifest(p, uz, manifest);
                
                if(bResult)
                {
                  // Fixup some prefs...
                  if(p->m68k_speed >= 0)
                    p->cachesize = 0; // Use JIT only if max. speed selected
                  p->input_joymouse_multiplier = 5; // Most games need slower mouse movement...
                }
              }
            }
            free(manifest);
          }
        }
      }
  
    	unzClose (uz);
    }
    zfile_fclose(zf);  
  }
  
  return bResult;
}

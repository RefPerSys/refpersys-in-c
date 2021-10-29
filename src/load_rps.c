/****************************************************************
 * file load_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It contains the initial loading machinery.
 *
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *      Nimesh Neema <nimeshneema@gmail.com>
 *
 *      © Copyright 2019 - 2021 The Reflective Persistent System Team
 *      team@refpersys.org & http://refpersys.org/
 *
 * License:
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
 ******************************************************************************/

#include "Refpersys.h"

#define RPS_LOADER_MAGIC 0x156e62d5	/*359555797 */


enum rps_loaderstate_en
{
  RPSLOAD__NONE,
  RPSLOADING_PARSE_MANIFEST_PASS,
  RPSLOADING_CREATE_OBJECTS_PASS,
  RPSLOADING_FILL_OBJECTS_PASS,
  RPSLOADING_EPILOGUE_PASS,
  RPSLOAD__LAST
};

struct RpsPayl_Loader_st
{
  RPSFIELDS_ZONED_VALUE;
  unsigned ld_magic;		/* always RPS_LOADER_MAGIC */
  enum rps_loaderstate_en ld_state;
  FILE *ld_manifest_file;
  json_t *ld_json_manifest;
  unsigned ld_nbglobroot;
  RpsObject_t **ld_globrootarr;
  unsigned ld_nbconstob;
  RpsObject_t **ld_constobarr;
};

void rps_load_first_pass (RpsLoader_t * ld, int spix, RpsOid_t spaceid);

bool
rps_is_valid_loader (RpsLoader_t * ld)
{
  if (!ld)
    return false;
  if (ld->ld_magic != RPS_LOADER_MAGIC)
    return false;
  return ld->ld_state == RPSLOADING_PARSE_MANIFEST_PASS
    || ld->ld_state == RPSLOADING_CREATE_OBJECTS_PASS
    || ld->ld_state == RPSLOADING_FILL_OBJECTS_PASS
    || ld->ld_state == RPSLOADING_EPILOGUE_PASS;
}				/* end rps_is_valid_loader */

bool
rps_is_valid_filling_loader (RpsLoader_t * ld)
{
  if (!ld)
    return false;
  if (ld->ld_magic == RPS_LOADER_MAGIC)
    return ld->ld_state == RPSLOADING_FILL_OBJECTS_PASS;
  return false;
}				/* end rps_is_valid_filling_loader */

bool
rps_is_valid_creating_loader (RpsLoader_t * ld)
{
  if (!ld)
    return false;
  if (ld->ld_magic == RPS_LOADER_MAGIC)
    return ld->ld_state == RPSLOADING_CREATE_OBJECTS_PASS;
  return false;
}				/* end rps_is_valid_creating_loader */

RpsObject_t *
rps_load_create_object_from_json_id (RpsLoader_t * ld, json_t * js)
{
  RPS_ASSERT (rps_is_valid_creating_loader (ld));
  RPS_ASSERT (js != NULL);
  if (!json_is_string (js))
    return NULL;
  RpsOid_t oid = rps_cstr_to_oid (json_string_value (js), NULL);
  if (!rps_oid_is_valid (oid))
    return NULL;
  return rps_get_loaded_object_by_oid (ld, oid);
}				/* end rps_load_create_object_from_json_id */

void
rps_load_parse_manifest (RpsLoader_t * ld)
{
  int linenum = 0;
  char manifestpath[256];
  memset (manifestpath, 0, sizeof (manifestpath));
  if (!rps_is_valid_loader (ld))
    RPS_FATAL ("invalid loader %p to rps_load_parse_manifest", ld);
  snprintf (manifestpath, sizeof (manifestpath),
	    "%s/rps_manifest.json", rps_load_directory);
  ld->ld_manifest_file = fopen (manifestpath, "r");
  if (!ld->ld_manifest_file)
    RPS_FATAL ("missing loader manifest file %s - %m", manifestpath);
  /// skip initial comment lines in manifest
  {
    char linbuf[256];
    long linoff = -1;
    while (memset (linbuf, 0, sizeof (linbuf)),
	   (linoff = ftell (ld->ld_manifest_file)),
	   fgets (linbuf, sizeof (linbuf), ld->ld_manifest_file))
      {
	linenum++;
	if (linbuf[0] != '/')
	  break;
      }
    fseek (ld->ld_manifest_file, linoff, SEEK_SET);
  }
  /// parse the JSON of the manifest
  {
    json_error_t jerr = { };
    ld->ld_json_manifest
      = json_loadf (ld->ld_manifest_file,
		    JSON_REJECT_DUPLICATES | JSON_DISABLE_EOF_CHECK, &jerr);
    if (!ld->ld_json_manifest)
      RPS_FATAL
	("failed to parse JSON in manifest file %s line %d - %s @%sL%dC%d",
	 manifestpath, linenum, jerr.text, jerr.source, jerr.line,
	 jerr.column);
  }
  json_t *jsglobroot = json_object_get (ld->ld_json_manifest, "globalroots");
  ld->ld_state = RPSLOADING_CREATE_OBJECTS_PASS;
  if (json_is_array (jsglobroot))
    {
      unsigned nbgr = json_array_size (jsglobroot);
      ld->ld_nbglobroot = 0;
      ld->ld_globrootarr = RPS_ALLOC_ZEROED (nbgr * sizeof (RpsObject_t *));
      for (int gix = 0; gix < (int) nbgr; gix++)
	{
	  RpsObject_t *curoot = NULL;
	  json_t *curjs = json_array_get (jsglobroot, gix);
	  if (json_is_string (curjs))
	    {
	      curoot = rps_load_create_object_from_json_id (ld, curjs);
	      ld->ld_globrootarr[ld->ld_nbglobroot++] = curoot;
	    }
	  else
	    RPS_FATAL ("bad JSON for global #%d", gix);
	  RPS_ASSERT (curoot);
	};
      /// now that the infant root objects are created, we can assign
      /// them to global C variables:
#define RPS_INSTALL_ROOT_OB(Oid) do {                           \
        RpsOid_t curoid##Oid =                                  \
          rps_cstr_to_oid(#Oid, NULL);				\
        RPS_ROOT_OB(Oid) =                                      \
          rps_find_object_by_oid(curoid##Oid);                  \
        if (!RPS_ROOT_OB(Oid))                                  \
            RPS_FATAL("failed to install root object %s",       \
                      #Oid);                                    \
            } while (0);
#include "generated/rps-roots.h"
    }
  json_t *jsconstset = json_object_get (ld->ld_json_manifest, "constset");
  if (json_is_array (jsconstset))
    {
      unsigned nbcon = json_array_size (jsconstset);
      ld->ld_nbconstob = 0;
      ld->ld_constobarr = RPS_ALLOC_ZEROED (nbcon * sizeof (RpsObject_t *));
      for (int cix = 0; cix < (int) nbcon; cix++)
	{
	  json_t *curjs = json_array_get (jsconstset, cix);
	  if (json_is_string (curjs))
	    ld->ld_globrootarr[ld->ld_nbconstob++]
	      = rps_load_create_object_from_json_id (ld, curjs);
	}
    }
  printf ("Created %u global roots and %u constants from directory %s\n",
	  ld->ld_nbglobroot, ld->ld_nbconstob, rps_load_directory);
#warning missing code using the JSON manifest in rps_load_parse_manifest
  fclose (ld->ld_manifest_file), ld->ld_manifest_file = NULL;
}				/* end rps_load_parse_manifest */

void
rps_load_initial_heap (void)
{
  RpsLoader_t *loader = RPS_ALLOC_ZONE (sizeof (RpsLoader_t), -RpsPyt_Loader);
  loader->ld_magic = RPS_LOADER_MAGIC;
  loader->ld_state = RPSLOADING_PARSE_MANIFEST_PASS;
  rps_load_parse_manifest (loader);
  json_t *jsspaceset = json_object_get (loader->ld_json_manifest, "spaceset");
  if (json_is_array (jsspaceset))
    {
      unsigned nbspace = json_array_size (jsspaceset);
      for (int spix = 0; spix < (int) nbspace; spix++)
	{
	  json_t *jscurspace = json_array_get (jsspaceset, spix);
	  if (!jscurspace || !json_is_string (jscurspace))
	    RPS_FATAL ("invalid space #%d in directory %s\n",
		       spix, rps_load_directory);
	  const char *spacestr = json_string_value (jscurspace);
	  RpsOid_t spaceid = rps_cstr_to_oid (spacestr, NULL);
	  if (!rps_oid_is_valid (spaceid))
	    RPS_FATAL ("invalid space #%d id %s in directory %s\n",
		       spix, spacestr, rps_load_directory);
	  rps_load_first_pass (loader, spix, spaceid);
	}
    }
#warning rps_load_initial_heap needs to be completed
  RPS_FATAL ("incomplete rps_load_initial_heap load directory %s",
	     rps_load_directory);
}				/* end rps_load_initial_heap */


void
rps_load_first_pass (RpsLoader_t * ld, int spix, RpsOid_t spaceid)
{
  char spacebuf[32];
  char filepath[256];
  memset (spacebuf, 0, sizeof (spacebuf));
  memset (filepath, 0, sizeof (filepath));
  rps_oid_to_cbuf (spaceid, spacebuf);
  snprintf (filepath, sizeof (filepath),
	    "%s/persistore/sp%s-rps.json", rps_load_directory, spacebuf);
  FILE *spfil = fopen (filepath, "r");
  if (!spfil)
    RPS_FATAL ("failed to open %s for space #%d : %m", filepath, spix);
#warning rps_load_first_pass has to be coded
  RPS_FATAL
    ("unimplemented rps_load_first_pass spix#%d space %s load directory %s",
     spix, spacebuf, rps_load_directory);
}				/* end rps_load_first_pass */

/************************ end of file load_rps.c *****************/

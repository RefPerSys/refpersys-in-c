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
	  printf ("spix=%d spacestr:%s load-dir %s (%s:%d)\n",
		  spix, spacestr, rps_load_directory, __FILE__, __LINE__);
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
  printf
    ("rps_load_first_pass spaceid {id_hi=%015ld,id_lo=%015ld} %s (%s:%d)\n",
     spaceid.id_hi, spaceid.id_lo, spacebuf, __FILE__, __LINE__);
  snprintf (filepath, sizeof (filepath), "%s/persistore/sp%s-rps.json",
	    rps_load_directory, spacebuf);
  FILE *spfil = fopen (filepath, "r");
  if (!spfil)
    RPS_FATAL ("failed to open %s for space #%d : %m", filepath, spix);
  long linoff = 0;
  int lincnt = 0;
  char linbuf[256];
  do
    {
      memset (linbuf, 0, sizeof (linbuf));
      linoff = ftell (spfil);
      if (!fgets (linbuf, sizeof (linbuf), spfil))
	break;
      if (linbuf[0] == '/' || isspace (linbuf[0]))
	continue;
      lincnt++;
      if (linbuf[0] == '{')
	break;
    }
  while (!feof (spfil));
  //// parse the JSON prologue
  fseek (spfil, linoff, SEEK_SET);
  json_error_t jerror = { };
  json_t *jsprologue = json_loadf (spfil, JSON_DISABLE_EOF_CHECK, &jerror);
  json_t *jsformat = NULL, *jsnbobjects = NULL, *jsspaceid = NULL;
  long nbobjects = -1;
  if (!jsprologue)
    RPS_FATAL ("failed to read prologue for space #%d in %s:%d - %s",
	       spix, filepath, lincnt, jerror.text);
  if (!json_is_object (jsprologue)
      || !(jsformat = json_object_get (jsprologue, "format"))
      || !(jsnbobjects = json_object_get (jsprologue, "nbobjects"))
      || !(jsspaceid = json_object_get (jsprologue, "spaceid")))
    RPS_FATAL ("invalid prologue JSON for space #%d in %s:%d", spix, filepath,
	       lincnt);
  if (!json_is_string (jsformat)
      || strcmp (json_string_value (jsformat), RPS_MANIFEST_FORMAT))
    RPS_FATAL
      ("invalid prologue JSON for space #%d in %s:%d format, expecting %s",
       spix, filepath, lincnt, RPS_MANIFEST_FORMAT);
  if (!json_is_string (jsspaceid)
      || strcmp (json_string_value (jsspaceid), spacebuf))
    RPS_FATAL
      ("invalid prologue JSON for space #%d  in %s:%d bad spaceid - expecting %s",
       spix, filepath, lincnt, spacebuf);
  if (json_is_integer (jsnbobjects))
    nbobjects = json_integer_value (jsnbobjects);
  if (nbobjects < 0)
    RPS_FATAL
      ("invalid prologue JSON for space #%d in %s:%d - bad nbobjects %ld",
       spix, filepath, lincnt, nbobjects);
  printf ("rps_load_first_pass should load %ld objects from %s\n",
	  nbobjects, filepath);
  json_decref (jsprologue), jsprologue = NULL, jsnbobjects = NULL, jsspaceid =
    NULL;
  /*****************
   * TODO:
   *  loop and search for start of objects JSON....
   *****************/
  long objcount = 0;
  while (objcount < nbobjects)
    {
      if (feof (spfil))
	RPS_FATAL
	  ("rps_load_first_pass space#%d incomplete file %s:%d - loaded only %ld objects expecting %ld of them",
	   spix, filepath, lincnt, objcount, nbobjects);
      memset (linbuf, 0, sizeof (linbuf));
      linoff = ftell (spfil);
      if (!fgets (linbuf, sizeof (linbuf), spfil))
	break;
      lincnt++;
      RpsOid_t curobid = RPS_NULL_OID;
      {
	int endcol = -1;
	char obidbuf[32];
	memset (obidbuf, 0, sizeof (obidbuf));
	/// should test for lines starting objects, i.e. //+ob.... then
	/// fetch all the lines in some buffer, etc...
	if (sscanf (linbuf, "//+ob_%18[0-9A-Za-z]", obidbuf) >= 1)
	  curobid = rps_cstr_to_oid (obidbuf, NULL);
	if (rps_oid_is_valid (curobid))
	  {
	    char endlin[48];
	    memset (endlin, 0, sizeof (endlin));
	    snprintf (endlin, sizeof (endlin), "//-ob_%s\n", obidbuf);
	    size_t bufsz = 256;
	    char *bufjs = RPS_ALLOC_ZEROED (bufsz);
	    char *linbuf = NULL;
	    size_t linsz = 0;
	    long startlin = lincnt;
	    FILE *obstream = open_memstream (&bufjs, &bufsz);
	    RPS_ASSERT (obstream);
	    while (getline (&linbuf, &linsz, spfil) > 0)
	      {
		lincnt++;
		if (!strcmp (linbuf, endlin))
		  break;
		if (linbuf[0] != '/')
		  fputs (linbuf, obstream);
	      }
	    fputc ('\n', obstream);
	    fflush (obstream);
	    long obsiz = ftell (obstream);
	    json_error_t jerror = { };
	    json_t *jsobject =
	      json_loadb (bufjs, obsiz, JSON_DISABLE_EOF_CHECK, &jerror);
	    if (!jsobject)
	      RPS_FATAL
		("failed to parse JSON#%ld in spix#%d  at %s:%ld - %s",
		 objcount, spix, filepath, startlin, jerror.text);
	    json_t *jsoid = json_object_get (jsobject, "oid");
	    if (!json_is_string (jsoid))
	      RPS_FATAL
		(" JSON#%ld in spix#%d  at %s:%ld without oid JSON attribute",
		 objcount, spix, filepath, startlin);
	    if (strcmp (json_string_value (jsoid), obidbuf))
	      RPS_FATAL
		(" JSON#%ld in spix#%d  at %s:%ld with bad oid JSON attribute %s - expecting %s",
		 objcount, spix, filepath, startlin,
		 json_string_value (jsoid), obidbuf);
#warning should parse the JSON in bufjs of size bufsz
	    /// should create the object, set its class, ...
	    objcount++;
	    /// should json_decref(jsobject) after handling it
	  }
      }
    }
  printf ("rps_load_first_pass parsed %ld objects at %s:%d\n", objcount,
	  filepath, lincnt);
#warning rps_load_first_pass has to be coded
  RPS_FATAL
    ("unimplemented rps_load_first_pass spix#%d space %s load directory %s",
     spix, spacebuf, rps_load_directory);
}				/* end rps_load_first_pass */

/************************ end of file load_rps.c *****************/

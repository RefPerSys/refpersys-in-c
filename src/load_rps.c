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
 *      © Copyright 2019 - 2022 The Reflective Persistent System Team
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

#include "malloc.h"		/*for mallopt */

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
  double ld_start_elapsedtime;
  double ld_start_processcputime;
  unsigned ld_nbglobroot;
  RpsObject_t **ld_globrootarr;
  unsigned ld_nbconstob;
  RpsObject_t **ld_constobarr;
  unsigned ld_totalobjectnb;
};

void rps_load_first_pass (RpsLoader_t * ld, int spix, RpsOid spaceid);
void rps_load_second_pass (RpsLoader_t * ld, int spix, RpsOid spaceid);
void rps_loader_fill_object_second_pass (RpsLoader_t * ld, int spix,
					 RpsObject_t * obj, json_t * jsobj,
					 RpsObject_t * obspac);
void rps_load_install_global_root_objects (RpsLoader_t * ld);

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

unsigned
rps_loader_nb_globals (RpsLoader_t * ld)
{
  if (!rps_is_valid_loader (ld))
    return 0;
  return ld->ld_nbglobroot;
}				/* end rps_loader_nb_globals */

unsigned
rps_loader_nb_constants (RpsLoader_t * ld)
{
  if (!rps_is_valid_loader (ld))
    return 0;
  return ld->ld_nbconstob;
}				/* end rps_loader_nb_constants */

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
  RpsOid oid = rps_cstr_to_oid (json_string_value (js), NULL);
  if (!rps_oid_is_valid (oid))
    return NULL;
  return rps_get_loaded_object_by_oid (ld, oid);
}				/* end rps_load_create_object_from_json_id */

void
rps_load_parse_manifest (RpsLoader_t * ld)
{
  int linenum = 0;
  long totnbob = 0;
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
  json_t *jstotobjnb =
    json_object_get (ld->ld_json_manifest, "totalobjectnumber");
  json_t *jsformat = json_object_get (ld->ld_json_manifest, "format");
  if (!json_is_string (jsformat)
      || strcmp (json_string_value (jsformat), RPS_MANIFEST_FORMAT))
    RPS_FATAL ("bad JSON format in manifest file %s, expecting %s",
	       manifestpath, RPS_MANIFEST_FORMAT);

  if (!json_is_integer (jstotobjnb))
    RPS_FATAL ("missing totalobjectnumber JSON attribute in manifest file %s",
	       manifestpath);
  totnbob = json_integer_value (jstotobjnb);
  ld->ld_state = RPSLOADING_CREATE_OBJECTS_PASS;
  if (json_is_array (jsglobroot))
    {
      unsigned nbgr = json_array_size (jsglobroot);
      RPS_ASSERT (nbgr > 0);
      ld->ld_nbglobroot = 0;
      ld->ld_globrootarr = RPS_ALLOC_ZEROED (nbgr * sizeof (RpsObject_t *));
      rps_initialize_objects_for_loading (ld, totnbob);
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
        RpsOid curoid##Oid =					\
          rps_cstr_to_oid(#Oid, NULL);				\
        RPS_ROOT_OB(Oid) =                                      \
          rps_find_object_by_oid(curoid##Oid);                  \
        if (!RPS_ROOT_OB(Oid))                                  \
            RPS_FATAL("failed to install root object %s",       \
                      #Oid);                                    \
            } while (0);
#include "generated/rps-roots.h"
    }
  else
    RPS_FATAL ("missing globalroots in manifest file %s", manifestpath);
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


/// this routine sets the class of root objects temporarily to `object`
void
rps_load_initialize_root_objects (RpsLoader_t * loader)
{
  RPS_ASSERT (rps_is_valid_loader (loader));
  RpsOid oid = RPS_OID_NULL;
#define RPS_INSTALL_ROOT_OB(Oidstr) do {			\
  oid= rps_cstr_to_oid(#Oidstr, NULL);				\
  RpsObject_t* rootob = rps_find_object_by_oid(oid);		\
  if (rootob && !rootob->ob_class)				\
    rootob->ob_class						\
      = RPS_ROOT_OB(_5yhJGgxLwLp00X0xEQ);	/*object∈class*/	\
  } while(0);
#include "generated/rps-roots.h"
}				/* end rps_load_initialize_root_objects */

void
rps_load_initial_heap (void)
{
  unsigned nbspace = 0;
  RpsLoader_t *loader = RPS_ALLOC_ZONE (sizeof (RpsLoader_t), -RpsPyt_Loader);
  loader->ld_magic = RPS_LOADER_MAGIC;
  loader->ld_state = RPSLOADING_PARSE_MANIFEST_PASS;
  loader->ld_start_elapsedtime = rps_clocktime (CLOCK_REALTIME);
  loader->ld_start_processcputime = rps_clocktime (CLOCK_PROCESS_CPUTIME_ID);
  printf ("rps_load_initial_heap directory %s\n", rps_load_directory);
  RPS_DEBUG_NLPRINTF (LOAD, "loading directory %s with loader @%p",
		      rps_load_directory, loader);
  rps_load_parse_manifest (loader);
  RPS_DEBUG_PRINTF (LOAD, "parsed load manifest from %s", rps_load_directory);
  rps_check_all_objects_buckets_are_valid ();
  json_t *jsspaceset = json_object_get (loader->ld_json_manifest, "spaceset");
  if (json_is_array (jsspaceset))
    {
      nbspace = json_array_size (jsspaceset);
      for (int spix = 0; spix < (int) nbspace; spix++)
	{
	  json_t *jscurspace = json_array_get (jsspaceset, spix);
	  if (!jscurspace || !json_is_string (jscurspace))
	    RPS_FATAL ("invalid space #%d in directory %s\n",
		       spix, rps_load_directory);
	  const char *spacestr = json_string_value (jscurspace);
	  printf ("spix=%d spacestr:%s load-dir %s (%s:%d)\n",
		  spix, spacestr, rps_load_directory, __FILE__, __LINE__);
	  RpsOid spaceid = rps_cstr_to_oid (spacestr, NULL);
	  if (!rps_oid_is_valid (spaceid))
	    RPS_FATAL ("invalid space #%d id %s in directory %s\n",
		       spix, spacestr, rps_load_directory);
	  rps_load_first_pass (loader, spix, spaceid);
	  rps_check_all_objects_buckets_are_valid ();
	}
    }
  else
    RPS_FATAL ("bad spaceset in load directory %s", rps_load_directory);
  loader->ld_state = RPSLOADING_FILL_OBJECTS_PASS;
  rps_load_initialize_root_objects (loader);
  for (int spix = 0; spix < (int) nbspace; spix++)
    {
      json_t *jscurspace = json_array_get (jsspaceset, spix);
      const char *spacestr = json_string_value (jscurspace);
      RpsOid spaceid = rps_cstr_to_oid (spacestr, NULL);
      rps_load_second_pass (loader, spix, spaceid);
      rps_check_all_objects_buckets_are_valid ();
    };
  loader->ld_state = RPSLOADING_EPILOGUE_PASS;
  rps_load_install_global_root_objects (loader);
#warning temporary call to mallopt. Should be removed once loading and dumping completes.
  mallopt (M_CHECK_ACTION, 03);
  double elapsedtime =
    rps_clocktime (CLOCK_REALTIME) - loader->ld_start_elapsedtime;
  double processcputime =
    rps_clocktime (CLOCK_PROCESS_CPUTIME_ID) -
    loader->ld_start_processcputime;
  long totnbob = loader->ld_totalobjectnb;
  // we don't free the loader, it will be later garbage collected!
  printf
    ("*REFPERSYS* loaded %ld objects in %d spaces in %.3f elapsed %.3f cpu seconds (git %s)\n"
     ".. %.3f elapsed %.3f cpu µs/obj\n", totnbob, nbspace, elapsedtime,
     processcputime, _rps_git_short_id, (1.0e6 * elapsedtime) / totnbob,
     (1.0e6 * processcputime) / totnbob);
}				/* end rps_load_initial_heap */


void
rps_load_first_pass (RpsLoader_t * ld, int spix, RpsOid spaceid)
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
  rps_check_all_objects_buckets_are_valid ();
  long linoff = 0;
  int lincnt = 0;
  size_t linsz = 256;
  char *linbuf = RPS_ALLOC_ZEROED (linsz);
  do
    {
      memset (linbuf, 0, sizeof (linbuf));
      linoff = ftell (spfil);
      if (!fgets (linbuf, linsz, spfil))
	break;
      lincnt++;
      if (linbuf[0] == '/' || isspace (linbuf[0]))
	continue;
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
  printf ("rps_load_first_pass should load %ld objects from %s (%s:%d)\n",
	  nbobjects, filepath, __FILE__, __LINE__);
  lincnt += json_object_size (jsprologue);
  json_decref (jsprologue), jsprologue = NULL,
    jsnbobjects = NULL, jsspaceid = NULL;
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
      if (objcount % 8 == 0)
	{
	  rps_check_all_objects_buckets_are_valid ();
//     if (objcount % 16 == 0)
//       printf
//         ("rps_load_first_pass space#%d objcount %ld file %s:%d (%s:%d)\n",
//          spix, objcount, filepath, lincnt, __FILE__, __LINE__);
	};
      memset (linbuf, 0, linsz);
      linoff = ftell (spfil);
      if (!fgets (linbuf, linsz, spfil))
	break;
      lincnt++;
      if (lincnt % 16 == 0)
	rps_check_all_objects_buckets_are_valid ();
      if (isspace (linbuf[0]))
	continue;
      RpsOid curobid = RPS_OID_NULL;
      {
	int endcol = -1;
	char obidbuf[32];
	memset (obidbuf, 0, sizeof (obidbuf));
	/// should test for lines starting objects, i.e. //+ob.... then
	/// fetch all the lines in some buffer, etc...
	if (sscanf (linbuf, "//+ob_%18[0-9A-Za-z]", obidbuf + 1) >= 1)
	  {
	    obidbuf[0] = '_';
	    curobid = rps_cstr_to_oid (obidbuf, NULL);
	  }
	if (rps_oid_is_valid (curobid))
	  {
	    char endlin[48];
	    memset (endlin, 0, sizeof (endlin));
	    snprintf (endlin, sizeof (endlin), "//-ob%s\n", obidbuf);
	    rps_check_all_objects_buckets_are_valid ();
	    size_t bufsz = 256;
	    char *bufjs = RPS_ALLOC_ZEROED (bufsz);
	    long startlin = lincnt;
	    FILE *obstream = open_memstream (&bufjs, &bufsz);
	    RPS_ASSERT (obstream);
	    while ((linbuf[0] = (char) 0),
		   getline (&linbuf, &linsz, spfil) > 0)
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
	    json_t *jsclass = json_object_get (jsobject, "class");
	    RpsObject_t *obclass = NULL;
	    if (json_is_string (jsclass))
	      obclass = rps_load_create_object_from_json_id (ld, jsclass);
	    else
	      obclass = RPS_ROOT_OB (_5yhJGgxLwLp00X0xEQ);	//object∈class
	    RPS_ASSERTPRINTF (obclass != NULL,
			      "no class for object of oid %s near %s:%d",
			      obidbuf, filepath, lincnt);
	    RpsObject_t *curob =
	      rps_load_create_object_from_json_id (ld, jsoid);
	    if (curob && obclass)
	      curob->ob_class = obclass;
	    /// the other fields of curob are set later... in the second pass
	    objcount++;
	    json_decref (jsobject);
	    fclose (obstream);
	    free (bufjs), bufjs = NULL;
	    bufsz = 0;
	  }
	else			// invalid oid
	  RPS_FATAL ("in %s:%d invalid oid %s", filepath, lincnt, obidbuf);
      }
    }
  printf ("rps_load_first_pass created %ld objects at %s:%d (%s:%d)\n",
	  objcount, filepath, lincnt, __FILE__, __LINE__);
  fclose (spfil);
  rps_check_all_objects_buckets_are_valid ();
}				/* end rps_load_first_pass */



RpsObject_t *
rps_loader_json_to_object (RpsLoader_t * ld, json_t * jv)
{
  RpsObject_t *obres = NULL;
  RPS_ASSERT (rps_is_valid_loader (ld));
  RPS_ASSERT (jv != NULL);
  RpsOid oid = RPS_OID_NULL;
  if (json_is_string (jv))
    {
      const char *end = NULL;
      oid = rps_cstr_to_oid (json_string_value (jv), &end);
      if (end && *end == 0)
	obres = rps_find_object_by_oid (oid);
    }
  else if (json_is_object (jv))
    {
      json_t *joid = json_object_get (jv, "oid");
      if (joid && json_is_string (joid))
	{
	  const char *end = NULL;
	  oid = rps_cstr_to_oid (json_string_value (joid), &end);
	  if (end && *end == 0)
	    obres = rps_find_object_by_oid (oid);
	}
    }
  else if (json_is_null (jv))
    obres = NULL;
  return obres;
}				/* end rps_loader_json_to_object */



/// This function should be compatible with conventions followed by
/// rps_dump_json_for_value function in file dumper_rps.c
RpsValue_t
rps_loader_json_to_value (RpsLoader_t * ld, json_t * jv)
{
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  RPS_ASSERT (jv != NULL);
  if (json_is_integer (jv))
    return rps_tagged_integer_value (json_integer_value (jv));
  else if (json_is_real (jv))
    return (RpsValue_t) rps_alloc_boxed_double (json_real_value (jv));
  else if (json_is_null (jv))
    return RPS_NULL_VALUE;
  else if (json_is_string (jv))
    {
      const char *str = json_string_value (jv);
      RPS_ASSERT (str != NULL);
      if (str[0] == '_' && isdigit (str[1]))
	{
	  RpsOid oid = RPS_OID_NULL;
	  const char *end = NULL;
	  oid = rps_cstr_to_oid (str, &end);
	  if (end && *end == (char) 0 && rps_oid_is_valid (oid))
	    {
	      RpsObject_t *obj = rps_find_object_by_oid (oid);
	      if (obj != NULL)
		return (RpsValue_t) obj;
	    };
	}
      return (RpsValue_t) rps_alloc_string (str);
    }
  else if (json_is_object (jv))
    {
      json_t *jsvtype = json_object_get (jv, "vtype");
      if (!jsvtype || !json_is_string (jsvtype))
	RPS_FATAL ("rps_loader_json_to_value missing vtype \n... json %s",
		   json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
      const char *strvtyp = json_string_value (jsvtype);
      if (!strcmp (strvtyp, "closure"))
	{
	  json_t *jsenv = json_object_get (jv, "env");
	  json_t *jsfn = json_object_get (jv, "fn");
	  json_t *jsmeta = json_object_get (jv, "meta");
	  if (!json_is_array (jsenv) || !json_is_string (jsfn))
	    goto corruptedjson;
	  RpsValue_t vmeta = RPS_NULL_VALUE;
	  if (jsmeta)
	    vmeta = rps_loader_json_to_value (ld, jsmeta);
	  RPS_ASSERT (vmeta == RPS_NULL_VALUE
		      || rps_value_type (vmeta) > RPS_TYPE__NONE);
	  RpsObject_t *obfn = rps_loader_json_to_object (ld, jsfn);
	  RPS_ASSERT (obfn != NULL);
	  unsigned envsiz = json_array_size (jsenv);
	  if (envsiz < 15)
	    {
	      RpsValue_t envarrv[16];
	      memset (envarrv, 0, sizeof (envarrv));
	      for (unsigned vix = 0; vix < envsiz; vix++)
		{
		  envarrv[vix] =	//
		    rps_loader_json_to_value (ld,
					      json_array_get (jsenv, vix));
		}
	      return (RpsValue_t) rps_closure_array_make (obfn,	//
							  /*meta: */ vmeta,
							  envsiz, envarrv);
	    }
	  else
	    {
	      RpsValue_t *envdynarr =
		RPS_ALLOC_ZEROED ((envsiz + 1) * sizeof (RpsValue_t));
	      for (unsigned vix = 0; vix < envsiz; vix++)
		{
		  envdynarr[vix] =	//
		    rps_loader_json_to_value (ld,
					      json_array_get (jsenv, vix));
		}
	      const RpsClosure_t *clos =
		rps_closure_array_make (obfn, /*meta: */ vmeta,
					envsiz, envdynarr);
	      free (envdynarr);
	      return (RpsValue_t) clos;
	    }
	}
      else if (!strcmp (strvtyp, "string"))
	{
	  json_t *jstr = json_object_get (jv, "string");
	  if (json_is_string (jstr))
	    return (RpsValue_t) rps_alloc_string (json_string_value (jstr));
	}
      else if (!strcmp (strvtyp, "json"))
	{
	  return (RpsValue_t) rps_alloc_json (json_object_get (jv, "json"));
	}
      else if (!strcmp (strvtyp, "tuple"))
	{
	  json_t *jstuple = json_object_get (jv, "tuple");
	  if (json_is_array (jstuple))
	    {
	      unsigned tupsiz = json_array_size (jstuple);
	      if (tupsiz < 15)
		{
		  RpsObject_t *tuparr[16];
		  memset (tuparr, 0, sizeof (tuparr));
		  for (int tix = 0; tix < (int) tupsiz; tix++)
		    {
		      tuparr[tix] =	//
			rps_loader_json_to_object (ld,
						   json_array_get (jstuple,
								   tix));
		    }
		  return (RpsValue_t) rps_alloc_tuple_sized (tupsiz, tuparr);
		}
	      else
		{
		  RpsObject_t **dynarr =
		    RPS_ALLOC_ZEROED ((tupsiz + 1) * sizeof (RpsObject_t *));
		  for (int tix = 0; tix < (int) tupsiz; tix++)
		    {
		      dynarr[tix] =	//
			rps_loader_json_to_object (ld,
						   json_array_get (jstuple,
								   tix));
		    };
		  RpsValue_t vtup =
		    (RpsValue_t) rps_alloc_tuple_sized (tupsiz, dynarr);
		  free (dynarr);
		  return vtup;
		}
	    }
	}
#warning incomplete rps_loader_json_to_value
      RPS_FATAL ("incomplete rps_loader_json_to_value \n... json %s",
		 json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
    corruptedjson:
      RPS_FATAL ("rps_loader_json_to_value corrupted ...\n... json %s",
		 json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
    }
  else
    RPS_FATAL ("rps_loader_json_to_value unexpected ...\n... json %s",
	       json_dumps (jv, JSON_INDENT (2) | JSON_SORT_KEYS));
}				/* end rps_loader_json_to_value */




void
rps_loader_fill_object_second_pass (RpsLoader_t * ld, int spix,
				    RpsObject_t * obj, json_t * jsobj,
				    RpsObject_t * obspac)
{
  RPS_ASSERT (rps_is_valid_filling_loader (ld));
  RPS_ASSERT (obj != NULL);
  RPS_ASSERT (json_is_object (jsobj));
  char obidbuf[32];
  memset (obidbuf, 0, sizeof (obidbuf));
  rps_oid_to_cbuf (obj->ob_id, obidbuf);
  RPS_DEBUG_NLPRINTF (LOAD, "start load&fill object %s @%p", obidbuf,
		      (void *) obj);
  pthread_mutex_lock (&obj->ob_mtx);
  /// set the object class
  {
    json_t *jsclass = json_object_get (jsobj, "class");
    RPS_ASSERT (json_is_string (jsclass));
    RpsOid classoid = rps_cstr_to_oid (json_string_value (jsclass), NULL);
    RpsObject_t *classob = rps_find_object_by_oid (classoid);
    RPS_ASSERT (classob != NULL);
    obj->ob_class = classob;
  }
  /// set the object mtime and space
  {
    json_t *jsmtime = json_object_get (jsobj, "mtime");
    RPS_ASSERT (json_is_real (jsmtime));
    double mtime = json_real_value (jsmtime);
    RPS_ASSERT (mtime > 0.0 && mtime < 1e12);
    obj->ob_mtime = mtime;
    obj->ob_space = obspac;
  }
  /// load the object attributes
  {
    json_t *jsattrarr = json_object_get (jsobj, "attrs");
    if (json_is_array (jsattrarr))
      {
	int nbattr = json_array_size (jsattrarr);
	obj->ob_attrtable =
	  rps_alloc_empty_attr_table (nbattr + nbattr / 4 + 1);
	for (int aix = 0; aix < nbattr; aix++)
	  {
	    json_t *jscurattr = json_array_get (jsattrarr, aix);
	    if (!json_is_object (jscurattr))
	      continue;
	    json_t *jsat = json_object_get (jscurattr, "at");
	    json_t *jsva = json_object_get (jscurattr, "va");
	    RPS_ASSERT (json_is_string (jsat));
	    RPS_ASSERT (jsva != NULL);
	    const char *atstr = json_string_value (jsat);
	    RpsOid atoid = rps_cstr_to_oid (atstr, NULL);
	    RpsObject_t *atob = rps_find_object_by_oid (atoid);
	    RPS_ASSERT (atob != NULL);
	    RpsValue_t atval = rps_loader_json_to_value (ld, jsva);
	    obj->ob_attrtable =
	      rps_attr_table_put (obj->ob_attrtable, atob, atval);
	  }
      }
  }
  /// load the object components
  {
    json_t *jscomparr = json_object_get (jsobj, "comps");
    if (json_is_array (jscomparr))
      {
	int nbcomp = json_array_size (jscomparr);
	if (nbcomp > 0)
	  {
	    /* Notice that the below call also locks the mutex. When C
	       code is generated, we could avoid this useless
	       lock.... */
	    rps_object_reserve_components (obj, nbcomp);
	    RPS_ASSERT (obj->ob_comparr != NULL);
	    for (int cix = 0; cix < nbcomp; cix++)
	      obj->ob_comparr[cix] =
		rps_loader_json_to_value (ld,
					  json_array_get (jscomparr, cix));
	    obj->ob_nbcomp = nbcomp;
	  }
      }
  }
  //// load the object payload
  {
    json_t *jspayload = json_object_get (jsobj, "payload");
    if (json_is_string (jspayload))
      {
	char paylroutname[80];
	memset (paylroutname, 0, sizeof (paylroutname));
	snprintf (paylroutname, sizeof (paylroutname),
		  RPS_PAYLOADING_PREFIX "%s", json_string_value (jspayload));
	void *routad = dlsym (rps_dlhandle, paylroutname);
	if (!routad)
	  RPS_FATAL ("failed dlsym %s: %s - for loading payload of object %s in space#%d\n... json %s", paylroutname, dlerror (), obidbuf, spix,	//
		     json_dumps (jsobj, JSON_INDENT (2) | JSON_SORT_KEYS));
	rpsldpysig_t *payloader = (rpsldpysig_t *) routad;
	(*payloader) (obj, ld, jsobj, spix);
      }
  }
  pthread_mutex_unlock (&obj->ob_mtx);
  ld->ld_totalobjectnb++;
  RPS_DEBUG_PRINTF (LOAD, "done load&fill object#%ld %s space#%d\n",
		    ld->ld_totalobjectnb, obidbuf, spix);
}				/* end rps_loader_fill_object_second_pass */



void
rps_load_second_pass (RpsLoader_t * ld, int spix, RpsOid spaceid)
{
  char spacebuf[32];
  char filepath[256];
  memset (spacebuf, 0, sizeof (spacebuf));
  memset (filepath, 0, sizeof (filepath));
  RPS_ASSERT (rps_is_valid_loader (ld));
  rps_oid_to_cbuf (spaceid, spacebuf);
  snprintf (filepath, sizeof (filepath), "%s/persistore/sp%s-rps.json",
	    rps_load_directory, spacebuf);
  FILE *spfil = fopen (filepath, "r");
  if (!spfil)
    RPS_FATAL ("failed to open %s for space #%d : %m", filepath, spix);
  rps_check_all_objects_buckets_are_valid ();
  ld->ld_state = RPSLOADING_FILL_OBJECTS_PASS;
  RpsObject_t *obspac = rps_find_object_by_oid (spaceid);
  RPS_ASSERT (obspac != NULL);
  long linoff = 0;
  int lincnt = 0;
  size_t linsz = 256;
  char *linbuf = RPS_ALLOC_ZEROED (linsz);
  do
    {
      memset (linbuf, 0, sizeof (linbuf));
      linoff = ftell (spfil);
      if (!fgets (linbuf, linsz, spfil))
	break;
      lincnt++;
      if (linbuf[0] == '/' || isspace (linbuf[0]))
	continue;
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
  lincnt += json_object_size (jsprologue);
  json_decref (jsprologue), jsprologue = NULL,
    jsnbobjects = NULL, jsspaceid = NULL;
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
      if (objcount % 8 == 0)
	{
	  rps_check_all_objects_buckets_are_valid ();
	  if (objcount % 16 == 0)
	    {
#warning temporary call to mallopt. Should be removed once loading and dumping completes.
	      mallopt (M_CHECK_ACTION, 03);
	    }
	  // if (objcount % 16 == 0)
	  //  printf
	  //    ("rps_load_first_pass space#%d objcount %ld file %s:%d (%s:%d)\n",
	  //     spix, objcount, filepath, lincnt, __FILE__, __LINE__);
	};
      memset (linbuf, 0, linsz);
      linoff = ftell (spfil);
      if (!fgets (linbuf, linsz, spfil))
	break;
      lincnt++;
      if (lincnt % 16 == 0)
	rps_check_all_objects_buckets_are_valid ();
      if (isspace (linbuf[0]))
	continue;
      RpsOid curobid = RPS_OID_NULL;
      {
	int endcol = -1;
	char obidbuf[32];
	memset (obidbuf, 0, sizeof (obidbuf));
	/// should test for lines starting objects, i.e. //+ob.... then
	/// fetch all the lines in some buffer, etc...
	if (sscanf (linbuf, "//+ob_%18[0-9A-Za-z]", obidbuf + 1) >= 1)
	  {
	    obidbuf[0] = '_';
	    curobid = rps_cstr_to_oid (obidbuf, NULL);
	    mallopt (M_CHECK_ACTION, 03);
	  }
	if (rps_oid_is_valid (curobid))
	  {
	    char endlin[48];
	    memset (endlin, 0, sizeof (endlin));
	    snprintf (endlin, sizeof (endlin), "//-ob%s\n", obidbuf);
	    rps_check_all_objects_buckets_are_valid ();
	    size_t bufsz = 256;
	    char *bufjs = RPS_ALLOC_ZEROED (bufsz);
	    long startlin = lincnt;
	    FILE *obstream = open_memstream (&bufjs, &bufsz);
	    RPS_ASSERT (obstream);
	    while ((linbuf[0] = (char) 0),
		   getline (&linbuf, &linsz, spfil) > 0)
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
	    RpsObject_t *curob = rps_find_object_by_oid (curobid);
	    RPS_ASSERT (curob != NULL);
	    RPS_DEBUG_NLPRINTF (LOAD,
				"before ldfillobj2ndpass obidbuf=%s lincnt=%d",
				obidbuf, lincnt);
	    // TEMPORARY: to debug make testdump failure in commit b25fd8051caba9
	    // for object _7oa7eIzzcxv03TmmZH
	    if (obidbuf[1] == '7' && obidbuf[2] == 'o' && obidbuf[3] == 'a')
	      {
		RPS_DEBUG_PRINTF (LOAD,
				  "**bug in commit b25fd8051 for obidbuf %s curob@%p id/hi=%lld,lo=%lld",
				  obidbuf, curob,
				  curob->ob_id.id_hi, curob->ob_id.id_lo);
		usleep (1000);
	      }
	    // for object _5KVs9kaMYiU040KwVj
	    if (obidbuf[1] == '5' && obidbuf[2] == 'K' && obidbuf[3] == 'V')
	      {
		RPS_DEBUG_PRINTF (LOAD,
				  "**bug in commit 3746f439c9f for obidbuf %s curob@%p id/hi=%lld,lo=%lld",
				  obidbuf, curob,
				  curob->ob_id.id_hi, curob->ob_id.id_lo);
		usleep (1000);
	      }
	    // for object _6NVM7sMcITg01ug5TC
	    if (obidbuf[1] == '6' && obidbuf[2] == 'N' && obidbuf[3] == 'V')
	      {
		RPS_DEBUG_PRINTF (LOAD,
				  "**bug in commit 4ca7a21e083 for obidbuf %s curob@%p id/hi=%lld,lo=%lld",
				  obidbuf, curob,
				  curob->ob_id.id_hi,  curob->ob_id.id_lo);
		usleep (1000);
	      }
	    /// ENDTEMPORARYCHECKS
	    rps_loader_fill_object_second_pass (ld, spix, curob, jsobject,
						obspac);
	    RPS_DEBUG_PRINTF (LOAD,
			      "after ldfillobj2ndpass obidbuf=%s lincnt=%d curob=%-1O\n",
			      obidbuf, lincnt, curob);
	    objcount++;
	    json_decref (jsobject);
	    fclose (obstream);
	    free (bufjs), bufjs = NULL;
	    bufsz = 0;
	  }
	else
	  RPS_FATAL ("in %s:%d invalid oid %s", filepath, lincnt, obidbuf);
      }
    }
}				/* end rps_load_second_pass */


void
rps_load_install_global_root_objects (RpsLoader_t * ld)
{
  RPS_ASSERT (rps_is_valid_loader (ld)
	      && ld->ld_state == RPSLOADING_EPILOGUE_PASS);
  json_t *jsglobroots = json_object_get (ld->ld_json_manifest, "globalroots");
  if (!jsglobroots || !json_is_array (jsglobroots))
    RPS_FATAL ("bad globalroots in JSON manifest...\n %s",
	       json_dumps (ld->ld_json_manifest,
			   JSON_INDENT (2) | JSON_SORT_KEYS));
  int nbglob = json_array_size (jsglobroots);
  for (int ix = 0; ix < nbglob; ix++)
    {
      json_t *jscurglob = json_array_get (jsglobroots, ix);
      RpsObject_t *globob = rps_loader_json_to_object (ld, jscurglob);
      if (!globob)
	RPS_FATAL ("unknown globalroot #%d\n... for %s", ix,
		   json_dumps (jscurglob, JSON_INDENT (2) | JSON_SORT_KEYS));
      rps_add_global_root_object (globob);
    }
  printf ("installed %d global root objects [%s:%d]\n",
	  nbglob, __FILE__, __LINE__);
}				/* end rps_load_install_global_objects */


/************************ end of file load_rps.c *****************/

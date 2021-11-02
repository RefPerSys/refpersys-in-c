#ifndef __REFPERSYS_INCLUDE_OID_RPS_H_INCLUDED__
#define __REFPERSYS_INCLUDE_OID_RPS_H_INCLUDED__


typedef struct _RpsOid
{
  uint64_t id_hi, id_lo;
} RpsOid;

#define RPS_B62DIGITS \
  "0123456789"				  \
  "abcdefghijklmnopqrstuvwxyz"		  \
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

#define RPS_NULL_OID ((RpsOid){id_hi:0, id_lo:0})
#define RPS_OIDBUFLEN 24
#define RPS_OIDBASE (sizeof(RPS_B62DIGITS)-1)
#define RPS_MIN_OID_HI (62*62*62)
#define RPS_MAX_OID_HI /* 8392993658683402240, about 8.392994e+18 */ \
  ((uint64_t)10 * 62 * (62 * 62 * 62) * (62 * 62 * 62) * (62 * 62 * 62))
#define RPS_NBDIGITS_OID_HI 11
#define RPS_DELTA_OID_HI (RPS_MAX_OID_HI - RPS_MIN_OID_HI)
#define RPS_MIN_OID_LO (62*62*62)
#define RPS_MAX_OID_LO /* about 3.52161e+12 */ \
  ((uint64_t)62 * (62L * 62 * 62) * (62 * 62 * 62))
#define RPS_DELTA_OID_LO (RPS_MAX_OID_LO - RPS_MIN_OID_LO)
#define RPS_NBDIGITS_OID_LO 8
#define RPS_OID_NBCHARS (RPS_NBDIGITS_OID_HI+RPS_NBDIGITS_OID_LO+1)
#define RPS_OID_MAXBUCKETS (10*62)


extern bool rps_oid_is_null (const RpsOid oid);
extern bool rps_oid_is_valid (const RpsOid oid);
extern bool rps_oid_equal (const RpsOid oid1, const RpsOid oid2);
extern bool rps_oid_less_than (const RpsOid oid1, const RpsOid oid2);
extern bool rps_oid_less_equal (const RpsOid oid1, const RpsOid oid2);
extern int rps_oid_cmp (const RpsOid oid1, const RpsOid oid2);
extern void rps_oid_to_cbuf (const RpsOid oid, char cbuf[RPS_OIDBUFLEN]);
extern RpsOid rps_cstr_to_oid (const char *cstr, const char **pend);
extern unsigned rps_oid_bucket_num (const RpsOid oid);
extern RpsHash_t rps_oid_hash (const RpsOid oid);

// compute a random and valid oid
extern RpsOid rps_random_valid_oid (void);

#endif /* __REFPERSYS_INCLUDE_OID_RPS_H_INCLUDED__ */


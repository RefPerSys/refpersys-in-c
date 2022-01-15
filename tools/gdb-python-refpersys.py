# file refpersys-in-c/tools/gdb-python-refpersys.py
# see sourceware.org/gdb/current/onlinedocs/gdb/Writing-a-Pretty_002dPrinter.html

## pretty printing of RpsOid

global RPS_B62DIGITS = \
  "0123456789"				  \
  "abcdefghijklmnopqrstuvwxyz"		  \
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

class RpsOid_pprinter(oid):
    "pretty-prints some RpsOid"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        idhi = self.val["id_hi"]
        idlo = self.val["id_lo"]
        if (idhi == 0 && idlo == 0):
            return "__"
        res = "*******************"
        
## pretty printing of RpsObject_t*
class RpsObjectPtr_pprinter(obptr):
    "pretty-prints some RpsObject_t* pointer"

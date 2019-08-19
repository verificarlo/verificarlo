/* Simplified test case from OpenBlas */
#include <math.h>

#define FLOAT double
#define ONE 1.0
#define ZERO 0.0

void CNAME(FLOAT *DA, FLOAT *DB, FLOAT *C, FLOAT *S){
  long double da = *DA;
  long double db = *DB;
  long double c;
  long double s;
  long double r, roe, z;

  long double ada = fabsl(da);
  long double adb = fabsl(db);
  long double scale = ada + adb;

  roe = db;
  if (ada > adb) roe = da;

  if (scale == ZERO) {
    *C = ONE;
    *S = ZERO;
    *DA = ZERO;
    *DB = ZERO;
  } else {
    r = sqrt(da * da + db * db);
    if (roe < 0) r = -r;
    c = da / r;
    s = db / r;
    z = ONE;
    if (da != ZERO) {
      if (ada > adb){
	z = s;
      } else {
	z = ONE / c;
      }
    }

    *C = c;
    *S = s;
    *DA = r;
    *DB = z;
  }
}


#pragma once

#include <stdarg.h>
#include <stdlib.h>

#include "interflop/hashmap/vfc_hashmap.h"
#include "interflop/interflop.h"

#define MAX_BACKENDS 16

typedef struct interflop_backend_interface_t (*interflop_init_t)(void *context);

/* --- Global backend state ------------------------------------------------ */
extern struct interflop_backend_interface_t backends[MAX_BACKENDS];
extern void *contexts[MAX_BACKENDS];
extern unsigned char loaded_backends;

/* --- Delta-debug state --------------------------------------------------- */
extern unsigned char ddebug_enabled;
extern char *dd_exclude_path;
extern char *dd_include_path;
extern char *dd_generate_path;
extern vfc_hashmap_t dd_must_instrument;
extern vfc_hashmap_t dd_mustnot_instrument;

/* --- Scalar backend dispatch functions ----------------------------------- */
float _floatadd(float a, float b);
float _floatsub(float a, float b);
float _floatmul(float a, float b);
float _floatdiv(float a, float b);

double _doubleadd(double a, double b);
double _doublesub(double a, double b);
double _doublemul(double a, double b);
double _doublediv(double a, double b);

int _floatcmp(enum FCMP_PREDICATE p, float a, float b);
int _doublecmp(enum FCMP_PREDICATE p, double a, double b);

float _floatfma(float a, float b, float c);
double _doublefma(double a, double b, double c);

float _doubletofloatcast(double a);

void interflop_call(interflop_call_id id, ...);

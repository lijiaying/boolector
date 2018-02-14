/*  Boolector: Satisfiability Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2014-2018 Aina Niemetz.
 *  Copyright (C) 2014-2017 Mathias Preiner.
 *  Copyright (C) 2014-2015 Armin Biere.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifndef BTOROPTS_H_INCLUDED
#define BTOROPTS_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "btortypes.h"
#include "utils/btorhashptr.h"
#include "utils/btormem.h"

/*------------------------------------------------------------------------*/

struct BtorOpt
{
  bool internal;    /* internal option? */
  bool isflag;      /* flag? */
  const char *shrt; /* short option identifier (may be 0) */
  const char *lng;  /* long option identifier */
  const char *desc; /* description */
  uint32_t val;     /* value */
  uint32_t dflt;    /* default value */
  uint32_t min;     /* min value */
  uint32_t max;     /* max value */
  char *valstr;     /* optional option string value */
};

typedef struct BtorOpt BtorOpt;

/*------------------------------------------------------------------------*/

/* enum BtorOption is in btortypes.h */

/*------------------------------------------------------------------------*/

#define BTOR_VERBOSITY_MAX 4

#define BTOR_PROB_MAX 1000

enum BtorOptSatEngine
{
  BTOR_SAT_ENGINE_MIN,
  BTOR_SAT_ENGINE_LINGELING,
  BTOR_SAT_ENGINE_PICOSAT,
  BTOR_SAT_ENGINE_MINISAT,
  BTOR_SAT_ENGINE_CADICAL,
  BTOR_SAT_ENGINE_MAX,
};
#define BTOR_SAT_ENGINE_DFLT (BTOR_SAT_ENGINE_MIN + 1)
typedef enum BtorOptSatEngine BtorOptSatEngine;

enum BtorOptEngine
{
  BTOR_ENGINE_MIN,
  BTOR_ENGINE_FUN,
  BTOR_ENGINE_SLS,
  BTOR_ENGINE_PROP,
  BTOR_ENGINE_AIGPROP,
  BTOR_ENGINE_QUANT,
  BTOR_ENGINE_MAX,
};
#define BTOR_ENGINE_DFLT BTOR_ENGINE_FUN
typedef enum BtorOptEngine BtorOptEngine;

enum BtorOptInputFormat
{
  BTOR_INPUT_FORMAT_MIN,
  BTOR_INPUT_FORMAT_NONE,
  BTOR_INPUT_FORMAT_BTOR,
  BTOR_INPUT_FORMAT_SMT1,
  BTOR_INPUT_FORMAT_SMT2,
  BTOR_INPUT_FORMAT_MAX,
};
#define BTOR_INPUT_FORMAT_DFLT BTOR_INPUT_FORMAT_NONE
typedef enum BtorOptInputFormat BtorOptInputFormat;

enum BtorOptOutputBase
{
  BTOR_OUTPUT_BASE_MIN,
  BTOR_OUTPUT_BASE_BIN,
  BTOR_OUTPUT_BASE_HEX,
  BTOR_OUTPUT_BASE_DEC,
  BTOR_OUTPUT_BASE_MAX,
};
#define BTOR_OUTPUT_BASE_DFLT BTOR_OUTPUT_BASE_BIN
typedef enum BtorOptOutputBase BtorOptOutputBase;

enum BtorOptOutputFormat
{
  BTOR_OUTPUT_FORMAT_MIN,
  BTOR_OUTPUT_FORMAT_BTOR,
  BTOR_OUTPUT_FORMAT_BTOR2,
  BTOR_OUTPUT_FORMAT_SMT2,
  BTOR_OUTPUT_FORMAT_AIGER_ASCII,
  BTOR_OUTPUT_FORMAT_AIGER_BINARY,
  BTOR_OUTPUT_FORMAT_MAX,
};
#define BTOR_OUTPUT_FORMAT_DFLT BTOR_OUTPUT_FORMAT_BTOR
typedef enum BtorOptOutputFormat BtorOptOutputFormat;

enum BtorOptDPQsort
{
  BTOR_DP_QSORT_MIN,
  BTOR_DP_QSORT_JUST,
  BTOR_DP_QSORT_ASC,
  BTOR_DP_QSORT_DESC,
  BTOR_DP_QSORT_MAX,
};
#define BTOR_DP_QSORT_DFLT BTOR_DP_QSORT_JUST
typedef enum BtorOptDPQsort BtorOptDPQsort;

enum BtorOptJustHeur
{
  BTOR_JUST_HEUR_MIN,
  BTOR_JUST_HEUR_LEFT,
  BTOR_JUST_HEUR_BRANCH_MIN_APP,
  BTOR_JUST_HEUR_BRANCH_MIN_DEP,
  BTOR_JUST_HEUR_MAX,
};
#define BTOR_JUST_HEUR_DFLT BTOR_JUST_HEUR_BRANCH_MIN_APP
typedef enum BtorOptJustHeur BtorOptJustHeur;

enum BtorOptSLSStrat
{
  BTOR_SLS_STRAT_MIN,
  BTOR_SLS_STRAT_BEST_MOVE,
  BTOR_SLS_STRAT_RAND_WALK,
  BTOR_SLS_STRAT_FIRST_BEST_MOVE,
  BTOR_SLS_STRAT_BEST_SAME_MOVE,
  BTOR_SLS_STRAT_ALWAYS_PROP,
  BTOR_SLS_STRAT_MAX,
};
#define BTOR_SLS_STRAT_DFLT BTOR_SLS_STRAT_BEST_MOVE
typedef enum BtorOptSLSStrat BtorOptSLSStrat;

enum BtorOptPropPathSel
{
  BTOR_PROP_PATH_SEL_MIN,
  BTOR_PROP_PATH_SEL_CONTROLLING,
  BTOR_PROP_PATH_SEL_ESSENTIAL,
  BTOR_PROP_PATH_SEL_RANDOM,
  BTOR_PROP_PATH_SEL_MAX,
};
#define BTOR_PROP_PATH_SEL_DFLT BTOR_PROP_PATH_SEL_ESSENTIAL
typedef enum BtorOptPropPathSel BtorOptPropPathSel;

enum BtorOptQuantSynth
{
  BTOR_QUANT_SYNTH_MIN,
  BTOR_QUANT_SYNTH_NONE,
  BTOR_QUANT_SYNTH_EL,
  BTOR_QUANT_SYNTH_ELMC,
  BTOR_QUANT_SYNTH_EL_ELMC,
  BTOR_QUANT_SYNTH_ELMR,
  BTOR_QUANT_SYNTH_MAX,
};
#define BTOR_QUANT_SYNTH_DFLT BTOR_QUANT_SYNTH_ELMR
typedef enum BtorOptQuantSynth BtorOptQuantSynt;

enum BtorOptFunEagerLemmas
{
  BTOR_FUN_EAGER_LEMMAS_MIN,
  BTOR_FUN_EAGER_LEMMAS_NONE,
  BTOR_FUN_EAGER_LEMMAS_CONF,
  BTOR_FUN_EAGER_LEMMAS_ALL,
  BTOR_FUN_EAGER_LEMMAS_MAX,
};
#define BTOR_FUN_EAGER_LEMMAS_DFLT BTOR_FUN_EAGER_LEMMAS_CONF
typedef enum BtorOptFunEagerLemmas BtorOptFunEagerLemmas;

/*------------------------------------------------------------------------*/

void btor_opt_init_opts (Btor *btor);
void btor_opt_clone_opts (Btor *btor, Btor *clone);
void btor_opt_delete_opts (Btor *btor);

bool btor_opt_is_valid (Btor *btor, const BtorOption opt);

uint32_t btor_opt_get (Btor *btor, const BtorOption opt);
uint32_t btor_opt_get_min (Btor *btor, const BtorOption opt);
uint32_t btor_opt_get_max (Btor *btor, const BtorOption opt);
uint32_t btor_opt_get_dflt (Btor *btor, const BtorOption opt);
const char *btor_opt_get_lng (Btor *btor, const BtorOption opt);
const char *btor_opt_get_shrt (Btor *btor, const BtorOption opt);
const char *btor_opt_get_desc (Btor *btor, const BtorOption opt);
const char *btor_opt_get_valstr (Btor *btor, const BtorOption opt);

void btor_opt_set (Btor *btor, const BtorOption opt, uint32_t val);
void btor_opt_set_str (Btor *btor, const BtorOption opt, const char *str);

BtorOption btor_opt_first (Btor *btor);
BtorOption btor_opt_next (Btor *btor, BtorOption cur);

#ifndef NBTORLOG
void btor_opt_log_opts (Btor *btor);
#endif
#endif

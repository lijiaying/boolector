/*  Boolector: Satisfiability Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2007-2009 Robert Daniel Brummayer.
 *  Copyright (C) 2007-2016 Armin Biere.
 *  Copyright (C) 2012-2018 Aina Niemetz.
 *  Copyright (C) 2012-2016 Mathias Preiner.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#include "btormain.h"
#include "boolector.h"
#include "btorconfig.h"
#include "btorcore.h"
#include "btorexit.h"
#include "btoropt.h"
#include "btorparse.h"
#include "utils/btorhashptr.h"
#include "utils/btormem.h"
#include "utils/btoroptparse.h"
#include "utils/btorstack.h"
#include "utils/btorutil.h"

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct BtorMainApp BtorMainApp;
static BtorMainApp *g_app;

bool g_dual_threads;
static double g_start_time_real;
static uint32_t g_verbosity;
static uint32_t g_set_alarm;
static bool g_caught_sig;

static void (*sig_int_handler) (int32_t);
static void (*sig_segv_handler) (int32_t);
static void (*sig_abrt_handler) (int32_t);
static void (*sig_term_handler) (int32_t);
static void (*sig_bus_handler) (int32_t);

static void (*sig_alrm_handler) (int32_t);

/*------------------------------------------------------------------------*/

enum BtorMainOption
{
  BTORMAIN_OPT_HELP,
  BTORMAIN_OPT_COPYRIGHT,
  BTORMAIN_OPT_VERSION,
  BTORMAIN_OPT_TIME,
  BTORMAIN_OPT_OUTPUT,
  BTORMAIN_OPT_ENGINE,
  BTORMAIN_OPT_SAT_ENGINE,
  BTORMAIN_OPT_LGL_NOFORK,
  BTORMAIN_OPT_HEX,
  BTORMAIN_OPT_DEC,
  BTORMAIN_OPT_BIN,
  BTORMAIN_OPT_BTOR,
  BTORMAIN_OPT_SMT2,
  BTORMAIN_OPT_SMT1,
  BTORMAIN_OPT_DUMP_BTOR,
#if 0
  BTORMAIN_OPT_DUMP_BTOR2,
#endif
  BTORMAIN_OPT_DUMP_SMT,
  BTORMAIN_OPT_DUMP_AAG,
  BTORMAIN_OPT_DUMP_AIG,
  BTORMAIN_OPT_DUMP_AIGER_MERGE,
  BTORMAIN_OPT_SMT2_MODEL,
  /* this MUST be the last entry! */
  BTORMAIN_OPT_NUM_OPTS,
};
typedef enum BtorMainOption BtorMainOption;

typedef struct BtorMainOpt
{
  bool general;       /* general option? */
  const char *shrt;   /* short option identifier (may be 0) */
  const char *lng;    /* long option identifier */
  const char *desc;   /* description */
  uint32_t val;       /* value */
  uint32_t dflt;      /* default value */
  uint32_t min;       /* min value */
  uint32_t max;       /* max value */
  bool candisable;    /* can be disabled via '-(-)no-XX'? */
  bool isflag;        /* is option flag? */
  BtoArgExpected arg; /* expects argument? */
} BtorMainOpt;

/*------------------------------------------------------------------------*/

struct BtorMainApp
{
  Btor *btor;
  BtorMemMgr *mm;
  BtorMainOpt *options;
  bool done;
  int32_t err;
  char *infile_name;
  FILE *infile;
  int32_t close_infile;
  FILE *outfile;
  char *outfile_name;
  bool close_outfile;
};

/*------------------------------------------------------------------------*/

static void
btormain_init_opt (BtorMainApp *app,
                   BtorMainOption opt,
                   bool general,
                   bool isflag,
                   char *lng,
                   char *shrt,
                   uint32_t val,
                   uint32_t min,
                   uint32_t max,
                   bool candisable,
                   BtoArgExpected arg,
                   char *desc)
{
  assert (app);
  assert (lng);
  assert (desc);
  assert (max <= UINT_MAX);
  assert (min <= val);
  assert (val <= max);

  app->options[opt].general    = general;
  app->options[opt].lng        = lng;
  app->options[opt].shrt       = shrt;
  app->options[opt].val        = val;
  app->options[opt].dflt       = val;
  app->options[opt].min        = min;
  app->options[opt].max        = max;
  app->options[opt].desc       = desc;
  app->options[opt].candisable = candisable;
  app->options[opt].isflag     = isflag;
  app->options[opt].arg        = arg;
}

static void
btormain_init_opts (BtorMainApp *app)
{
  assert (app);

  BTOR_CNEWN (app->mm, app->options, BTORMAIN_OPT_NUM_OPTS);

  btormain_init_opt (app,
                     BTORMAIN_OPT_HELP,
                     true,
                     true,
                     "help",
                     "h",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "print this message and exit");
  btormain_init_opt (app,
                     BTORMAIN_OPT_COPYRIGHT,
                     true,
                     true,
                     "copyright",
                     "c",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "print copyright and exit");
  btormain_init_opt (app,
                     BTORMAIN_OPT_VERSION,
                     true,
                     true,
                     "version",
                     "V",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "print version and exit");
  btormain_init_opt (app,
                     BTORMAIN_OPT_TIME,
                     true,
                     false,
                     "time",
                     "t",
                     0,
                     0,
                     UINT_MAX,
                     false,
                     BTOR_ARG_EXPECT_INT,
                     "set time limit");
  btormain_init_opt (app,
                     BTORMAIN_OPT_OUTPUT,
                     true,
                     false,
                     "output",
                     "o",
                     0,
                     0,
                     0,
                     false,
                     BTOR_ARG_EXPECT_STR,
                     "set output file for dumping");
  btormain_init_opt (
      app,
      BTORMAIN_OPT_ENGINE,
      true,
      false,
      (char *) boolector_get_opt_lng (app->btor, BTOR_OPT_ENGINE),
      (char *) boolector_get_opt_shrt (app->btor, BTOR_OPT_ENGINE),
      BTOR_ENGINE_DFLT,
      BTOR_ENGINE_MIN,
      BTOR_ENGINE_MAX,
      false,
      BTOR_ARG_EXPECT_STR,
      "set engine (core sls prop aigprop) [core]");
  btormain_init_opt (
      app,
      BTORMAIN_OPT_SAT_ENGINE,
      true,
      false,
      (char *) boolector_get_opt_lng (app->btor, BTOR_OPT_SAT_ENGINE),
      (char *) boolector_get_opt_shrt (app->btor, BTOR_OPT_SAT_ENGINE),
      BTOR_SAT_ENGINE_DFLT,
      BTOR_SAT_ENGINE_MIN + 1,
      BTOR_SAT_ENGINE_MAX - 1,
      false,
      BTOR_ARG_EXPECT_STR,
      "set sat solver");
#ifdef BTOR_USE_LINGELING
  btormain_init_opt (app,
                     BTORMAIN_OPT_LGL_NOFORK,
                     true,
                     true,
                     "lingeling-nofork",
                     0,
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "do not use 'fork/clone' for Lingeling");
#endif
  btormain_init_opt (app,
                     BTORMAIN_OPT_HEX,
                     true,
                     true,
                     "hex",
                     "x",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "force hexadecimal number output");
  btormain_init_opt (app,
                     BTORMAIN_OPT_DEC,
                     true,
                     true,
                     "dec",
                     "d",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "force decimal number output");
  btormain_init_opt (app,
                     BTORMAIN_OPT_BIN,
                     true,
                     true,
                     "bin",
                     "b",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "force binary number output");
  btormain_init_opt (app,
                     BTORMAIN_OPT_BTOR,
                     true,
                     true,
                     "btor",
                     0,
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "force BTOR input format");
  btormain_init_opt (app,
                     BTORMAIN_OPT_SMT2,
                     true,
                     true,
                     "smt2",
                     0,
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "force SMT-LIB v2 input format");
  btormain_init_opt (app,
                     BTORMAIN_OPT_SMT1,
                     true,
                     true,
                     "smt1",
                     0,
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "force SMT-LIB v1 input format");
  btormain_init_opt (app,
                     BTORMAIN_OPT_DUMP_BTOR,
                     true,
                     true,
                     "dump-btor",
                     "db",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "dump formula in BTOR format");
#if 0
  btormain_init_opt (app, BTORMAIN_OPT_DUMP_BTOR2, true, true,
                     "dump-btor2", "db2", 0, 0, 1,
                     false, BTOR_ARG_EXPECT_NONE,
                     "dump formula in BTOR 2.0 format");
#endif
  btormain_init_opt (app,
                     BTORMAIN_OPT_DUMP_SMT,
                     true,
                     true,
                     "dump-smt",
                     "ds",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "dump formula in SMT-LIB v2 format");
  btormain_init_opt (app,
                     BTORMAIN_OPT_DUMP_AAG,
                     true,
                     true,
                     "dump-aag",
                     "daa",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "dump QF_BV formula in ascii AIGER format");
  btormain_init_opt (app,
                     BTORMAIN_OPT_DUMP_AIG,
                     true,
                     true,
                     "dump-aig",
                     "dai",
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "dump QF_BV formula in binary AIGER format");
  btormain_init_opt (app,
                     BTORMAIN_OPT_DUMP_AIGER_MERGE,
                     true,
                     true,
                     "dump-aiger-merge",
                     "dam",
                     0,
                     0,
                     1,
                     true,
                     BTOR_ARG_EXPECT_NONE,
                     "merge all roots of AIG [0]");
  btormain_init_opt (app,
                     BTORMAIN_OPT_SMT2_MODEL,
                     false,
                     true,
                     "smt2-model",
                     0,
                     0,
                     0,
                     1,
                     false,
                     BTOR_ARG_EXPECT_NONE,
                     "print model in SMT-LIB v2 format "
                     "if model generation is enabled");
}

static bool
btormain_opt_has_str_arg (const char *opt)
{
  assert (opt);

  BtorMainOption mopt;
  BtorMainOpt *mo;

  for (mopt = 0; mopt < BTORMAIN_OPT_NUM_OPTS; mopt++)
  {
    mo = &g_app->options[mopt];
    if ((mo->shrt && strcmp (mo->shrt, opt) == 0) || strcmp (mo->lng, opt) == 0)
    {
      return g_app->options[mopt].arg == BTOR_ARG_EXPECT_STR;
    }
  }
  return false;
}

/*------------------------------------------------------------------------*/

static BtorMainApp *
btormain_new_btormain (Btor *btor)
{
  assert (btor);

  BtorMainApp *res;
  BtorMemMgr *mm;

  mm = btor_mem_mgr_new ();
  BTOR_CNEWN (mm, res, 1);
  res->mm          = mm;
  res->btor        = btor;
  res->infile      = stdin;
  res->infile_name = "<stdin>";
  res->outfile     = stdout;
  btormain_init_opts (res);
  return res;
}

static void
btormain_delete_btormain (BtorMainApp *app)
{
  assert (app);

  BtorMemMgr *mm;

  mm = app->mm;
  BTOR_DELETEN (mm, app->options, BTORMAIN_OPT_NUM_OPTS);
  boolector_delete (app->btor);
  BTOR_DELETE (mm, app);
  btor_mem_mgr_delete (mm);
}

/*------------------------------------------------------------------------*/

static void
btormain_error (BtorMainApp *app, char *msg, ...)
{
  assert (app);

  va_list list;
  va_start (list, msg);
  fputs ("boolector: ", stderr);
  vfprintf (stderr, msg, list);
  fprintf (stderr, "\n");
  va_end (list);
  app->err = BTOR_ERR_EXIT;
}

static void
btormain_msg (char *msg, ...)
{
  assert (msg);

  va_list list;
  va_start (list, msg);
  fprintf (stdout, "[btor>main] ");
  vfprintf (stdout, msg, list);
  fprintf (stdout, "\n");
  va_end (list);
}

/*------------------------------------------------------------------------*/

#define LEN_OPTSTR 38
#define LEN_PARAMSTR 16
#define LEN_HELPSTR 80

#define IS_OPT(optlng, lng) (!strcmp (optlng, lng))

static void
print_opt (BtorMainApp *app,
           const char *lng,
           const char *shrt,
           bool isflag,
           uint32_t dflt,
           const char *desc,
           bool print_dflt)
{
  assert (app);
  assert (lng);
  assert (desc);

  char optstr[LEN_OPTSTR], paramstr[LEN_PARAMSTR];
  char *descstr, descstrline[LEN_HELPSTR], *word;
  int32_t i, j, len;
  BtorCharPtrStack words;

  if (!strcmp (lng, "time"))
    sprintf (paramstr, "<seconds>");
  else if (!strcmp (lng, "output"))
    sprintf (paramstr, "<file>");
  else if (!strcmp (lng, boolector_get_opt_lng (app->btor, BTOR_OPT_ENGINE))
           || !strcmp (lng,
                       boolector_get_opt_lng (app->btor, BTOR_OPT_SAT_ENGINE)))
    sprintf (paramstr, "<engine>");
  else if (!isflag)
    sprintf (paramstr, "<n>");
  else
    paramstr[0] = '\0';

  /* option string ------------------------------------------ */
  memset (optstr, ' ', LEN_OPTSTR * sizeof (char));
  optstr[LEN_OPTSTR - 1] = '\0';
  len                    = strlen (lng);
  sprintf (optstr,
           "  %s%s%s%s%s--%s%s%s",
           shrt ? "-" : "",
           shrt ? shrt : "",
           shrt && strlen (paramstr) > 0 ? " " : "",
           shrt ? paramstr : "",
           shrt ? ", " : "",
           lng,
           strlen (paramstr) > 0 ? "=" : "",
           paramstr);
  len = strlen (optstr);
  for (i = len; i < LEN_OPTSTR - 1; i++) optstr[i] = ' ';
  optstr[LEN_OPTSTR - 1] = '\0';

  /* formatted description ---------------------------------- */
  /* append default value to description */
  if (print_dflt)
  {
    len = strlen (desc) + 3 + btor_util_num_digits (dflt);
    BTOR_CNEWN (app->mm, descstr, len + 1);
    sprintf (descstr, "%s [%u]", desc, dflt);
  }
  else
  {
    len = strlen (desc);
    BTOR_CNEWN (app->mm, descstr, len + 1);
    sprintf (descstr, "%s", desc);
  }
  BTOR_INIT_STACK (app->mm, words);
  word = strtok (descstr, " ");
  while (word)
  {
    BTOR_PUSH_STACK (words, btor_mem_strdup (app->mm, word));
    word = strtok (0, " ");
  }
  BTOR_DELETEN (app->mm, descstr, len + 1);

  BTOR_CLRN (descstrline, LEN_HELPSTR);
  sprintf (descstrline, "%s ", optstr);
  i = 0;
  do
  {
    j = LEN_OPTSTR;
    for (; i < BTOR_COUNT_STACK (words); i++)
    {
      word = BTOR_PEEK_STACK (words, i);
      len  = strlen (word);

      /* word does not fit into remaining line */
      if (j + 1 + len >= LEN_HELPSTR) break;

      strcpy (descstrline + j, word);
      j += len;
      descstrline[j++] = ' ';
    }
    descstrline[j] = 0;
    fprintf (app->outfile, "%s\n", descstrline);
    BTOR_CLRN (descstrline, LEN_HELPSTR);
    memset (descstrline, ' ', LEN_OPTSTR * sizeof (char));
  } while (i < BTOR_COUNT_STACK (words));

  /* cleanup */
  while (!BTOR_EMPTY_STACK (words))
    btor_mem_freestr (app->mm, BTOR_POP_STACK (words));
  BTOR_RELEASE_STACK (words);
}

#define PRINT_MAIN_OPT(app, opt) \
  do                             \
  {                              \
    print_opt (app,              \
               (opt)->lng,       \
               (opt)->shrt,      \
               (opt)->isflag,    \
               (opt)->dflt,      \
               (opt)->desc,      \
               false);           \
  } while (0)

#define BOOLECTOR_OPTS_INFO_MSG                                                \
  "The following options can also be used in the form '-<short name>=<int>'"   \
  "\n"                                                                         \
  "and '--<long name>=<int>'. Flags are disabled with 0 and enabled with a "   \
  "pos."                                                                       \
  "\n"                                                                         \
  "integer. Alternatively, use '-no-<short name>' and '--no-<long name>' for"  \
  "\n"                                                                         \
  "disabling, and '-<short name>' and '--<long name>' for enabling flags."     \
  "\n"                                                                         \
  "Note that all of the following options can also be set via env. variables " \
  "of"                                                                         \
  "\n"                                                                         \
  "the form 'BTOR<capitalized long name without '-' and ':'>=<int>'."          \
  "\n\n"

static void
print_help (BtorMainApp *app)
{
  assert (app);

  BtorOption o;
  BtorMainOption mo;
  FILE *out;

  out = app->outfile;

  fprintf (out, "usage: boolector [<option>...][<input>]\n");
  fprintf (out, "\n");
  fprintf (out, "where <option> is one of the following:\n");
  fprintf (out, "\n");

  for (mo = 0; mo < BTORMAIN_OPT_NUM_OPTS; mo++)
  {
    if (!app->options[mo].general) continue;
    if (mo == BTORMAIN_OPT_TIME || mo == BTORMAIN_OPT_ENGINE
        || mo == BTORMAIN_OPT_HEX || mo == BTORMAIN_OPT_BTOR
        || mo == BTORMAIN_OPT_DUMP_BTOR)
      fprintf (out, "\n");
    PRINT_MAIN_OPT (app, &app->options[mo]);
  }

  fprintf (out, "\n");

  for (mo = 0; mo < BTORMAIN_OPT_NUM_OPTS; mo++)
  {
    if (app->options[mo].general) continue;
    PRINT_MAIN_OPT (app, &app->options[mo]);
    if (mo == BTORMAIN_OPT_SMT2_MODEL) fprintf (out, "\n");
  }

  fprintf (out, "\n");

  fprintf (out, BOOLECTOR_OPTS_INFO_MSG);

  for (o = boolector_first_opt (app->btor); boolector_has_opt (app->btor, o);
       o = boolector_next_opt (app->btor, o))
  {
    if (app->btor->options[o].internal) continue;
    if (o == BTOR_OPT_ENGINE || o == BTOR_OPT_SAT_ENGINE
        || o == BTOR_OPT_INPUT_FORMAT || o == BTOR_OPT_OUTPUT_NUMBER_FORMAT
        || o == BTOR_OPT_OUTPUT_FORMAT)
      continue;
    if (o == BTOR_OPT_INCREMENTAL || o == BTOR_OPT_REWRITE_LEVEL
        || o == BTOR_OPT_BETA_REDUCE_ALL || o == BTOR_OPT_AUTO_CLEANUP
        || o == BTOR_OPT_FUN_DUAL_PROP || o == BTOR_OPT_SLS_STRATEGY
        || o == BTOR_OPT_SORT_EXP)
      fprintf (out, "\n");
    print_opt (app,
               app->btor->options[o].lng,
               app->btor->options[o].shrt,
               app->btor->options[o].isflag,
               app->btor->options[o].dflt,
               app->btor->options[o].desc,
               true);
  }

  app->done = true;
}

static void
print_static_stats (int32_t sat_res)
{
  double real = btor_util_current_time () - g_start_time_real;
#ifdef BTOR_HAVE_GETRUSAGE
  double process = btor_util_time_stamp ();
  if (g_dual_threads)
    btormain_msg ("%.1f seconds process, %.0f%% utilization",
                  process,
                  real > 0 ? (100 * process) / real / 2 : 0.0);
  else
    btormain_msg ("%.1f seconds process", process);
#else
  btormain_msg ("can not determine run-time in seconds (no getrusage)");
#endif
  btormain_msg ("%.1f seconds real", real);
  btormain_msg ("%s",
                sat_res == BOOLECTOR_SAT
                    ? "sat"
                    : (sat_res == BOOLECTOR_UNSAT ? "unsat" : "unknown"));
}

static void
print_sat_result (BtorMainApp *app, int32_t sat_result)
{
  assert (app);
  if (sat_result == BOOLECTOR_UNSAT)
    fprintf (app->outfile, "unsat\n");
  else if (sat_result == BOOLECTOR_SAT)
    fprintf (app->outfile, "sat\n");
  else
  {
    assert (sat_result == BOOLECTOR_UNKNOWN);
    fprintf (app->outfile, "unknown\n");
  }
}

/*------------------------------------------------------------------------*/

static void
reset_sig_handlers (void)
{
  (void) signal (SIGINT, sig_int_handler);
  (void) signal (SIGSEGV, sig_segv_handler);
  (void) signal (SIGABRT, sig_abrt_handler);
  (void) signal (SIGTERM, sig_term_handler);
  (void) signal (SIGBUS, sig_bus_handler);
}

static void
catch_sig (int32_t sig)
{
  if (!g_caught_sig)
  {
    g_caught_sig = true;
    if (g_verbosity > 0)
    {
      boolector_print_stats (g_app->btor);
      print_static_stats (0);
    }
    btormain_msg ("CAUGHT SIGNAL %d", sig);
    fputs ("unknown\n", stdout);
    fflush (stdout);
  }
  reset_sig_handlers ();
  raise (sig);
}

static void
set_sig_handlers (void)
{
  sig_int_handler  = signal (SIGINT, catch_sig);
  sig_segv_handler = signal (SIGSEGV, catch_sig);
  sig_abrt_handler = signal (SIGABRT, catch_sig);
  sig_term_handler = signal (SIGTERM, catch_sig);
  sig_bus_handler  = signal (SIGBUS, catch_sig);
}

static void
reset_alarm (void)
{
  alarm (0);
  (void) signal (SIGALRM, sig_alrm_handler);
}

static void
catch_alarm (int32_t sig)
{
  (void) sig;
  assert (sig == SIGALRM);
  if (g_set_alarm > 0)
  {
    btormain_msg ("ALARM TRIGGERED: time limit %d seconds reached",
                  g_set_alarm);
    if (g_verbosity > 0)
    {
      boolector_print_stats (g_app->btor);
      print_static_stats (0);
    }
    fputs ("unknown\n", stdout);
    fflush (stdout);
  }
  reset_alarm ();
  _exit (0);
}

static void
set_alarm (void)
{
  sig_alrm_handler = signal (SIGALRM, catch_alarm);
  assert (g_set_alarm > 0);
  alarm (g_set_alarm);
}

/*------------------------------------------------------------------------*/

int32_t
boolector_main (int32_t argc, char **argv)
{
  int32_t i, len, format;
  int32_t res;
  int32_t parse_res, parse_status;
  int32_t sat_res;
  uint32_t mgen, pmodel, inc, dump;
  uint32_t val;
  bool dump_merge;
  char *cmd, *parse_err_msg;
  BtorParsedOpt *po;
  BtorParsedOptPtrStack opts;
  BtorParsedInput *pin;
  BtorParsedInputPtrStack infiles;
  BtorOption bopt;
  BtorOpt *bo;
  BtorMainOption bmopt;
  BtorMainOpt *bmo;
  BtorMemMgr *mm;
  Btor *btor;

  g_start_time_real = btor_util_current_time ();

  g_app = btormain_new_btormain (boolector_new ());
  btor  = g_app->btor;
  mm    = g_app->mm;

  res          = BTOR_UNKNOWN_EXIT;
  parse_res    = BOOLECTOR_UNKNOWN;
  parse_status = BOOLECTOR_UNKNOWN;
  sat_res      = BOOLECTOR_UNKNOWN;

  inc    = 0;
  mgen   = boolector_get_opt (btor, BTOR_OPT_MODEL_GEN);
  pmodel = 0;
  dump   = 0;

  dump_merge = false;

  BTOR_INIT_STACK (mm, opts);
  BTOR_INIT_STACK (mm, infiles);

  btor_optparse_parse (
      mm, argc, argv, &opts, &infiles, btormain_opt_has_str_arg);

  /* input file ======================================================= */

  if (BTOR_COUNT_STACK (infiles) > 1)
  {
    btormain_error (g_app, "multiple input files");
    goto DONE;
  }
  else if (BTOR_COUNT_STACK (infiles) == 1)
  {
    g_app->infile_name = BTOR_PEEK_STACK (infiles, 0)->name;
    if (!btor_util_file_exists (g_app->infile_name))
    {
      g_app->infile = 0;
    }
    else if (btor_util_file_has_suffix (g_app->infile_name, ".gz")
             || btor_util_file_has_suffix (g_app->infile_name, ".bz2")
             || btor_util_file_has_suffix (g_app->infile_name, ".7z")
             || btor_util_file_has_suffix (g_app->infile_name, ".zip"))
    {
      len = strlen (g_app->infile_name);
      BTOR_NEWN (g_app->mm, cmd, len + 40);
      if (btor_util_file_has_suffix (g_app->infile_name, ".gz"))
        sprintf (cmd, "gunzip -c %s", g_app->infile_name);
      else if (btor_util_file_has_suffix (g_app->infile_name, ".bz2"))
        sprintf (cmd, "bzcat %s", g_app->infile_name);
      else if (btor_util_file_has_suffix (g_app->infile_name, ".7z"))
        sprintf (cmd, "7z x -so %s 2> /dev/null", g_app->infile_name);
      else if (btor_util_file_has_suffix (g_app->infile_name, ".zip"))
        sprintf (cmd, "unzip -p %s", g_app->infile_name);

      if ((g_app->infile = popen (cmd, "r"))) g_app->close_infile = 2;

      BTOR_DELETEN (g_app->mm, cmd, len + 40);
    }
    else if ((g_app->infile = fopen (g_app->infile_name, "r")))
    {
      g_app->close_infile = 1;
    }

    if (!g_app->infile)
    {
      btormain_error (g_app, "can not read '%s'", g_app->infile_name);
      goto DONE;
    }
  }

  /* options ========================================================== */

  for (i = 0; i < BTOR_COUNT_STACK (opts); i++)
  {
    po = BTOR_PEEK_STACK (opts, i);

    for (bmopt = 0, bmo = 0; bmopt < BTORMAIN_OPT_NUM_OPTS; bmopt++)
    {
      bmo = &g_app->options[bmopt];
      if ((po->isshrt && bmo->shrt && !strcmp (bmo->shrt, po->name.start))
          || (!po->isshrt && !strcmp (bmo->lng, po->name.start)))
      {
        break;
      }
      bmo = 0;
    }

    /* main options ----------------------------------------------------- */
    if (bmo)
    {
      /* check opt */
      if (po->isdisable && !bmo->candisable)
      {
        btormain_error (g_app, "invalid option '%s'", po->orig.start);
        goto DONE;
      }
      if (bmo->arg == BTOR_ARG_EXPECT_NONE)
      {
        if (BTOR_ARG_IS_UNEXPECTED (bmo->arg, po->readval, po->isdisable))
        {
          btormain_error (
              g_app, "option '%s' does not expect an argument", po->orig.start);
          goto DONE;
        }
      }
      else
      {
        if (BTOR_ARG_IS_MISSING (bmo->arg, bmo->candisable, po->readval))
        {
          btormain_error (g_app, "missing argument for '%s'", po->orig.start);
          goto DONE;
        }
        if (BTOR_ARG_IS_INVALID (bmo->arg, bmo->candisable, po->readval))
        {
          btormain_error (
              g_app, "invalid argument for '%s', expected int", po->orig.start);
          goto DONE;
        }
      }
      /* set opt */
      switch (bmopt)
      {
        case BTORMAIN_OPT_HELP: print_help (g_app); goto DONE;

        case BTORMAIN_OPT_COPYRIGHT:
          fprintf (g_app->outfile, "%s", boolector_copyright (btor));
          g_app->done = true;
          goto DONE;

        case BTORMAIN_OPT_VERSION:
          fprintf (g_app->outfile, "%s\n", boolector_version (btor));
          g_app->done = true;
          goto DONE;

        case BTORMAIN_OPT_TIME: g_set_alarm = po->val; break;

        case BTORMAIN_OPT_OUTPUT:
          if (g_app->close_outfile)
          {
            btormain_error (g_app, "multiple output files");
            goto DONE;
          }
          g_app->outfile_name = po->valstr;
          break;

        case BTORMAIN_OPT_SMT2_MODEL:
          g_app->options[BTORMAIN_OPT_SMT2_MODEL].val += 1;
          break;

        case BTORMAIN_OPT_ENGINE:
          if (!strcasecmp (po->valstr, "core"))
            boolector_set_opt (btor, BTOR_OPT_ENGINE, BTOR_ENGINE_FUN);
          else if (!strcasecmp (po->valstr, "sls"))
            boolector_set_opt (btor, BTOR_OPT_ENGINE, BTOR_ENGINE_SLS);
          else if (!strcasecmp (po->valstr, "prop"))
            boolector_set_opt (btor, BTOR_OPT_ENGINE, BTOR_ENGINE_PROP);
          else if (!strcasecmp (po->valstr, "aigprop"))
            boolector_set_opt (btor, BTOR_OPT_ENGINE, BTOR_ENGINE_AIGPROP);
          else if (!strcasecmp (po->valstr, "ef"))
            boolector_set_opt (btor, BTOR_OPT_ENGINE, BTOR_ENGINE_QUANT);
          else
          {
            btormain_error (g_app,
                            "invalid engine '%s' for '%s'",
                            po->valstr,
                            po->orig.start);
            goto DONE;
          }
          break;

        case BTORMAIN_OPT_SAT_ENGINE:
          if (!strcasecmp (po->valstr, "lingeling"))
          {
            boolector_set_opt (
                btor, BTOR_OPT_SAT_ENGINE, BTOR_SAT_ENGINE_LINGELING);
          }
          else if (!strcasecmp (po->valstr, "picosat"))
          {
            boolector_set_opt (
                btor, BTOR_OPT_SAT_ENGINE, BTOR_SAT_ENGINE_PICOSAT);
          }
          else if (!strcasecmp (po->valstr, "minisat"))
            boolector_set_opt (
                btor, BTOR_OPT_SAT_ENGINE, BTOR_SAT_ENGINE_MINISAT);
          else if (!strcasecmp (po->valstr, "cadical"))
            boolector_set_opt (
                btor, BTOR_OPT_SAT_ENGINE, BTOR_SAT_ENGINE_CADICAL);
          else
          {
            btormain_error (g_app,
                            "invalid sat solver '%s' for '%s'",
                            po->valstr,
                            po->orig.start);
            goto DONE;
          }
          break;

        case BTORMAIN_OPT_LGL_NOFORK:
          boolector_set_opt (btor, BTOR_OPT_SAT_ENGINE_LGL_FORK, 0);
          break;

        case BTORMAIN_OPT_HEX:
          format = BTOR_OUTPUT_BASE_HEX;
        SET_OUTPUT_NUMBER_FORMAT:
          boolector_set_opt (btor, BTOR_OPT_OUTPUT_NUMBER_FORMAT, format);
          break;

        case BTORMAIN_OPT_DEC:
          format = BTOR_OUTPUT_BASE_DEC;
          goto SET_OUTPUT_NUMBER_FORMAT;

        case BTORMAIN_OPT_BIN:
          format = BTOR_OUTPUT_BASE_BIN;
          goto SET_OUTPUT_NUMBER_FORMAT;

        case BTORMAIN_OPT_BTOR:
          format = BTOR_INPUT_FORMAT_BTOR;
        SET_INPUT_FORMAT:
          boolector_set_opt (btor, BTOR_OPT_INPUT_FORMAT, format);
          break;

        case BTORMAIN_OPT_SMT2:
          format = BTOR_INPUT_FORMAT_SMT2;
          goto SET_INPUT_FORMAT;

        case BTORMAIN_OPT_SMT1:
          format = BTOR_INPUT_FORMAT_SMT1;
          goto SET_INPUT_FORMAT;

        case BTORMAIN_OPT_DUMP_BTOR:
          dump = BTOR_OUTPUT_FORMAT_BTOR;
        SET_OUTPUT_FORMAT:
          boolector_set_opt (btor, BTOR_OPT_OUTPUT_FORMAT, dump);
          boolector_set_opt (btor, BTOR_OPT_PARSE_INTERACTIVE, 0);
          break;
#if 0
              case BTORMAIN_OPT_DUMP_BTOR2:
                dump = BTOR_OUTPUT_FORMAT_BTOR2;
                goto SET_OUTPUT_FORMAT;
#endif
        case BTORMAIN_OPT_DUMP_SMT:
          dump = BTOR_OUTPUT_FORMAT_SMT2;
          goto SET_OUTPUT_FORMAT;

        case BTORMAIN_OPT_DUMP_AAG:
          dump = BTOR_OUTPUT_FORMAT_AIGER_ASCII;
          goto SET_OUTPUT_FORMAT;

        case BTORMAIN_OPT_DUMP_AIG:
          dump = BTOR_OUTPUT_FORMAT_AIGER_BINARY;
          goto SET_OUTPUT_FORMAT;

        case BTORMAIN_OPT_DUMP_AIGER_MERGE: dump_merge = true; break;

        default:
          /* get rid of compiler warnings, should be unreachable */
          assert (bmopt == BTORMAIN_OPT_NUM_OPTS);
      }
    }
    /* >> btor options ------------------------------------------------ */
    else
    {
      for (bopt = boolector_first_opt (btor), bo = 0;
           boolector_has_opt (btor, bopt);
           bopt = btor_opt_next (btor, bopt))
      {
        bo = &btor->options[bopt];
        if ((po->isshrt && bo->shrt && !strcmp (bo->shrt, po->name.start))
            || (!po->isshrt && !strcmp (bo->lng, po->name.start)))
        {
          break;
        }
        bo = 0;
      }
      /* check opt */
      if (!bo)
      {
        btormain_error (g_app, "invalid option '%s'", po->orig.start);
        goto DONE;
      }
      if (BTOR_ARG_IS_MISSING (BTOR_ARG_EXPECT_INT, bo->isflag, po->readval))
      {
        btormain_error (g_app, "missing argument for '%s'", po->orig.start);
        goto DONE;
      }
      if (BTOR_ARG_IS_INVALID (BTOR_ARG_EXPECT_INT, bo->isflag, po->readval))
      {
        btormain_error (
            g_app, "invalid argument for '%s', expected int", po->orig.start);
        goto DONE;
      }
      if (bo->isflag)
      {
        if (po->isdisable)
        {
          if (bopt == BTOR_OPT_MODEL_GEN)
          {
            mgen   = 0;
            pmodel = 0;
          }
          else
            boolector_set_opt (btor, bopt, 0);
        }
        else
        {
          switch (bopt)
          {
            case BTOR_OPT_MODEL_GEN:
              if (BTOR_ARG_READ_IS_INT (po->readval) && po->val == 0)
              {
                mgen   = 0;
                pmodel = 0;
              }
              else
              {
                mgen += 1;
                pmodel = 1;
              }
              break;
            case BTOR_OPT_VERBOSITY:
            case BTOR_OPT_LOGLEVEL:
              if (BTOR_ARG_READ_IS_INT (po->readval))
                boolector_set_opt (btor, bopt, po->val);
              else
                boolector_set_opt (
                    btor, bopt, boolector_get_opt (btor, bopt) + 1);
              break;
            default:
              assert (bopt != BTOR_OPT_NUM_OPTS);
              if (BTOR_ARG_READ_IS_INT (po->readval))
                boolector_set_opt (btor, bopt, po->val);
              else
                boolector_set_opt (btor, bopt, 1);
          }
        }
      }
      else
      {
        assert (BTOR_ARG_READ_IS_INT (po->readval));
        boolector_set_opt (btor, bopt, po->val);
      }
    }
  }

  assert (!g_app->done && !g_app->err);

  g_verbosity = boolector_get_opt (g_app->btor, BTOR_OPT_VERBOSITY);

  /* open output file */
  if (g_app->outfile_name)
  {
    if (!strcmp (g_app->outfile_name, g_app->infile_name))
    {
      btormain_error (g_app, "input and output file must not be the same");
      goto DONE;
    }

    g_app->outfile = fopen (g_app->outfile_name, "w");
    if (!g_app->outfile)
    {
      btormain_error (g_app, "can not create '%s'", g_app->outfile_name);
      goto DONE;
    }
    g_app->close_outfile = true;
  }

  /* automatically enable model generation if smt2 models are forced */
  val  = g_app->options[BTORMAIN_OPT_SMT2_MODEL].val;
  mgen = !mgen && g_app->options[BTORMAIN_OPT_SMT2_MODEL].val ? val : mgen;

  // TODO: disabling model generation not yet supported (ma)
  if (mgen > 0) boolector_set_opt (btor, BTOR_OPT_MODEL_GEN, mgen);

  /* print verbose info and set signal handlers */
  if (g_verbosity)
  {
    if (inc) btormain_msg ("incremental mode through command line option");
    btormain_msg ("Boolector Version %s %s", BTOR_VERSION, BTOR_ID);
    btormain_msg ("%s", BTOR_CFLAGS);
    btormain_msg ("released %s", BTOR_RELEASED);
    btormain_msg ("compiled %s", BTOR_COMPILED);
    if (*BTOR_CC) btormain_msg ("%s", BTOR_CC);
    btormain_msg ("setting signal handlers");
  }
  set_sig_handlers ();

  /* set alarm */
  if (g_set_alarm)
  {
    if (g_verbosity)
      btormain_msg ("setting time limit to %d seconds", g_set_alarm);
    set_alarm ();
  }
  else if (g_verbosity)
    btormain_msg ("no time limit given");

  if (inc && g_verbosity) btormain_msg ("starting incremental mode");

  /* parse */
  val = boolector_get_opt (btor, BTOR_OPT_INPUT_FORMAT);
  switch (val)
  {
    case BTOR_INPUT_FORMAT_BTOR:
      if (g_verbosity)
        btormain_msg ("BTOR input forced through cmd line options");
      parse_res = boolector_parse_btor (btor,
                                        g_app->infile,
                                        g_app->infile_name,
                                        g_app->outfile,
                                        &parse_err_msg,
                                        &parse_status);
      break;
    case BTOR_INPUT_FORMAT_SMT1:
      if (g_verbosity)
        btormain_msg ("SMT-LIB v1 input forced through cmd line options");
      parse_res = boolector_parse_smt1 (btor,
                                        g_app->infile,
                                        g_app->infile_name,
                                        g_app->outfile,
                                        &parse_err_msg,
                                        &parse_status);
      break;
    case BTOR_INPUT_FORMAT_SMT2:
      if (g_verbosity)
        btormain_msg ("SMT-LIB v2 input forced through cmd line options");
      parse_res = boolector_parse_smt2 (btor,
                                        g_app->infile,
                                        g_app->infile_name,
                                        g_app->outfile,
                                        &parse_err_msg,
                                        &parse_status);
      break;

    default:
      parse_res = boolector_parse (btor,
                                   g_app->infile,
                                   g_app->infile_name,
                                   g_app->outfile,
                                   &parse_err_msg,
                                   &parse_status);
  }

  /* verbosity may have been increased via input (set-option) */
  g_verbosity = boolector_get_opt (btor, BTOR_OPT_VERBOSITY);

  g_dual_threads =
      boolector_get_opt (g_app->btor, BTOR_OPT_QUANT_DUAL_SOLVER) == 1
      && g_app->btor->quantifiers->count > 0;

  if (parse_res == BOOLECTOR_PARSE_ERROR)
  {
    /* NOTE: do not use btormain_error here as 'parse_err_msg' must not be
     * treated as format string --- it might contain unescaped '%' due to
     * invalid user input. */
    fprintf (stderr, "boolector: %s\n", parse_err_msg);
    g_app->err = BTOR_ERR_EXIT;
    goto DONE;
  }

  /* incremental mode */
  if (inc)
  {
    if (parse_res == BOOLECTOR_SAT)
    {
      if (g_verbosity) btormain_msg ("one formula SAT in incremental mode");
      sat_res = BOOLECTOR_SAT;
    }
    else if (parse_res == BOOLECTOR_UNSAT)
    {
      if (g_verbosity) btormain_msg ("all formulas UNSAT in incremental mode");
      sat_res = BOOLECTOR_UNSAT;
    }

    if (g_verbosity) boolector_print_stats (btor);

    if (pmodel && sat_res == BOOLECTOR_SAT)
    {
      assert (boolector_get_opt (btor, BTOR_OPT_MODEL_GEN));
      val = g_app->options[BTORMAIN_OPT_SMT2_MODEL].val;
      boolector_print_model (btor, val ? "smt2" : "btor", g_app->outfile);
    }

#ifdef BTOR_HAVE_GETRUSAGE
    if (g_verbosity) btormain_msg ("%.1f seconds", btor_util_time_stamp ());
#endif
    goto DONE;
  }
  /* we don't dump formula(s) in incremental mode */
  else if (dump)
  {
    (void) boolector_simplify (btor);

    switch (dump)
    {
      case BTOR_OUTPUT_FORMAT_BTOR:
        if (g_verbosity) btormain_msg ("dumping BTOR expressions");
        boolector_dump_btor (btor, g_app->outfile);
        break;
#if 0
          case BTOR_OUTPUT_FORMAT_BTOR2:
            if (g_verbosity) btormain_msg ("dumping BTOR 2.0 expressions");
            boolector_dump_btor2 (btor, g_app->outfile);
            break;
#endif
      case BTOR_OUTPUT_FORMAT_SMT2:
        if (g_verbosity) btormain_msg ("dumping in SMT 2.0 format");
        boolector_dump_smt2 (btor, g_app->outfile);
        break;
      case BTOR_OUTPUT_FORMAT_AIGER_ASCII:
        if (g_verbosity) btormain_msg ("dumping in ascii AIGER format");
        boolector_dump_aiger_ascii (btor, g_app->outfile, dump_merge);
        break;
      default:
        assert (dump == BTOR_OUTPUT_FORMAT_AIGER_BINARY);
        if (g_verbosity) btormain_msg ("dumping in binary AIGER format");
        boolector_dump_aiger_binary (btor, g_app->outfile, dump_merge);
    }

    if (g_verbosity) boolector_print_stats (btor);

    goto DONE;
  }

  /* call sat (if not yet called) */
  if (parse_res == BOOLECTOR_PARSE_UNKNOWN && !boolector_terminate (btor))
  {
    sat_res = boolector_sat (btor);
    print_sat_result (g_app, sat_res);
  }
  else
    sat_res = parse_res;

  assert (boolector_terminate (btor) || sat_res != BOOLECTOR_UNKNOWN);

  /* check if status is equal to benchmark status (if provided) */
  if (sat_res == BOOLECTOR_SAT && parse_status == BOOLECTOR_UNSAT)
    btormain_error (g_app,
                    "'sat' but status of benchmark in '%s' is 'unsat'",
                    g_app->infile_name);
  else if (sat_res == BOOLECTOR_UNSAT && parse_status == BOOLECTOR_SAT)
    btormain_error (g_app,
                    "'unsat' but status of benchmark in '%s' is 'sat'",
                    g_app->infile_name);

  /* print stats */
  if (g_verbosity)
  {
    boolector_print_stats (btor);
    print_static_stats (sat_res);
  }

  /* print model */
  if (pmodel && sat_res == BOOLECTOR_SAT)
  {
    assert (boolector_get_opt (btor, BTOR_OPT_MODEL_GEN));
    val = g_app->options[BTORMAIN_OPT_SMT2_MODEL].val;
    boolector_print_model (btor, val ? "smt2" : "btor", g_app->outfile);
  }

DONE:
  if (g_app->done)
    res = BTOR_SUCC_EXIT;
  else if (g_app->err)
    res = BTOR_ERR_EXIT;
  else if (sat_res == BOOLECTOR_UNSAT)
    res = BTOR_UNSAT_EXIT;
  else if (sat_res == BOOLECTOR_SAT)
    res = BTOR_SAT_EXIT;

  assert (res == BTOR_ERR_EXIT || res == BTOR_SUCC_EXIT || res == BTOR_SAT_EXIT
          || res == BTOR_UNSAT_EXIT || res == BTOR_UNKNOWN_EXIT);

  if (g_app->close_infile == 1)
    fclose (g_app->infile);
  else if (g_app->close_infile == 2)
    pclose (g_app->infile);
  if (g_app->close_outfile) fclose (g_app->outfile);

  if (!boolector_get_opt (btor, BTOR_OPT_EXIT_CODES))
  {
    switch (res)
    {
      case BTOR_UNSAT_EXIT:
      case BTOR_SAT_EXIT: res = BTOR_SUCC_EXIT; break;
      default: res = BTOR_ERR_EXIT;
    }
  }

  while (!BTOR_EMPTY_STACK (opts))
  {
    po = BTOR_POP_STACK (opts);
    assert (po->mm == mm);
    BTOR_RELEASE_STACK (po->orig);
    BTOR_RELEASE_STACK (po->name);
    BTOR_DELETE (mm, po);
  }
  BTOR_RELEASE_STACK (opts);
  while (!BTOR_EMPTY_STACK (infiles))
  {
    pin = BTOR_POP_STACK (infiles);
    assert (pin->mm == mm);
    BTOR_DELETE (mm, pin);
  }
  BTOR_RELEASE_STACK (infiles);

  btormain_delete_btormain (g_app);
  reset_sig_handlers ();

  return res;
}

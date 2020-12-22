/*
 * Preservation Virtual Machine Project
 * Common hooks for the ivm64 target
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: Oct 2019 - Dec 2020
 *
 */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "common/common-target.h"
#include "common/common-target-def.h"
#include "opts.h"
#include "flags.h"
#include "params.h"

const struct default_options ivm64_target_option_optimization_table[]=
  {
    { OPT_LEVELS_ALL, OPT_fomit_frame_pointer, NULL, 1 },
    { OPT_LEVELS_ALL, OPT_fdefer_pop, NULL, 0 },
    { OPT_LEVELS_NONE, 0, NULL, 0 }
  };
#undef TARGET_OPTION_OPTIMIZATION_TABLE
#define TARGET_OPTION_OPTIMIZATION_TABLE ivm64_target_option_optimization_table

#undef TARGET_DEFAULT_TARGET_FLAGS
#define TARGET_DEFAULT_TARGET_FLAGS (TARGET_DEFAULT)

static void
ivm64_option_init_struct (struct gcc_options *opts)
{
  opts->x_optimize = 2; // set optimization -O2 by default
}
#undef TARGET_OPTION_INIT_STRUCT
#define TARGET_OPTION_INIT_STRUCT ivm64_option_init_struct

static void ivm64_option_default_params(void)
{
  set_default_param_value(PARAM_MAX_GROW_COPY_BB_INSNS, 48);
  set_default_param_value(PARAM_INLINE_UNIT_GROWTH, 30);      // allow 50% increment (def. 20%)
  set_default_param_value(PARAM_EARLY_INLINING_INSNS, 20);    // allow 50% increment (def. 14%)
  set_default_param_value(PARAM_LARGE_STACK_FRAME, 512);      // def. 256
  set_default_param_value(PARAM_STACK_FRAME_GROWTH, 1500);    // def. 1000
  set_default_param_value(PARAM_MAX_HOIST_DEPTH, 20);         // 1 is the minimun, 0 disable limit, def:30
}
#undef TARGET_OPTION_DEFAULT_PARAMS
#define TARGET_OPTION_DEFAULT_PARAMS ivm64_option_default_params

struct gcc_targetm_common targetm_common = TARGETM_COMMON_INITIALIZER;

/*
 * Preservation Virtual Machine Project
 * Common hooks for the ivm64 target
 *
 * Authors:
 *  Eladio Gutierrez Carrasco
 *  Sergio Romero Montiel
 *  Oscar Plata Gonzalez
 *
 * Date: Oct 2019 - Feb 2021
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

const struct default_options ivm64_target_option_optimization_table[]=
  {
    { OPT_LEVELS_ALL, OPT_fomit_frame_pointer, NULL, 1 },
    { OPT_LEVELS_ALL, OPT_fdefer_pop, NULL, 0 },
    { OPT_LEVELS_1_PLUS, OPT_fpeephole2, NULL, 1 },
    { OPT_LEVELS_ALL, OPT_freorder_blocks, NULL, 0 },
    { OPT_LEVELS_ALL, OPT_ftree_ter, NULL, 0 },
    { OPT_LEVELS_ALL, OPT_fdce, NULL, 0 },
    { OPT_LEVELS_ALL, OPT_fdse, NULL, 0 },
    { OPT_LEVELS_ALL, OPT_fcrossjumping, NULL, 0 },
    { OPT_LEVELS_ALL, OPT_fdelete_null_pointer_checks, NULL, 0 },
    { OPT_LEVELS_ALL, OPT_fisolate_erroneous_paths_dereference, NULL, 0 },
    { OPT_LEVELS_ALL, OPT_freorder_blocks_algorithm_, NULL, REORDER_BLOCKS_ALGORITHM_STC},
    { OPT_LEVELS_NONE, 0, NULL, 0 }
  };
#undef TARGET_OPTION_OPTIMIZATION_TABLE
#define TARGET_OPTION_OPTIMIZATION_TABLE ivm64_target_option_optimization_table

static void
ivm64_option_init_struct (struct gcc_options *opts)
{
  opts->x_optimize = 2; // set optimization -O2 by default
}
#undef TARGET_OPTION_INIT_STRUCT
#define TARGET_OPTION_INIT_STRUCT ivm64_option_init_struct

struct gcc_targetm_common targetm_common = TARGETM_COMMON_INITIALIZER;

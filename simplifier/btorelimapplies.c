/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2013-2015 Mathias Preiner.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#include "btorelimapplies.h"
#include "btorcore.h"
#include "btordbg.h"
#include "btoriter.h"
#include "btorutil.h"

void
btor_eliminate_applies (Btor *btor)
{
  assert (btor);

  int num_applies, num_applies_total = 0, round;
  double start, delta;
  BtorPtrHashTable *apps;
  BtorNode *app, *fun;
  BtorNodeIterator it;
  BtorHashTableIterator h_it;
  BtorMemMgr *mm;

  if (btor->lambdas->count == 0) return;

  start = btor_time_stamp ();

  mm    = btor->mm;
  round = 1;

  /* NOTE: in some cases substitute_and_rebuild creates applies that can be
   * beta-reduced. this can happen when parameterized applies become not
   * parameterized. hence, we beta-reduce applies until fix point.
   */
  do
  {
    apps = btor_new_ptr_hash_table (mm,
                                    (BtorHashPtr) btor_hash_exp_by_id,
                                    (BtorCmpPtr) btor_compare_exp_by_id);

    /* collect function applications */
    init_node_hash_table_iterator (&h_it, btor->lambdas);
    while (has_next_node_hash_table_iterator (&h_it))
    {
      fun = next_node_hash_table_iterator (&h_it);

      init_apply_parent_iterator (&it, fun);
      while (has_next_parent_apply_parent_iterator (&it))
      {
        app = next_parent_apply_parent_iterator (&it);

        if (btor_find_in_ptr_hash_table (apps, app)) continue;

        if (app->parameterized) continue;

        btor_insert_in_ptr_hash_table (apps, btor_copy_exp (btor, app));
      }
    }

    num_applies = apps->count;
    num_applies_total += num_applies;
    BTOR_MSG (btor->msg,
              1,
              "eliminate %d applications in round %d",
              num_applies,
              round);

    btor_substitute_and_rebuild (btor, apps, -1);

    init_node_hash_table_iterator (&h_it, apps);
    while (has_next_node_hash_table_iterator (&h_it))
      btor_release_exp (btor, next_node_hash_table_iterator (&h_it));
    btor_delete_ptr_hash_table (apps);
    round++;
  } while (num_applies > 0);

#ifndef NDEBUG
  init_node_hash_table_iterator (&h_it, btor->lambdas);
  while (has_next_node_hash_table_iterator (&h_it))
  {
    fun = next_node_hash_table_iterator (&h_it);

    init_apply_parent_iterator (&it, fun);
    while (has_next_parent_apply_parent_iterator (&it))
    {
      app = next_parent_apply_parent_iterator (&it);
      assert (app->parameterized);
    }
  }
#endif

  delta = btor_time_stamp () - start;
  btor->time.betareduce += delta;
  BTOR_MSG (btor->msg,
            1,
            "eliminated %d function applications in %.1f seconds",
            num_applies_total,
            delta);
  assert (check_all_hash_tables_proxy_free_dbg (btor));
  assert (check_all_hash_tables_simp_free_dbg (btor));
  assert (check_unique_table_children_proxy_free_dbg (btor));
}

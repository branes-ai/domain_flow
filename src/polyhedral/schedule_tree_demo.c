#include <isl/ctx.h>
#include <isl/union_set.h>
#include <isl/union_map.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>
#include <isl/printer.h>
#include <stdio.h>

int main() {
    isl_ctx *ctx = isl_ctx_alloc();

    // Step 1: Define the iteration domain
    isl_union_set *domain = isl_union_set_read_from_str(ctx,
        "{ S[i,j] : 0 <= i < 4 and 0 <= j < 3 }");

    // Step 2: Define a schedule mapping (e.g. tiling)
    isl_union_map *schedule = isl_union_map_read_from_str(ctx,
        "{ S[i,j] -> [i/2, j/2, i%2, j%2] }");

    // Step 3: Create a schedule from domain and mapping
    isl_schedule *sched = isl_schedule_from_domain(domain);
    sched = isl_schedule_insert_partial_schedule(sched, isl_multi_union_pw_aff_from_union_map(schedule));

    // Step 4: Traverse and print the schedule tree
    isl_schedule_node *root = isl_schedule_get_root(sched);
    isl_printer *p = isl_printer_to_file(ctx, stdout);
    p = isl_printer_set_yaml_style(p, ISL_YAML_STYLE_BLOCK);
    p = isl_printer_print_schedule_node(p, root);

    // Cleanup
    isl_printer_free(p);
    isl_schedule_node_free(root);
    isl_schedule_free(sched);
    isl_ctx_free(ctx);

    return 0;
}
#include <isl/ctx.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>
#include <isl/printer.h>
#include <iostream>

int main() {
    isl::ctx ctx;

    // Step 1: Define the iteration domain
    isl::union_set domain = isl::union_set::read_from_str(ctx,
        "{ S[i,j] : 0 <= i < 4 and 0 <= j < 3 }");

    // Step 2: Define a schedule mapping (e.g. tiling with Fibonacci sizes)
    isl::union_map schedule = isl::union_map::read_from_str(ctx,
        "{ S[i,j] -> [i/34, j/55, i%34, j%55] }");

    // Step 3: Create a schedule and insert the partial schedule
    isl::schedule sched = isl::schedule::from_domain(domain);
    sched = sched.insert_partial_schedule(isl::multi_union_pw_aff::from_union_map(schedule));

    // Step 4: Print the schedule tree
    isl::schedule_node root = sched.get_root();
    isl::printer p = isl::printer(ctx).set_output_format(isl_format_yaml);
    p = p.print_schedule_node(root);

    std::cout << p.get_str() << std::endl;

    return 0;
}
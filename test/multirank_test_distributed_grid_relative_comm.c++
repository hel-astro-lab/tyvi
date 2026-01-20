#include <boost/ut.hpp> // import boost.ut;

#include <print>
#include <stdexcept>
#include <utility>

#include "pika/init.hpp"
#include "pika/mpi.hpp"

#include "tyvi/execution.h"

namespace mpix = pika::mpi::experimental;

namespace {
using namespace boost::ut;
[[maybe_unused]]
const suite<"distributed_grid relative communication"> _ = [] {
    /*

    "any rank can access any index"_test = [] {
        struct test_type {
            int value{};

            constexpr auto communicate(tyvi::communication_tag<int>) const {
                return tyvi::exec::just(value);
            }
        };

        using E   = std::dextents<std::size_t, 1>;
        auto grid = tyvi::distributed_grid<int, E>(8);


        auto snd = grid[]

        tyvi::this_thread::sync_wait(std::move(snd));

        expect(grid[0].value == 1);
        expect(grid[1].value == 3);
        expect(grid[2].value == 2);
    };


   /*
    "1D ring communication"_test = [] {
        struct test_type {
            int value{};

            constexpr auto communicate(tyvi::communication_tag<int>) const {
                return tyvi::exec::just(value);
            }
        };

        using E   = std::dextents<std::size_t, 1>;
        auto grid = tyvi::distributed_grid<int, E>(8);

        auto init_to_idx = [&](const auto idx){ grid[idx].value = idx[0]; };
        auto add = [](test_type& me, const test_type& other){ me.value += other.value; };

        tyvi::exec::thread_pool_scheduler sch{};

        grid.for_each_index([&](const auto idx){
            grid[idx] = idx[0];

            tyvi::exec::just(idx)
            | tyvi::exec::continues_on(sch)
            | tyvi::exec::then(init_to_idx)
            | tyvi::exec::let_value(grid.read<{-1}>(idx))
            | tyvi::exec::then([](const test_type& other){})
        });

        grid.read<{-1}>(addk);

        auto init_to_index = grid.for_each_index([&](const auto idx) );
        auto add_from_right = grid.communicate_pairwise_relative<int, { 1 }>(
            [&](const auto idx, auto&& x) { grid[idx].value += x; });

        auto snd = tyvi::just() | tyvi::exec::let_value([&] { return init_to_index; })
                   | tyvi::exec::let_value([&] { return add_from_right; });

        tyvi::this_thread::sync_wait(std::move(snd));

        expect(grid[0].value == 1);
        expect(grid[1].value == 3);
        expect(grid[2].value == 2);
    };

    */
};

} // namespace

int
main(int argc, char** argv) {
    int provided, preferred = mpix::get_preferred_thread_mode();

    MPI_Init_thread(&argc, &argv, preferred, &provided);
    if (provided != preferred) { throw std::runtime_error{ "Provided MPI is not as requested" }; }

    pika::init_params init_args;
    init_args.cfg.emplace_back("pika.mpi.enable_pool=true");
    const auto result = pika::init(
        [&] {
            const auto x = static_cast<int>(
                cfg<override>.run(run_cfg{ .argc = argc, .argv = const_cast<const char**>(argv) }));
            pika::finalize();
            return x;
        },
        0,
        nullptr,
        init_args);

    if (MPI_Finalize()) { throw std::runtime_error{ "MPI_Finalize() failed!" }; }

    return result;
}

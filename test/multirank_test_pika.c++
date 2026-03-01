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
const suite<"pika"> _ = [] {
    "mpi polling"_test = [] {
        int size{}, rank{};
        MPI_Comm comm = MPI_COMM_WORLD;
        MPI_Comm_size(comm, &size);
        MPI_Comm_rank(comm, &rank);
        const mpix::enable_polling enable_polling{};
        int send_data{ rank }, recv_data{};

        const tyvi::exec::thread_pool_scheduler sch{};

        auto sch1 = tyvi::exec::just(&send_data, 1, MPI_INT, (rank + 1) % size, 0, comm)
                    | tyvi::exec::continues_on(sch) | mpix::transform_mpi(MPI_Isend);

        auto sch2 = tyvi::exec::just(&recv_data, 1, MPI_INT, (rank + size - 1) % size, 0, comm)
                    | tyvi::exec::continues_on(sch) | mpix::transform_mpi(MPI_Irecv);

        expect(recv_data == 0);
        tyvi::this_thread::sync_wait(tyvi::exec::when_all(std::move(sch1), std::move(sch2)));
        expect(recv_data == (rank + size - 1) % size);
    };
};

} // namespace

int
main(int argc, char** argv) {
    int provided{}, preferred = mpix::get_preferred_thread_mode();

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

    if (static_cast<bool>(MPI_Finalize())) { throw std::runtime_error{ "MPI_Finalize() failed!" }; }

    return result;
}

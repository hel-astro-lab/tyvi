#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <experimental/mdspan>

#include "thrust/copy.h"
#include "thrust/device_vector.h"
#include "thrust/execution_policy.h"
#include "thrust/for_each.h"
#include "thrust/host_vector.h"
#include "thrust/logical.h"
#include "thrust/sequence.h"

#include "tyvi/mdgrid_buffer.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_buffer"> _ = [] {
    using element_type          = int;
    using vec                   = std::vector<element_type>;
    using element_extents       = tyvi::sstd::geometric_extents<2, 2>;
    using element_layout_policy = std::layout_right;
    using grid_extents          = std::dextents<std::size_t, 3>;
    using grid_layout_policy    = std::layout_right;

    using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                      element_extents,
                                                      element_layout_policy,
                                                      grid_extents,
                                                      grid_layout_policy>;

    "mdgrid_buffer is constructible"_test = [] {
        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        [[maybe_unused]]
        auto _ = testing_mdgrid_buffer(grid_mapping);
        expect(true);
    };

    "mdgrid_buffer is constructible from grid extents"_test = [] {
        auto mdgbA = testing_mdgrid_buffer(4, 2, 1);
        auto mdgbB = testing_mdgrid_buffer(mdgbA.grid_extents());
        expect(mdgbA.grid_extents() == mdgbB.grid_extents());
    };

    "mdgrid_buffer gives correct element extents"_test = [] {
        auto mdgb = testing_mdgrid_buffer(4, 2, 1);
        expect(mdgb.element_extents() == element_extents{});
    };

    "mdgrid_buffer is copyable and const correct viewable through mdspan"_test = [] {
        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        auto buff = testing_mdgrid_buffer(grid_mapping);

        const auto mds = buff.mds();

        expect(not std::is_const_v<
               std::remove_reference_t<typename decltype(mds)::element_type::reference>>);

        namespace rv = std::views;
        auto i_space = rv::iota(0uz, 2uz);
        auto j_space = rv::iota(0uz, 3uz);
        auto k_space = rv::iota(0uz, 4uz);

        auto bad_hash = [](const auto i, const auto j, const auto k, const auto idx) {
            return static_cast<element_type>(i + 10 * j + 100 * k + 1000 * idx[0] + 10000 * idx[1]);
        };

        for (const auto [i, j, k] : rv::cartesian_product(i_space, j_space, k_space)) {
            for (const auto idx : tyvi::sstd::geometric_index_space<2, 2>()) {
                mds[i, j, k][idx] = bad_hash(i, j, k, idx);
            }
        }

        const auto cbuff = buff;
        const auto cmds  = cbuff.mds();

        expect(std::is_const_v<
               std::remove_reference_t<typename decltype(cmds)::element_type::reference>>);

        for (const auto [i, j, k] : rv::cartesian_product(i_space, j_space, k_space)) {
            for (const auto idx : tyvi::sstd::geometric_index_space<2, 2>()) {
                expect(cmds[i, j, k][idx] == bad_hash(i, j, k, idx));
            }
        }
    };

    "mdgrid_buffer models thrust copyable range"_test = [] {
        /* thrust::copy(ExecutionPolicy,
                        InputIterator first,
                        InputIterator last,
                        OutputIterator result)

           InputIterator – must be a model of Input Iterator
                           and InputIterator's value_type must be convertible
                           to OutputIterator's value_type.

           OutputIterator – must be a model of Output Iterator. */

        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        auto buff        = testing_mdgrid_buffer(grid_mapping);
        const auto cbuff = testing_mdgrid_buffer(grid_mapping);

        namespace rn     = std::ranges;
        using Begin      = decltype(rn::begin(buff));
        using End        = decltype(rn::end(buff));
        using ConstBegin = decltype(rn::begin(cbuff));
        using ConstEnd   = decltype(rn::end(cbuff));

        expect(std::input_iterator<Begin>);
        expect(std::input_iterator<End>);
        expect(std::input_iterator<ConstBegin>);
        expect(std::input_iterator<ConstEnd>);

        using ValueType      = std::iterator_traits<Begin>::value_type;
        using ConstValueType = std::iterator_traits<ConstBegin>::value_type;

        expect(std::output_iterator<Begin, ValueType>);
        expect(not std::output_iterator<ConstBegin, ValueType>);
        expect(not std::output_iterator<ConstBegin, ConstValueType>);

        constexpr auto N = 2 * 2 * 2 * 3 * 4;
        expect(N == rn::distance(rn::begin(buff), rn::end(buff)));
        expect(N == rn::distance(rn::begin(cbuff), rn::end(cbuff)));
    };

    "thrust::copy between {device,host}_vector backed mdgrid_buffer"_test = [] {
        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        using host_mdg_buff   = tyvi::mdgrid_buffer<thrust::host_vector<int>,
                                                    element_extents,
                                                    element_layout_policy,
                                                    grid_extents,
                                                    grid_layout_policy>;
        using device_mdg_buff = tyvi::mdgrid_buffer<thrust::device_vector<int>,
                                                    element_extents,
                                                    element_layout_policy,
                                                    grid_extents,
                                                    grid_layout_policy>;

        auto host_A   = host_mdg_buff(grid_mapping);
        auto host_B   = host_mdg_buff(grid_mapping);
        auto device_A = device_mdg_buff(grid_mapping);
        auto device_B = device_mdg_buff(grid_mapping);

        thrust::sequence(host_A.begin(), host_A.end());
        thrust::copy(host_A.begin(), host_A.end(), device_A.begin());
        thrust::copy(device_A.begin(), device_A.end(), device_B.begin());
        thrust::copy(device_B.begin(), device_B.end(), host_B.begin());

        // Note that this is true because the layout policies are the same.
        expect(thrust::equal(host_A.begin(), host_A.end(), host_B.begin()));
    };

    "mdgrid_buffer syntax sugar ctor"_test = [] {
        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        auto buffA = testing_mdgrid_buffer(grid_mapping);
        auto buffB = testing_mdgrid_buffer(2, 3, 4);

        const auto eA = buffA.grid_extents();
        const auto eB = buffB.grid_extents();

        expect(eA == eB);
        expect(eA == grid_extents{ 2, 3, 4 });
    };

    "index_buffer is modifieable from thrust kernel"_test = [] {
        using device_mdg_buff = tyvi::mdgrid_buffer<thrust::device_vector<int>,
                                                    element_extents,
                                                    element_layout_policy,
                                                    grid_extents,
                                                    grid_layout_policy>;

        auto device_vec        = device_mdg_buff(3, 4, 2);
        const auto mds         = device_vec.mds();
        const auto index_space = tyvi::sstd::index_space(mds);

        // Note that element is 2x2 matrix.
        thrust::for_each(thrust::device,
                         index_space.begin(),
                         index_space.end(),
                         [=](const auto idx) {
                             mds[idx][0, 0] = static_cast<int>(idx[2]);
                             mds[idx][0, 1] = static_cast<int>(idx[2]);
                             mds[idx][1, 0] = static_cast<int>(idx[2]);
                             mds[idx][1, 1] = static_cast<int>(idx[2]);
                         });

        thrust::all_of(thrust::device, device_vec.begin(), device_vec.end(), [=](const auto x) {
            return x == 0 or x == 1;
        });
    };

    "mdgrid_buffer offers span to the underlying bytes"_test = [] {
        auto mdg_buff      = testing_mdgrid_buffer(4, 5, 6);
        const auto mdg_mds = mdg_buff.mds();

        for (const auto idx : tyvi::sstd::index_space(mdg_mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mdg_mds[idx])) {
                mdg_mds[idx][tidx] = 100;
            }
        }

        const auto mdg_span = mdg_buff.span();

        namespace rn = std::ranges;
        expect(rn::all_of(mdg_span, [](const auto x) { return x == 100; }));
    };

    "span from non-const mdgrid_buffer is non-const"_test = [] {
        auto mdg_buff = testing_mdgrid_buffer(4, 5, 6);
        const auto s  = mdg_buff.span();

        const auto& mdg_buff_constref = mdg_buff;
        const auto ss                 = mdg_buff_constref.span();

        expect(std::same_as<decltype(s)::element_type, element_type>);
        expect(std::same_as<decltype(ss)::element_type, const element_type>);
    };

    "mdgrid_buffer offers copy of underlying buffer"_test = [] {
        auto mdg_buff = testing_mdgrid_buffer(4, 5, 6);

        const auto buff_A = mdg_buff.underlying_buffer();

        // Make sure it is copy:
        expect(buff_A.data() != mdg_buff.span().data());

        namespace rn = std::ranges;
        expect(rn::equal(buff_A, mdg_buff.span()));

        const auto mdg_mds = mdg_buff.mds();
        for (const auto idx : tyvi::sstd::index_space(mdg_mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mdg_mds[idx])) {
                mdg_mds[idx][tidx] = 100;
            }
        }

        expect(not rn::equal(buff_A, mdg_buff.span()));
        expect(rn::equal(mdg_buff.underlying_buffer(), mdg_buff.span()));
    };

    "underlying buffer in mdgrid_buffer<V, ...> can be set with V"_test = [] {
        auto mdg_buff_A  = testing_mdgrid_buffer(4, 5, 6);
        const auto mds_A = mdg_buff_A.mds();

        for (const auto idx : tyvi::sstd::index_space(mds_A)) {
            for (const auto tidx : tyvi::sstd::index_space(mds_A[idx])) {
                mds_A[idx][tidx] = static_cast<int>(idx[0] * tidx[0]) + 3;
            }
        }

        auto mdg_buff_B = testing_mdgrid_buffer(4, 5, 6);

        namespace rn = std::ranges;
        expect(not rn::equal(mdg_buff_A.span(), mdg_buff_B.span()));

        mdg_buff_B.set_underlying_buffer(mdg_buff_A.underlying_buffer());

        expect(rn::equal(mdg_buff_A.span(), mdg_buff_B.span()));
    };

    "setting underlying buffer mdgrid_buffer<V, ...> with wrong sized buffer throws"_test = [] {
        auto mdg_buff_A = testing_mdgrid_buffer(4, 5, 6);
        expect(throws([&] { mdg_buff_A.set_underlying_buffer(vec{ 1, 2, 3, 4 }); }));
    };

    "thrust::{host,device}_vector backged mdgrid_buffer can give spans"_test = [] {
        using host_mdg_buff = tyvi::mdgrid_buffer<thrust::host_vector<int>,
                                                  element_extents,
                                                  element_layout_policy,
                                                  grid_extents,
                                                  grid_layout_policy>;

        using device_mdg_buff = tyvi::mdgrid_buffer<thrust::device_vector<int>,
                                                    element_extents,
                                                    element_layout_policy,
                                                    grid_extents,
                                                    grid_layout_policy>;

        auto mdgb_host          = host_mdg_buff(2, 3, 4);
        auto mdgb_device        = device_mdg_buff(2, 3, 4);
        const auto cmdgb_host   = host_mdg_buff(2, 3, 4);
        const auto cmdgb_device = device_mdg_buff(2, 3, 4);

        std::ignore = mdgb_host.span();
        std::ignore = mdgb_device.span();
        std::ignore = cmdgb_host.span();
        std::ignore = cmdgb_device.span();

        expect(true);
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}

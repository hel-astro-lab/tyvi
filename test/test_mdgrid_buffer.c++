#include <boost/ut.hpp> // import boost.ut;

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>

#include <experimental/mdspan>

#include "thrust/copy.h"
#include "thrust/device_vector.h"
#include "thrust/host_vector.h"
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
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}

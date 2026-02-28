#include <boost/ut.hpp> // import boost.ut;

#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <type_traits>

#include "tyvi/dynamic_array_cpu.h"
#include "tyvi/mdgrid_buffer.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_buffer_cpu"> _ = [] {
    using element_type          = int;
    using vec                   = tyvi::dynamic_array<element_type>;
    using element_extents       = tyvi::sstd::geometric_extents<2, 2>;
    using element_layout_policy = std::layout_right;
    using grid_extents          = std::dextents<std::size_t, 3>;
    using grid_layout_policy    = std::layout_right;

    using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                      element_extents,
                                                      element_layout_policy,
                                                      grid_extents,
                                                      grid_layout_policy>;

    "mdgrid_buffer<dynamic_array> is constructible"_test = [] {
        [[maybe_unused]]
        auto _ = testing_mdgrid_buffer(2, 3, 4);
        expect(true);
    };

    "mdgrid_buffer<dynamic_array> mds() is usable"_test = [] {
        auto buff      = testing_mdgrid_buffer(2, 3, 4);
        const auto mds = buff.mds();

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) { mds[idx][tidx] = 42; }
        }

        const auto cbuff = buff;
        const auto cmds  = cbuff.mds();

        for (const auto idx : tyvi::sstd::index_space(cmds)) {
            for (const auto tidx : tyvi::sstd::index_space(cmds[idx])) {
                expect(cmds[idx][tidx] == 42);
            }
        }
    };

    "mdgrid_buffer<dynamic_array> span() works"_test = [] {
        auto buff      = testing_mdgrid_buffer(4, 5, 6);
        const auto mds = buff.mds();

        for (const auto idx : tyvi::sstd::index_space(mds)) {
            for (const auto tidx : tyvi::sstd::index_space(mds[idx])) { mds[idx][tidx] = 100; }
        }

        const auto s = buff.span();
        namespace rn = std::ranges;
        expect(rn::all_of(s, [](const auto x) { return x == 100; }));
    };

    "mdgrid_buffer<dynamic_array> invalidating_resize"_test = [] {
        auto buff  = testing_mdgrid_buffer(4, 5, 6);
        const auto e = grid_extents{ 2, 3, 4 };
        buff.invalidating_resize(e);
        expect(buff.grid_extents() == e);
        expect(buff.mds().extents() == e);
    };

    "mdgrid_buffer<dynamic_array> set_underlying_buffer"_test = [] {
        auto buff_A      = testing_mdgrid_buffer(4, 5, 6);
        const auto mds_A = buff_A.mds();

        for (const auto idx : tyvi::sstd::index_space(mds_A)) {
            for (const auto tidx : tyvi::sstd::index_space(mds_A[idx])) {
                mds_A[idx][tidx] = static_cast<int>(idx[0] * tidx[0]) + 3;
            }
        }

        auto buff_B = testing_mdgrid_buffer(4, 5, 6);

        namespace rn = std::ranges;
        expect(not rn::equal(buff_A.span(), buff_B.span()));

        buff_B.set_underlying_buffer(buff_A.underlying_buffer());

        expect(rn::equal(buff_A.span(), buff_B.span()));
    };

    "mdgrid_buffer<dynamic_array> offers copy of underlying buffer"_test = [] {
        auto buff = testing_mdgrid_buffer(4, 5, 6);

        const auto buf_A = buff.underlying_buffer();

        // Make sure it is a copy:
        expect(buf_A.data() != buff.span().data());

        namespace rn = std::ranges;
        expect(rn::equal(buf_A, buff.span()));
    };

    "mdgrid_buffer<dynamic_array> is constructible from grid extents"_test = [] {
        auto mdgbA = testing_mdgrid_buffer(4, 2, 1);
        auto mdgbB = testing_mdgrid_buffer(mdgbA.grid_extents());
        expect(mdgbA.grid_extents() == mdgbB.grid_extents());
    };

    "mdgrid_buffer<dynamic_array> gives correct element extents"_test = [] {
        auto mdgb = testing_mdgrid_buffer(4, 2, 1);
        expect(mdgb.element_extents() == element_extents{});
    };

    "mdgrid_buffer<dynamic_array> mds() is const correct"_test = [] {
        auto buff      = testing_mdgrid_buffer(2, 3, 4);
        const auto mds = buff.mds();

        expect(not std::is_const_v<
               std::remove_reference_t<typename decltype(mds)::element_type::reference>>);

        const auto cbuff = buff;
        const auto cmds  = cbuff.mds();

        expect(std::is_const_v<
               std::remove_reference_t<typename decltype(cmds)::element_type::reference>>);
    };

    "mdgrid_buffer<dynamic_array> syntax sugar ctor"_test = [] {
        const auto grid_mapping =
            grid_layout_policy::template mapping<grid_extents>(grid_extents{ 2, 3, 4 });

        auto buffA = testing_mdgrid_buffer(grid_mapping);
        auto buffB = testing_mdgrid_buffer(2, 3, 4);

        const auto eA = buffA.grid_extents();
        const auto eB = buffB.grid_extents();

        expect(eA == eB);
        expect(eA == grid_extents{ 2, 3, 4 });
    };

    "mdgrid_buffer<dynamic_array> span const correctness"_test = [] {
        auto mdg_buff = testing_mdgrid_buffer(4, 5, 6);
        const auto s  = mdg_buff.span();

        const auto& mdg_buff_constref = mdg_buff;
        const auto ss                 = mdg_buff_constref.span();

        expect(std::same_as<decltype(s)::element_type, element_type>);
        expect(std::same_as<decltype(ss)::element_type, const element_type>);
    };

    "mdgrid_buffer<dynamic_array> set_underlying_buffer with wrong size throws"_test = [] {
        auto mdg_buff  = testing_mdgrid_buffer(4, 5, 6);
        auto wrong_buf = vec(4); // wrong size: 4 instead of 4*5*6*2*2 = 480
        expect(throws([&] { mdg_buff.set_underlying_buffer(std::move(wrong_buf)); }));
    };

    "mdgrid_buffer<dynamic_array> models copyable range"_test = [] {
        auto buff        = testing_mdgrid_buffer(2, 3, 4);
        const auto cbuff = testing_mdgrid_buffer(2, 3, 4);

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
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}

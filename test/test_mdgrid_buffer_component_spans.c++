#include <boost/ut.hpp> // import boost.ut;

#include <algorithm>
#include <print>
#include <vector>

#include "tyvi/mdgrid_buffer.h"
#include "tyvi/mdspan.h"

namespace {
using namespace boost::ut;

[[maybe_unused]]
const suite<"mdgrid_buffer component spans"> _ = [] {
    using element_type          = int;
    using vec                   = std::vector<element_type>;
    using element_extents       = tyvi::sstd::geometric_extents<2, 2>;
    using element_layout_policy = std::layout_right;
    using grid_layout_policy    = std::layout_right;

    "component span is correct size for 2D grid"_test = [] {
        using grid_extents          = std::dextents<std::size_t, 2>;
        using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                          element_extents,
                                                          element_layout_policy,
                                                          grid_extents,
                                                          grid_layout_policy>;

        auto mdg_buff     = testing_mdgrid_buffer(10, 13);
        const auto span00 = mdg_buff.component_cspan<0, 0>();
        const auto span10 = mdg_buff.component_cspan<1, 0>();
        const auto span01 = mdg_buff.component_cspan<{ 1, 0 }>();
        const auto span11 = mdg_buff.component_cspan<{ 1, 1 }>();
        expect(span00.size() == 10uz * 13uz);
        expect(span10.size() == 10uz * 13uz);
        expect(span01.size() == 10uz * 13uz);
        expect(span11.size() == 10uz * 13uz);
    };

    "modification through component span is correct"_test = [] {
        using grid_extents          = std::dextents<std::size_t, 2>;
        using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                          element_extents,
                                                          element_layout_policy,
                                                          grid_extents,
                                                          grid_layout_policy>;

        auto mdg_buff     = testing_mdgrid_buffer(10, 13);
        const auto span00 = mdg_buff.component_span<0, 0>();
        const auto span10 = mdg_buff.component_span<1, 0>();
        const auto span01 = mdg_buff.component_span<{ 0, 1 }>();
        const auto span11 = mdg_buff.component_span<{ 1, 1 }>();

        std::ranges::fill(span00, 1);
        std::ranges::fill(span01, 2);
        std::ranges::fill(span10, 3);
        std::ranges::fill(span11, 4);

        const auto mds = mdg_buff.mds();
        for (const auto idx : tyvi::sstd::index_space(mds)) {
            expect(mds[idx][0, 0] == 1) << mds[idx][0, 0];
            expect(mds[idx][0, 1] == 2) << mds[idx][0, 1];
            expect(mds[idx][1, 0] == 3) << mds[idx][1, 0];
            expect(mds[idx][1, 1] == 4) << mds[idx][1, 1];
        }
    };

    "mdgrid_buffer is sortable through zip view of compnents"_test = [] {
        using grid_extents          = std::dextents<std::size_t, 2>;
        using testing_mdgrid_buffer = tyvi::mdgrid_buffer<vec,
                                                          element_extents,
                                                          element_layout_policy,
                                                          grid_extents,
                                                          grid_layout_policy>;

        auto mdg_buff = testing_mdgrid_buffer(10, 13);

        const auto mds = mdg_buff.mds();
        for (const auto idx : tyvi::sstd::index_space(mds)) {
            mds[idx][0, 0] = static_cast<element_type>(100 * idx[1]);
            mds[idx][0, 1] = static_cast<element_type>(idx[0]);
            mds[idx][1, 0] = static_cast<element_type>(idx[1]);
            mds[idx][1, 1] = static_cast<element_type>(idx[0]);
        }

        auto in_acending_trace = [&] {
            const auto index_space = tyvi::sstd::index_space(mds);
            auto previous          = mds[index_space[0]][0, 0] + mds[index_space[0]][1, 1];
            for (const auto idx : tyvi::sstd::index_space(mds) | std::views::drop(1)) {
                const auto current = mds[idx][0, 0] + mds[idx][1, 1];
                if (previous > current) { return false; }
                previous = current;
            }
            return true;
        };

        expect(not in_acending_trace());

        const auto span00 = mdg_buff.component_span<0, 0>();
        const auto span10 = mdg_buff.component_span<1, 0>();
        const auto span01 = mdg_buff.component_span<{ 0, 0 }>();
        const auto span11 = mdg_buff.component_span<{ 1, 1 }>();

        const auto zip = std::views::zip(span11, span01, span10, span00);

        std::ranges::sort(zip, [](const auto& lhs, const auto& rhs) {
            const auto lhs_trace = std::get<0>(lhs) + std::get<3>(lhs);
            const auto rhs_trace = std::get<0>(rhs) + std::get<3>(rhs);
            return lhs_trace < rhs_trace;
        });

        expect(in_acending_trace());
    };
};

} // namespace

int
main(int argc, const char** argv) {
    return static_cast<int>(cfg<override>.run(run_cfg{ .argc = argc, .argv = argv }));
}

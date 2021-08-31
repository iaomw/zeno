#pragma once

#include <cstdint>

namespace zinc {

template <class ValT, class GetF, class SumF>
inline ValT omp_parallel_reduce(size_t num, ValT init, GetF const &get, SumF const &sum) {
    size_t nproc = std::max(64, (int)(num / 100000));
    std::vector<ValT> tls(nproc);
    for (size_t p = 0; p < nproc; p++) {
        tls[p] = init;
    }
#pragma omp parallel for
    for (intptr_t p = 0; p < nproc; p++) {
        size_t i0 = num / nproc * p;
        size_t i1 = p == nproc - 1 ? num : num / nproc * (p + 1);
        for (size_t i = i0; i < i1; i++) {
            tls[p] = sum(tls[p], get(i));
        }
    }
    ValT ret = init;
    for (size_t p = 0; p < nproc; p++) {
        ret = sum(ret, tls[p]);
    }
    return ret;
}

}

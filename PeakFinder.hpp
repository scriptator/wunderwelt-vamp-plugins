//
//  PeakFinder.hpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 19.02.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#ifndef PeakFinder_hpp
#define PeakFinder_hpp

#include <stdio.h>
#include <vector>

namespace PeakFinder {

    template <class Iterator, class T = typename std::iterator_traits<Iterator>::value_type>
    std::vector<std::pair<size_t, T>> findPeaksNaive(Iterator begin, Iterator end, size_t n);
    
    template<class T> T foo(T in);

}

/////////// implementation of template functions

using std::vector;
using std::pair;

template <class Iterator, class T>
std::vector<std::pair<size_t, T>> PeakFinder::findPeaksNaive(Iterator begin, Iterator end, size_t n) {
    vector<pair<size_t, T>> outputBuffer;
    vector<pair<T, size_t>> indexedInput;
    size_t index = 0;
    
    for (auto it = begin; it < end; ++it) {
        indexedInput.push_back(pair<T, size_t>(index++, *it));
    }
    
    // find out the n biggest elements, sorted descending
    std::sort(indexedInput.rbegin(), indexedInput.rend());
    for (auto it=indexedInput.begin(); it < indexedInput.begin() + n; ++it) {
        outputBuffer.push_back(pair<size_t, T>(it->second, it->first));
    }

    return outputBuffer;
}

#endif /* PeakFinder_hpp */

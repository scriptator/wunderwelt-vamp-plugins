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

    // naively find peaks by returning the n biggest elements
    template <class Iterator, class T = typename std::iterator_traits<Iterator>::value_type>
    std::vector<std::pair<size_t, T>> findPeaksNaive(Iterator begin, Iterator end, size_t n);

    // find peaks by returning those elements where the surrounding is at least one threshold lower
    template <class Iterator, class T = typename std::iterator_traits<Iterator>::value_type>
    std::vector<std::pair<size_t, T>> findPeaksThreshold(Iterator begin, Iterator end, T threshold);


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

enum SignalDirection {
    ascending,
    descending,
    stagnating
};

template <class Iterator, class T>
std::vector<std::pair<size_t, T>> PeakFinder::findPeaksThreshold(Iterator begin, Iterator end, T threshold) {
    vector<pair<size_t, T>> outputBuffer;

    SignalDirection direction = stagnating;
    size_t index = 0;

    T previous = *begin;
    T current;
    
    pair<size_t, T> lastValley = pair<size_t, T>(0, previous);
    pair<size_t, T> candidate;
    bool validCandidate = false;

    for (auto it = begin; it < end; ++it) {
        current = *it;

        if (current < previous) {
            direction = descending;
            if (previous - lastValley.second > threshold) {
                candidate = pair<size_t, T>(index-1, previous);
                validCandidate = true;
            }
        } else if (current > previous) {
            if (direction != ascending) {
                direction = ascending;
                lastValley.first = index - 1;
                lastValley.second = previous;

                if (validCandidate && candidate.second - previous > threshold) {
                    outputBuffer.push_back(candidate);
                }
                validCandidate = false;
            }
        } else {
            direction = stagnating;
        }

        previous = current;
        index++;
    }

    return outputBuffer;
}


#endif /* PeakFinder_hpp */

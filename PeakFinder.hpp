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
#include <algorithm>

namespace PeakFinder {

    template<class T> struct Peak {
        T value;
        T height;
        size_t position;
        double interpolatedPosition;
    };

    // naively find peaks by returning the n biggest elements
    template <class Iterator, class T = typename std::iterator_traits<Iterator>::value_type>
    std::vector<std::pair<size_t, T>> findPeaksNaive(Iterator begin, Iterator end, size_t n);

    // find peaks by returning those elements where the surrounding is at least one threshold lower
    template <class Iterator, class T = typename std::iterator_traits<Iterator>::value_type>
    std::vector<Peak<T>> findPeaksThreshold(Iterator begin, Iterator end, T threshold);

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
std::vector<PeakFinder::Peak<T>> PeakFinder::findPeaksThreshold(Iterator begin, Iterator end, T threshold) {
    vector<PeakFinder::Peak<T>> outputBuffer;

    SignalDirection direction = stagnating;
    size_t index = 0;

    T previous = *begin;
    T current;
    T height;
    
    pair<size_t, T> lastValley = pair<size_t, T>(0, previous);
    PeakFinder::Peak<T> candidate;
    bool validCandidate = false;

    for (auto it = begin; it < end; ++it) {
        current = *it;

        if (current < previous) {
            direction = descending;
            height = previous - lastValley.second;
            if (height > threshold) {
                double interpolatedPosition = index - 1;
                candidate = PeakFinder::Peak<T>
                    {.value=previous, .height=height, .position=index-1, .interpolatedPosition=interpolatedPosition};
                validCandidate = true;
            }
        } else if (current > previous) {
            if (direction != ascending) {
                direction = ascending;
                lastValley.first = index - 1;
                lastValley.second = previous;

                height = candidate.value - previous;
                if (validCandidate && height > threshold) {
                    candidate.height = std::min(candidate.height, height);
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

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
#include <vamp-sdk/Plugin.h>

using Vamp::RealTime;

namespace PeakFinder {

    template<class T> struct Peak {
        T value;
        RealTime timestamp;
        T height;
        size_t position;
        double interpolatedPosition;

        Peak<T>(): value(-1), height(-1), position(-1), interpolatedPosition(-1), timestamp(RealTime::zeroTime) {}

        Peak<T>(T value, T height, size_t position, double interpolatedPosition, _VampPlugin::Vamp::RealTime timestamp):
            value(value), height(height), position(position), interpolatedPosition(interpolatedPosition), timestamp(timestamp) {}
        
        Peak<T>(const Peak<T> & other):
            value(other.value), height(other.height), position(other.position), interpolatedPosition(other.interpolatedPosition),
            timestamp(other.timestamp)
        { }
    };

    // find peaks by returning those elements where the next valleys on both sides are at least one threshold lower
    // the provided timestamp is set on the peak for later reference
    template <class Iterator, class T = typename std::iterator_traits<Iterator>::value_type>
    std::vector<Peak<T>*> findPeaksThreshold(Iterator begin, Iterator end, T threshold, RealTime timestamp);

    enum SignalDirection {
        ascending,
        descending,
        stagnating
    };
}

/////////// implementation of template functions

using std::vector;
using std::pair;

template <class Iterator, class T>
std::vector<PeakFinder::Peak<T>*> PeakFinder::findPeaksThreshold(Iterator begin, Iterator end, T threshold, RealTime timestamp) {
    vector<PeakFinder::Peak<T>*> outputBuffer;

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
            if (direction != SignalDirection::descending) {     // direction change downwards occurred
                direction = SignalDirection::descending;
                height = previous - lastValley.second;
                // if the height is sufficient, make it a candidate
                if (height >= threshold) {
                    candidate = PeakFinder::Peak<float>(previous, height, index - 1, index - 1, timestamp);
                    validCandidate = true;
                }
            }
        } else if (current > previous) {
            if (direction != SignalDirection::ascending) {      // direction change upwards occurred
                direction = SignalDirection::ascending;
                lastValley.first = index - 1;
                lastValley.second = previous;
                
                if (validCandidate) {
                    height = candidate.value - previous;
                    // if the height is sufficient, make the candidate a peak
                    if (height >= threshold) {
                        candidate.height = std::min(candidate.height, height);
                        outputBuffer.push_back(new PeakFinder::Peak<float>(candidate));
                    }
                }
                validCandidate = false;
            }
        } else {
            direction = SignalDirection::stagnating;
        }

        previous = current;
        index++;
    }

    return outputBuffer;
}


#endif /* PeakFinder_hpp */

//
//  PeakHistory.cpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 25.02.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#include "PeakHistory.hpp"
#include <math.h>

template<typename T> PeakHistory<T>::PeakHistory(size_t broadestAllowedInterruption):
    peaks(std::vector<const Peak<T>*>()),
    broadestAllowedInterruption(broadestAllowedInterruption),
    sumOfHeights(0),
    total(0),
    missed(0),
    recentlyMissed(0),
    alive(true) {
}

template<typename T> PeakHistory<T>::PeakHistory(Peak<T> *initalPeak, size_t broadestAllowedInterruption):
    PeakHistory<T>::PeakHistory(broadestAllowedInterruption) {
        this->addPeak(initalPeak);
}

template<typename T> void PeakHistory<T>::addPeak(const Peak<T> *peak) {
    this->peaks.push_back(peak);
    recentlyMissed = 0;
    sumOfHeights += peak->height;
    this->total++;
}

template<typename T> void PeakHistory<T>::noPeak() {
    this->missed++;
    this->recentlyMissed++;
    this->total++;
}

template<typename T> const Peak<T>* PeakHistory<T>::getStableBegin() {
    size_t stableLength = 0;
    double stableValue = 0.0;
    
    for (const Peak<T>* peak : this->peaks) {
        if (stableValue == peak->interpolatedPosition) {
            stableLength++;
        } else {
            stableLength = 0;
            stableValue = peak->interpolatedPosition;
        }
        
        if (stableLength >= STABLE_LENGTH_MINIMUM) {
            return peak;
        }
    }
    
    // no stable streak found --> return NULL
    return nullptr;
}

template<typename T> const Peak<T>* PeakHistory<T>::getStableEnd() {
    size_t stableLength = 0;
    double stableValue = 0.0;
    
    for (auto it = peaks.rbegin(); it < peaks.rend(); ++it) {
        auto peak = *it;
        if (fabs(stableValue - peak->interpolatedPosition) <= 1) {
            stableLength++;
        } else {
            stableLength = 0;
            stableValue = peak->interpolatedPosition;
        }
        
        if (stableLength > STABLE_LENGTH_MINIMUM) {
            return peak;
        }
    }
    
    // no stable streak found --> return NULL
    return nullptr;
}

template<typename T> void PeakHistory<T>::getInterpolatedPositionHistory(std::vector<std::pair<Vamp::RealTime, double>>& resultVector) const {
    for (auto peak : this->peaks) {
        resultVector.push_back(std::pair<Vamp::RealTime, double>(peak->timestamp, peak->interpolatedPosition));
    }
}


// template initializations
template class PeakHistory<float>;

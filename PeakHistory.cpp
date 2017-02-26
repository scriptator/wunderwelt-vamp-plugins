//
//  PeakHistory.cpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 25.02.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#include "PeakHistory.hpp"

template<typename T> PeakHistory<T>::PeakHistory(size_t broadestAllowedInterruption):
    peaks(std::vector<const PeakFinder::Peak<T>*>()),
    broadestAllowedInterruption(broadestAllowedInterruption),
    total(0),
    missed(0),
    recentlyMissed(0),
    alive(true) {
}

template<typename T> PeakHistory<T>::PeakHistory(PeakFinder::Peak<T> *initalPeak, size_t broadestAllowedInterruption):
    PeakHistory<T>::PeakHistory(broadestAllowedInterruption) {
        this->peaks = std::vector<const PeakFinder::Peak<T>*>{initalPeak};
}

template<typename T> void PeakHistory<T>::addPeak(const PeakFinder::Peak<T> *peak) {
    this->peaks.push_back(peak);
    recentlyMissed = 0;
    this->total++;
}

template<typename T> void PeakHistory<T>::noPeak() {
    this->missed++;
    this->recentlyMissed++;
    this->total++;
}

template<typename T> void PeakHistory<T>::getInterpolatedPositionHistory(std::vector<double>& resultVector) const {
    for (auto peak : this->peaks) {
        resultVector.push_back(peak->interpolatedPosition);
    }
}


// template initializations
template class PeakHistory<float>;

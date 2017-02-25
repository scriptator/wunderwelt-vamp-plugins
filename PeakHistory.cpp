//
//  PeakHistory.cpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 25.02.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#include "PeakHistory.hpp"

template<typename T> PeakHistory<T>::PeakHistory():
    peaks(std::vector<PeakFinder::Peak<T>>()) {
};

template<typename T> void PeakHistory<T>::addPeak(const PeakFinder::Peak<T>& peak) {
    this->peaks.emplace_back(peak);
}

template<typename T> void PeakHistory<T>::getInterpolatedPositionHistory(std::vector<double>& resultVector) const {
    for (auto it = this->peaks.begin(); it < this->peaks.end(); ++it) {
        resultVector.push_back(it->interpolatedPosition);
    }
}

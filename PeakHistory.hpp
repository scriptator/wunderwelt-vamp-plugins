//
//  PeakHistory.hpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 25.02.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#ifndef PeakHistory_hpp
#define PeakHistory_hpp

#include <stdio.h>
#include "PeakFinder.hpp"

template<typename T> class PeakHistory {
    
public:
    PeakHistory();

    void addPeak(const PeakFinder::Peak<T>& peak);

    void getInterpolatedPositionHistory(std::vector<double>& resultVector) const;

    PeakFinder::Peak<T> getFirst() const {
        return this->peaks.at(0);
    }

    PeakFinder::Peak<T> getLast() const {
        return this->peaks.at(peaks.size() - 1);
    }

private:
    const std::vector<PeakFinder::Peak<T>> peaks;
};

#endif /* PeakHistory_hpp */

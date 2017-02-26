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
    PeakHistory(size_t broadestAllowedInterruption);
    PeakHistory(PeakFinder::Peak<T> *initalPeak, size_t broadestAllowedInterruption);

    void addPeak(const PeakFinder::Peak<T> *peak);
    void noPeak();

    void getInterpolatedPositionHistory(std::vector<double>& resultVector) const;

    const PeakFinder::Peak<T>* getFirst() const {
        return this->peaks.at(0);
    }

    const PeakFinder::Peak<T>* getLast() const {
        return this->peaks.at(peaks.size() - 1);
    }
    
    bool isAlive() {
        alive = alive && recentlyMissed < broadestAllowedInterruption;
        return alive;
    }
    
    size_t size() {
        return this->total;
    }
    
    size_t numberOfMissed() {
        return this->missed;
    }

private:
    std::vector<const PeakFinder::Peak<T> *> peaks;

    size_t broadestAllowedInterruption;
    
    size_t total;
    size_t missed;
    size_t recentlyMissed;
    
    bool alive;
};

#endif /* PeakHistory_hpp */

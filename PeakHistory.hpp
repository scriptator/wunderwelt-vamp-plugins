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
#include <vamp-sdk/Plugin.h>

# define STABLE_LENGTH_MINIMUM 5

using PeakFinder::Peak;

template<typename T> class PeakHistory {
    
public:
    PeakHistory(size_t broadestAllowedInterruption);
    PeakHistory(Peak<T> *initalPeak, size_t broadestAllowedInterruption);

    void addPeak(const Peak<T> *peak);
    void noPeak();

    void getInterpolatedPositionHistory(std::vector<std::pair<Vamp::RealTime, double>>& resultVector) const;
    
    double getAveragePeakHeight() const {
        return this->sumOfHeights / total;
    }

    const Peak<T>* getFirst() const {
        return this->peaks.at(0);
    }

    const Peak<T>* getLast() const {
        return this->peaks.at(peaks.size() - 1);
    }
    
    const Peak<T>* getStableBegin();
    
    const Peak<T>* getStableEnd();
    
    bool isAlive() {
        alive = alive && recentlyMissed < broadestAllowedInterruption;
        return alive;
    }
    
    size_t size() const {
        return this->total;
    }
    
    size_t numberOfMissed() const {
        return this->missed;
    }

private:
    std::vector<const Peak<T> *> peaks;

    size_t broadestAllowedInterruption;
    
    size_t total;
    size_t missed;
    size_t recentlyMissed;
    
    double sumOfHeights;
    
    bool alive;
};

#endif /* PeakHistory_hpp */

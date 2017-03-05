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

# define STABLE_LENGTH_MINIMUM 3

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
    
    double getTotalPeakHeight() const {
        return this->sumOfHeights;
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
        if (!alive) {
            auto begin = this->getStableBegin();
            auto end = this->getStableEnd();
            alive = alive || (begin && end && begin->timestamp.sec < 2 && end->timestamp.sec >= 4 && begin->interpolatedPosition > end->interpolatedPosition);
        }
        if (!alive) {
            std::cerr << getLast()->interpolatedPosition << " (time " << getLast()->timestamp.sec*1000 + getLast()->timestamp.msec() << ", height " << getAveragePeakHeight() << ") is not alive any more :( \n";
        }
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

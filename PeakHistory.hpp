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

// PeakHistory is responsible for grouping together a set of peaks over time which probably
// belong together. It provides convenient access to the set by the following set of functions
template<typename T> class PeakHistory {

public:
    PeakHistory(size_t broadestAllowedInterruption);
    PeakHistory(Peak<T> *initalPeak, size_t broadestAllowedInterruption);

    // add a peak to the peak history, resets the number of recently missed peaks
    void addPeak(const Peak<T> *peak);

    // add nothing but tell the PeakHistory that a peak at this position was not found
    void noPeak();

    // convenient access to the list of values together with timestamps
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

    // return a Peak* within the stable beginning of the history or nullptr if there is none
    // stable means exactly the same value for at least three times
    const Peak<T>* getStableBegin();

    // returns a peak within the stable end of the history of nullptr if there is none
    // stable means a +-1 range for at least three times
    const Peak<T>* getStableEnd();

    // returns whether this peak history is still valid
    // it is alive if there were not too many peaks missed or there is a stable beginning and end at the right time
    bool isAlive() {
        alive = alive && recentlyMissed < broadestAllowedInterruption;
        if (!alive) {
            auto begin = this->getStableBegin();
            auto end = this->getStableEnd();
            alive = alive || (begin && end && begin->timestamp.sec < 2 && end->timestamp.sec >= 4 && begin->interpolatedPosition > end->interpolatedPosition);
        }
//        if (!alive) {
//            std::cerr << getLast()->interpolatedPosition << " (time " << getLast()->timestamp.sec*1000 + getLast()->timestamp.msec() << ", height " << getAveragePeakHeight() << ") is not alive any more :( \n";
//        }
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

    double sumOfHeights;

    size_t total;
    size_t missed;
    size_t recentlyMissed;
     
    bool alive;
};

#endif /* PeakHistory_hpp */

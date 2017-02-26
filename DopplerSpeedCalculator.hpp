//
//  wunderwelt-vamp-plugin.hpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 21.01.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#ifndef doppler_speed_calculator_hpp
#define doppler_speed_calculator_hpp

#include <stdio.h>
#include <vamp-sdk/Plugin.h>
#include <iostream>
#include <fstream>
#include <complex>

#include "PeakFinder.hpp"
#include "PeakHistory.hpp"

#define SPEED_OF_SOUND 343
#define PEAK_DETECTION_TIME 1000 // ms
#define UPPER_THRESHOLD_FREQUENCY 3000 // Hz
#define MAX_BIN_JUMP 2 // bins
#define BROADEST_ALLOWED_INTERRUPTION 5 // steps

using std::string;

/// Calculates the speed of a moving source relative to a still measuring point given a before-frequency and an after-frequency.
/// The frequencies may be in any unit, the speed is returned in km/h
template<typename T> static T dopplerSpeedMovingSource(T approachingFreq, T leavingFreq) {
    T speedInMetersPerSecond = (approachingFreq - leavingFreq) / (approachingFreq + leavingFreq) * SPEED_OF_SOUND; // in m/s
    return speedInMetersPerSecond * 3.6;    // in km/h
}

class DopplerSpeedCalculator : public Vamp::Plugin {

public:
    DopplerSpeedCalculator(float inputSampleRate);
    ~DopplerSpeedCalculator ();

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;
    
    InputDomain getInputDomain() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;
    
    ParameterList getParameterDescriptors() const;
    float getParameter(string identifier) const;
    void setParameter(string identifier, float value);
    
    ProgramList getPrograms() const;
    string getCurrentProgram() const;
    void selectProgram(string name);
    
    OutputList getOutputDescriptors() const;
    
    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();
    
    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);
    
    FeatureSet getRemainingFeatures();
    
    /// calculates the center frequency of a bin (i.e. the index of the bin or an interpolated value inbetween)
    template<typename T> float getFrequencyForBin(T bin) {
        return (1.0f * this->m_inputSampleRate * bin) / this->m_blockSize;
    };
    
    /// Calculates the normalized magnitude given a complex number
    /// formula reference https://groups.google.com/d/msg/comp.dsp/cZsS1ftN5oI/rEjHXKTxgv8J
    template<typename T> float calcNormalizedMagnitude(std::complex<T> complexVal) {
        return std::abs(complexVal) * 2 / (this->m_blockSize);
    };

    enum CarState {
        UNKNOWN,
        APPROACHING_STABLE,
        UNSTABLE,
        LEAVING_STABLE
    };

private:
    size_t m_blocksProcessed;
    size_t m_stepSize;
    size_t m_blockSize;
    mutable std::map<std::string, int> m_outputNumbers;
    std::map<std::string, float> m_parameterValues;

    CarState carState;
    _VampPlugin::Vamp::RealTime stableBegin;
    _VampPlugin::Vamp::RealTime stableEnd;
    
    void tracePeaks(const std::vector<PeakFinder::Peak<float> *> &peaks, bool allowNew);
    // store the history of all found peaks
    std::vector<std::vector<PeakFinder::Peak<float>*>> peakMatrix;
    std::vector<PeakHistory<float>> peakHistories;

    /// csv files for debug purposes which get (over)written on every execution
    std::ofstream csvfile;

};

#endif /* doppler_speed_calculator_hpp */

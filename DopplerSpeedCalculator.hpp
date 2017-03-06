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

// Parameter Identifiers
#define DEBUG_CSV_FILES "write-debug-csv"
#define PEAK_DETECTION_TIME_ID "peak-detection-time"
#define PEAK_DETECTION_HEIGHT_THRESHOLD_ID "peak-detection-height-threshold"
#define PEAK_TRACING_HEIGHT_THRESHOLD_ID "peak-tracing-height-threshold"
#define UPPER_THRESHOLD_FREQUENCY_ID "upper-threshold-frequency"
#define MAX_BIN_JUMP_ID "max-bin-jump"
#define BROADEST_ALLOWED_INTERRUPTION_ID "broadest-interruption"
#define MOVING_FFT_AVERAGE_WIDTH_ID "moving-fft-average-width"

// Parameter Default Values
#define PEAK_DETECTION_TIME 1.5 // s
#define PEAK_DETECTION_HEIGHT_THRESHOLD 15.0 // dB
#define PEAK_TRACING_HEIGHT_THRESHOLD 5.0 // dB
#define UPPER_THRESHOLD_FREQUENCY 1500 // Hz
#define MAX_BIN_JUMP 5 // bins
#define BROADEST_ALLOWED_INTERRUPTION 10 // steps
#define MOVING_FFT_AVERAGE_WIDTH 4

// Other constants
#define SPEED_OF_SOUND 343

using std::string;
using PeakFinder::Peak;

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
    
    /// calculates bin number of a given frequency
    template<typename T> size_t getBinForFrequency(T frequency) {
        return (1.0 * frequency * m_blockSize) / this->m_inputSampleRate;
    };
    
    /// Calculates the normalized magnitude in dB given a complex number
    /// formula reference https://groups.google.com/d/msg/comp.dsp/cZsS1ftN5oI/rEjHXKTxgv8J
    template<typename T> float calcNormalizedMagnitude(std::complex<T> complexVal) {
        return normalizeMagnitude(std::abs(complexVal));
    };
    
    /// Calculates the normalized magnitude in dB given the magnitude as a number
    /// formula reference https://groups.google.com/d/msg/comp.dsp/cZsS1ftN5oI/rEjHXKTxgv8J
    template<typename T> float normalizeMagnitude(T magnitude) {
        auto mag = magnitude * 2 / (this->m_blockSize);
        return 20 * log10(mag);
    };


private:
    size_t m_blocksProcessed;
    size_t m_stepSize;
    size_t m_blockSize;
    mutable std::map<std::string, int> m_outputNumbers;
    std::map<std::string, float> m_parameterValues;
    
    vector<vector<float>> fftData;
    
    // function which traces peaks over time
    void tracePeaks(const std::vector<Peak<float> *> &peaks, bool allowNew);
    
    // store the history of all found peaks
    std::vector<std::vector<Peak<float>*>> peakMatrix;
    std::vector<PeakHistory<float>> peakHistories;

    /// csv files for debug purposes which get (over)written on every execution
    std::ofstream csvfile;

};

#endif /* doppler_speed_calculator_hpp */

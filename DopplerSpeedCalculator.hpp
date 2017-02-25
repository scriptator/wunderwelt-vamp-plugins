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

using std::string;

class DopplerSpeedCalculator : public Vamp::Plugin {

public:
    DopplerSpeedCalculator(float inputSampleRate);

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
    
private:
    size_t m_blocksProcessed;
    size_t m_stepSize;
    size_t m_blockSize;
    std::map<float, std::vector<float>*> m_frequencyTimeline;
    mutable std::map<std::string, int> m_outputNumbers;
    std::map<std::string, float> m_parameterValues;

    FeatureSet m_featureSet;

    /// csv files for debug purposes which get (over)written on every execution
    std::ofstream csvfile;

};

#endif /* doppler_speed_calculator_hpp */

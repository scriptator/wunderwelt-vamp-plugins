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
    
private:
    size_t m_blocksProcessed;
    size_t m_stepSize;
    size_t m_blockSize;
    mutable std::map<float, std::vector<float>*> m_frequencyTimeline;
    mutable std::map<std::string, int> m_outputNumbers;
    FeatureSet m_featureSet;
    
    float getFrequencyForBin(size_t bin) {
        return this->m_inputSampleRate * bin / this->m_blockSize;
    };
    
    std::ofstream csvfile;

};

#endif /* doppler_speed_calculator_hpp */

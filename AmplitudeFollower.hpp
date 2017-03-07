//
//  wunderwelt-vamp-plugin.hpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 21.01.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#ifndef amplitude_follower_hpp
#define amplitude_follower_hpp

#include <stdio.h>
#include <vamp-sdk/Plugin.h>

using std::string;

class AmplitudeFollower : public Vamp::Plugin {

public:
    AmplitudeFollower(float inputSampleRate);

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
    size_t m_channels;
    size_t m_stepSize;
    size_t m_blockSize;
    mutable std::map<std::string, int> m_outputNumbers;
    FeatureSet m_featureSet;
};

#endif /* amplitude_follower_hpp */

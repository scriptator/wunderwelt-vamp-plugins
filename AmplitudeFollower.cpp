//
//  wunderwelt-vamp-plugin.cpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 21.01.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#include "AmplitudeFollower.hpp"
#include <vamp-sdk/FFT.h>

#include <iostream>
#include <sstream>

using std::string;
using std::vector;
using std::stringstream;
using Vamp::RealTime;

#include <cmath>

AmplitudeFollower::AmplitudeFollower (float inputSampleRate) :
    Vamp::Plugin(inputSampleRate),
    m_blocksProcessed(0),
    m_channels(0),
    m_stepSize(0),
    m_blockSize(0),
    m_outputNumbers({}),
    m_featureSet(FeatureSet())
    {}

string AmplitudeFollower::getIdentifier() const {
    return "wunderwelt-amplitude-follower";
}

string AmplitudeFollower::getName() const {
    return "Wunderwelt Amplitude Follower";
}

string AmplitudeFollower::getDescription() const {
    return "Plugin for tracking the amplitude of a sound file";
}

string AmplitudeFollower::getMaker() const {
    return "Johannes Vass";
}

int AmplitudeFollower::getPluginVersion() const {
    return 1;
}

string AmplitudeFollower::getCopyright() const {
    return "BSD";
}

AmplitudeFollower::InputDomain AmplitudeFollower::getInputDomain() const {
    return TimeDomain;
}

size_t AmplitudeFollower::getPreferredBlockSize() const {
    return 0;
}

size_t AmplitudeFollower::getPreferredStepSize() const {
    return 0;
}

size_t AmplitudeFollower::getMinChannelCount() const {
    return 1;
}

size_t AmplitudeFollower::getMaxChannelCount() const {
    return 10;
}

AmplitudeFollower::ParameterList AmplitudeFollower::getParameterDescriptors() const
{
    ParameterList list;
    return list;
}

float AmplitudeFollower::getParameter(string identifier) const {
    return 0;
}

void AmplitudeFollower::setParameter(string identifier, float value) {
}

AmplitudeFollower::ProgramList AmplitudeFollower::getPrograms() const {
    ProgramList list;
    return list;
}

string AmplitudeFollower::getCurrentProgram() const {
    return ""; // no programs
}

void AmplitudeFollower::selectProgram(string) {
}


AmplitudeFollower::OutputList AmplitudeFollower::getOutputDescriptors() const
{
    OutputList list;
    int n = 0;

    OutputDescriptor d;

    d.identifier = "curve-fsr-amplitude";
    d.name = "Curve: Amplitude";
    d.description = "For each step a feature with the maximum value of the block is returned.";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::OneSamplePerStep;
    d.hasDuration = false;
    m_outputNumbers[d.identifier] = n++;
    list.push_back(d);

    return list;
}

bool AmplitudeFollower::initialise(size_t channels, size_t stepSize, size_t blockSize) {
    if (channels < getMinChannelCount() ||
        channels > getMaxChannelCount()) return false;

    m_channels = channels;
    m_stepSize = stepSize;
    m_blockSize = blockSize;

    return true;
}

void AmplitudeFollower::reset() {
    m_blocksProcessed = 0;
}


AmplitudeFollower::FeatureSet AmplitudeFollower::process(const float *const *inputBuffers, RealTime timestamp) {
    Feature f;
    f.hasTimestamp = true;
    f.timestamp = timestamp;
    f.hasDuration = false;

    float maxVal = 0;

    for (int c = 0; c < m_channels; ++c) {
        // first value plus number of non-zero values
        for (int i = 0; i < m_blockSize; ++i) {
            maxVal = fmax(maxVal, fabsf(inputBuffers[c][i]));
        }
    }

    // we have bin count --> only one value per feature
    f.values.push_back(maxVal);

    // put the feature into the feature set
    FeatureSet fs;
    fs[m_outputNumbers["curve-fsr-amplitude"]].push_back(f);
    return fs;
}

AmplitudeFollower::FeatureSet AmplitudeFollower::getRemainingFeatures() {
    return FeatureSet();
}

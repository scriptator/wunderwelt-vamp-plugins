//
//  wunderwelt-vamp-plugin.cpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 21.01.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#include "DopplerSpeedCalculator.hpp"
#include <vamp-sdk/FFT.h>

#include <iostream>
#include <sstream>

using std::string;
using std::vector;
using std::stringstream;
using Vamp::RealTime;

#include <cmath>

DopplerSpeedCalculator::DopplerSpeedCalculator (float inputSampleRate) :
    Vamp::Plugin(inputSampleRate),
    m_blocksProcessed(0),
    m_channels(0),
    m_stepSize(0),
    m_blockSize(0),
    m_outputNumbers({}),
    m_featureSet(FeatureSet())
    {}

string DopplerSpeedCalculator::getIdentifier() const {
    return "doppler-speed-calculator";
}

string DopplerSpeedCalculator::getName() const {
    return "Wunderwelt Doppler-Effect Speed Calculator";
}

string DopplerSpeedCalculator::getDescription() const {
    return "Plugin for deriving the speed of a moving source which emits a stable noise relative to a fixed measuring point.";
}

string DopplerSpeedCalculator::getMaker() const {
    return "Johannes Vass";
}

int DopplerSpeedCalculator::getPluginVersion() const {
    return 1;
}

string DopplerSpeedCalculator::getCopyright() const {
    return "BSD";
}

DopplerSpeedCalculator::InputDomain DopplerSpeedCalculator::getInputDomain() const {
    return InputDomain::FrequencyDomain;
}

size_t DopplerSpeedCalculator::getPreferredBlockSize() const {
    return 0;
}

size_t DopplerSpeedCalculator::getPreferredStepSize() const {
    return 0;
}

size_t DopplerSpeedCalculator::getMinChannelCount() const {
    return 1;
}

size_t DopplerSpeedCalculator::getMaxChannelCount() const {
    return 10;
}

DopplerSpeedCalculator::ParameterList DopplerSpeedCalculator::getParameterDescriptors() const
{
    ParameterList list;
    return list;
}

float DopplerSpeedCalculator::getParameter(string identifier) const {
    return 0;
}

void DopplerSpeedCalculator::setParameter(string identifier, float value) {
}

DopplerSpeedCalculator::ProgramList DopplerSpeedCalculator::getPrograms() const {
    ProgramList list;
    return list;
}

string DopplerSpeedCalculator::getCurrentProgram() const {
    return ""; // no programs
}

void DopplerSpeedCalculator::selectProgram(string) {
}


DopplerSpeedCalculator::OutputList DopplerSpeedCalculator::getOutputDescriptors() const
{
    OutputList list;
    int n = 0;
    
    OutputDescriptor d;
    
    d.identifier = "naive-speed-of-source";
    d.name = "Speed of noise-emitting source";
    d.description = "Returns the speed of the noise-emitting source in m/s by calculating it directly from the frequency difference "
        "between the beginning and end of the event. This means it is negligent of the normal distance between measuring point and "
        "route of the source. Returns exactly one value per detected event.";
    d.unit = "m/s";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = true;
    m_outputNumbers[d.identifier] = n++;
    list.push_back(d);
    
    return list;
}

bool DopplerSpeedCalculator::initialise(size_t channels, size_t stepSize, size_t blockSize) {
    if (channels < getMinChannelCount() ||
        channels > getMaxChannelCount()) return false;
    
    m_channels = channels;
    m_stepSize = stepSize;
    m_blockSize = blockSize;
    
    return true;
}

void DopplerSpeedCalculator::reset() {
    m_blocksProcessed = 0;
}


DopplerSpeedCalculator::FeatureSet DopplerSpeedCalculator::process(const float *const *inputBuffers, RealTime timestamp) {
    return FeatureSet();
}

DopplerSpeedCalculator::FeatureSet DopplerSpeedCalculator::getRemainingFeatures() {
    return FeatureSet();
}




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
#include <complex>      // std::complex
#include <assert.h>

using std::string;
using std::vector;
using std::complex;
using std::stringstream;
using Vamp::RealTime;

#include <cmath>

#define CHANNEL 0

// Parameter Identifiers
#define DEBUG_CSV_FILES "write-debug-csv"

#define UPPER_THRESHOLD_FREQUENCY 3000

DopplerSpeedCalculator::DopplerSpeedCalculator (float inputSampleRate) :
    Vamp::Plugin(inputSampleRate),
    carState(CarState::UNKNOWN),
    m_blocksProcessed(0),
    m_stepSize(0),
    m_blockSize(0),
    m_outputNumbers({})
{
    // initialize parameters with default values
    ParameterList parameters = this->getParameterDescriptors();
    for (auto it=parameters.begin(); it < parameters.end(); ++it) {
        ParameterDescriptor& desc = *it;
        this->m_parameterValues[desc.identifier] = desc.defaultValue;
    }
}

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
    return 8192;
}

size_t DopplerSpeedCalculator::getPreferredStepSize() const {
    return 1024;
}

size_t DopplerSpeedCalculator::getMinChannelCount() const {
    return 1;
}

size_t DopplerSpeedCalculator::getMaxChannelCount() const {
    return 1;
}

DopplerSpeedCalculator::ParameterList DopplerSpeedCalculator::getParameterDescriptors() const {
    ParameterList plist = ParameterList();
    
    ParameterDescriptor desc = ParameterDescriptor();
    desc.identifier = DEBUG_CSV_FILES;
    desc.name = "Debug CSV Files";
    desc.description = "Set to 1 if you want Debug CSV Files to be written to your home directory";
    desc.defaultValue = 0;
    desc.quantizeStep = 1.0f;
    desc.isQuantized = true;
    desc.minValue = 0;
    desc.maxValue = 1;
    desc.valueNames = std::vector<std::string>{"off", "on"};
    plist.push_back(std::move(desc));
    
    return plist;
}

float DopplerSpeedCalculator::getParameter(string identifier) const {
    return this->m_parameterValues.at(identifier);
}

void DopplerSpeedCalculator::setParameter(string identifier, float value) {
    this->m_parameterValues[identifier] = value;
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
    
    d.identifier = "dominating-frequencies";
    d.name = "Dominating Frequencies";
    d.description = "Returns the 5 most dominating frequencies per step";
    d.unit = "Hz";
    d.hasKnownExtents = true;
    d.minValue = 0;
    d.maxValue = this->m_inputSampleRate / 2; // Nyquist Frequency
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.hasDuration = true;
    m_outputNumbers[d.identifier] = n++;
    list.push_back(d);
    
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
    
    m_stepSize = stepSize;
    m_blockSize = blockSize;
    
    if (this->getParameter(DEBUG_CSV_FILES)) {
        // open the debug csv file for writing
        csvfile = std::ofstream("fft.csv");
        if (csvfile.fail()) {
            std::cerr << "WARNING: could not open debug csv file\n";
        }
    } else {
        csvfile = std::ofstream(0);
    }

    for (size_t i = 1; i <= m_blockSize / 2; ++i) {
        float freq = getFrequencyForBin(i);
        csvfile<< freq << " Hz;";
    }
    csvfile << "\n";

    return true;
}

void DopplerSpeedCalculator::reset() {
    m_blocksProcessed = 0;
}

DopplerSpeedCalculator::FeatureSet DopplerSpeedCalculator::process(const float *const *inputBuffers, RealTime timestamp) {
    float curMag = 0;
    const float *const inputBuffer = inputBuffers[CHANNEL];
    vector<float> currentData = vector<float>();

    // 0 Hz term, equivalent to the average of all the samples in the window
    // complex<float> dcTerm = complex<float>(inputBuffer[0], inputBuffer[1]);
    
    for (size_t i = 2; i < m_blockSize + 2; i+=2) {
        curMag = calcNormalizedMagnitude(std::complex<float>(inputBuffer[i], inputBuffer[i+1]));
        curMag = 20 * log10(curMag);
        csvfile << curMag << ";";
        currentData.push_back(curMag);
    }
    csvfile << "\n";
    
    // find all peaks with relatively small threshold amplitude first
    std::vector<PeakFinder::Peak<float>> peaks = PeakFinder::findPeaksThreshold(currentData.begin(), currentData.end(), 8.0f);
    
    // iterate and throw those lower than a threshold frequency into the "dominating-frequencies" feature
    Feature f;
    for (auto it=peaks.begin(); it < peaks.end(); ++it) {
        double position = it->interpolatedPosition;
        float freq = getFrequencyForBin(position);
        if (freq < UPPER_THRESHOLD_FREQUENCY && it->height >= 15.0f) {
            f.values.push_back(freq);
        }
    }
    FeatureSet fs;
    fs[m_outputNumbers["dominating-frequencies"]].push_back(f);
    
    this->stableBegin = timestamp;
    this->stableEnd = timestamp;
    m_blocksProcessed++;
    return fs;
}

DopplerSpeedCalculator::FeatureSet DopplerSpeedCalculator::getRemainingFeatures() {
    // put the feature into the feature set
    FeatureSet fs;
    Feature f;
    f.timestamp = this->stableBegin;
    f.duration = this->stableEnd - this->stableBegin;

    vector<float>& speedCalculations = f.values;
    for (auto it = this->peakHistories.begin(); it < this->peakHistories.end(); ++it) {
        float approachingFreq = getFrequencyForBin(it->getFirst().interpolatedPosition);
        float leavingFreq = getFrequencyForBin(it->getLast().interpolatedPosition);
        float speed = dopplerSpeedMovingSource(approachingFreq, leavingFreq);
        speedCalculations.push_back(speed);
    }
    std::sort(speedCalculations.begin(), speedCalculations.end());

    fs[m_outputNumbers["naive-speed-of-source"]].push_back(f);

    return fs;
}




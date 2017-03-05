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
#include <math.h>

using std::string;
using std::vector;
using std::complex;
using std::stringstream;
using Vamp::RealTime;

#include <cmath>

// the channel we're gona use (plugin works only mono)
#define CHANNEL 0

// Parameter Identifiers
#define DEBUG_CSV_FILES "write-debug-csv"

// constructor of the calculator
DopplerSpeedCalculator::DopplerSpeedCalculator (float inputSampleRate) :
    Vamp::Plugin(inputSampleRate),
    peakMatrix(vector<vector<Peak<float>*>>()),
    m_blocksProcessed(0),
    m_stepSize(0),
    m_blockSize(0),
    fftData(vector<vector<float>>(
)),
    m_outputNumbers({})
{
    ParameterList parameters = this->getParameterDescriptors();
    for (auto it=parameters.begin(); it < parameters.end(); ++it) {
        ParameterDescriptor& desc = *it;
        this->m_parameterValues[desc.identifier] = desc.defaultValue;
    }
}

DopplerSpeedCalculator::~DopplerSpeedCalculator () {
    for (auto vec : peakMatrix) {
        for (auto peak : vec) {
            delete peak;
        }
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
    return 2048;
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
    desc.defaultValue = 1;
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
    d.sampleType = OutputDescriptor::VariableSampleRate;
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
    const float *const inputBuffer = inputBuffers[CHANNEL];
    vector<float> currentData = vector<float>();
    bool peakDectectionTime = timestamp < RealTime().fromMilliseconds(PEAK_DETECTION_TIME);

    // 0 Hz term, equivalent to the average of all the samples in the window
    // complex<float> dcTerm = complex<float>(inputBuffer[0], inputBuffer[1]);
    
    for (size_t i = 2; i < m_blockSize + 2; i+=2) {
        float curMag = std::abs(std::complex<float>(inputBuffer[i], inputBuffer[i+1]));
        currentData.push_back(curMag);
    }

    // TODO improve this,
    fftData.push_back(currentData);

    if (fftData.size() == MOVING_FFT_AVERAGE_WIDTH) {
        vector<float> summedData = vector<float>(m_blockSize / 2);
        for (auto fft : fftData) {
            for (size_t i = 0; i < fft.size(); i++) {
                summedData[i] += fft[i];
            }
        }
        
        vector<float> averagedData;
        for (auto val : summedData) {
            float avgMag = val / MOVING_FFT_AVERAGE_WIDTH;
            float avgNormMag = normalizeMagnitude(avgMag);
            csvfile << avgNormMag << ";";
            averagedData.push_back(avgNormMag);
        }
        csvfile << "\n";
        
        // find all peaks with relatively small threshold amplitude first
        auto beginIt = averagedData.begin();
        auto endit = beginIt + getBinForFrequency(UPPER_THRESHOLD_FREQUENCY);
        float heightThreshold = peakDectectionTime ? PEAK_DETECTION_HEIGHT_THRESHOLD : PEAK_TRACING_HEIGHT_THRESHOLD;
        std::vector<Peak<float>*> peaks = PeakFinder::findPeaksThreshold(beginIt, endit, heightThreshold, timestamp);
        this->peakMatrix.push_back(peaks);
        
        
        // trace the peaks
        this->tracePeaks(peaks, peakDectectionTime);
        
        // remove the oldest fft result from the fftData vector
        fftData.erase(fftData.begin());
    }

    FeatureSet fs;
    m_blocksProcessed++;
    return fs;
}

void DopplerSpeedCalculator::tracePeaks(const std::vector<Peak<float> *> &peaks, bool allowNew) {
    auto currentHist = peakHistories.begin();
    auto lastHistory = currentHist;

    double currentHistoryPosition = 0;
    double lastHistoryPosition = currentHist != peakHistories.end() ? currentHist->getLast()->interpolatedPosition : std::numeric_limits<double>::min();;;

    double currentDiff;
    double lastDiff;

    bool peakDone = false;
    bool addedPeakToLast = false;
    bool addedPeakToCurrent = false;

    std::vector<PeakHistory<float>> toInsert;
    
    // iterate through the peaks and PeakHistories at the same time
    // invariant: both vectors are sorted by the position ascendingly
    for (auto peak : peaks) {
        peakDone = false;
        while (currentHist != peakHistories.end() && !peakDone) {
            currentHistoryPosition = currentHist->getLast()->interpolatedPosition;
            lastDiff = fabs(peak->interpolatedPosition - lastHistoryPosition);
            currentDiff = fabs(peak->interpolatedPosition - currentHistoryPosition);
            
            // if the peak is still between the two PeakHistories, try to associate it with one
            if (peak->interpolatedPosition < currentHistoryPosition) {
                if (lastDiff <= MAX_BIN_JUMP || currentDiff <= MAX_BIN_JUMP) {  // the peak is near enough to one of the already existing peaks
                    if (lastDiff < currentDiff) {
                        if (peak->interpolatedPosition > lastHistoryPosition + 1) {
                            std::cerr << "Warning: " << peak->timestamp.sec*1000 + peak->timestamp.msec() << ": " << peak->interpolatedPosition << " vs. " << lastHistoryPosition << "\n";
                        } else {
                            lastHistory->addPeak(peak);
                            addedPeakToLast = true;
                        }
                    } else {
                        currentHist->addPeak(peak);
                        addedPeakToCurrent = true;
                    }
                } else if (allowNew) {          // the peak is not near enough, so insert it if allowNew is set
                    toInsert.emplace_back(peak, BROADEST_ALLOWED_INTERRUPTION);
                } // else ignore peak
                peakDone = true;
            } else { // go one step further in the vector of PeakHistories
                if (!addedPeakToLast) {
                    lastHistory->noPeak();
                }
                addedPeakToLast = addedPeakToCurrent;
                addedPeakToCurrent = false;
                
                lastHistoryPosition = currentHistoryPosition;
                lastHistory = currentHist;
                ++currentHist;
            }
        }
        
        if (!peakDone && allowNew) {
            toInsert.emplace_back(peak, BROADEST_ALLOWED_INTERRUPTION);
        }
    }
    
    // kill remove peak histories which are not alive
    peakHistories.erase(std::remove_if(peakHistories.begin(), peakHistories.end(),
                                       [](PeakHistory<float> & elem) -> bool { return !elem.isAlive(); }),
                        peakHistories.end());
    
    // insert new peaks into histories and keep them sorted
    peakHistories.insert(peakHistories.end(), toInsert.begin(), toInsert.end());
    std::sort(peakHistories.begin(), peakHistories.end(),
              [](const PeakHistory<float> & a, const PeakHistory<float> & b) -> bool {
        return a.getLast()->interpolatedPosition < b.getLast()->interpolatedPosition;
    });
}

DopplerSpeedCalculator::FeatureSet DopplerSpeedCalculator::getRemainingFeatures() {
    // put the feature into the feature set
    FeatureSet fs;

    // sort peaks by total height
    std::sort(peakHistories.begin(), peakHistories.end(),
              [](const PeakHistory<float> & a, const PeakHistory<float> & b) -> bool {
                  return a.getTotalPeakHeight() > b.getTotalPeakHeight();
              });

    // output the dominating frequencies feature
    Feature dominatingFrequencies;
    dominatingFrequencies.hasTimestamp = true;
    dominatingFrequencies.hasDuration = true;
    auto firstHist = this->peakHistories.begin();
    vector<pair<RealTime, double>> positions;
    firstHist->getInterpolatedPositionHistory(positions);
    for (auto pos : positions) {
        dominatingFrequencies.duration = RealTime().fromSeconds(m_blockSize / m_inputSampleRate * (1.0 * m_stepSize / m_blockSize));
        dominatingFrequencies.timestamp = pos.first;
        dominatingFrequencies.values = vector<float>(1, getFrequencyForBin(pos.second));
        fs[m_outputNumbers["dominating-frequencies"]].push_back(dominatingFrequencies);
    }
    
    Feature speed;
    while (firstHist != peakHistories.end()) {
        auto approaching = firstHist->getStableBegin();
        auto leaving = firstHist->getStableEnd();
        if (approaching && leaving) {
            speed.timestamp = approaching->timestamp;
            speed.duration = approaching->timestamp - speed.timestamp;
            speed.values.push_back(dopplerSpeedMovingSource(approaching->interpolatedPosition, leaving->interpolatedPosition));
        }

        ++firstHist;
    }

    fs[m_outputNumbers["naive-speed-of-source"]].push_back(speed);

    return fs;
}




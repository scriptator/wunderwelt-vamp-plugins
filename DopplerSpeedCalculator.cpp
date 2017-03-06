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
#include <cmath>

using std::string;
using std::vector;
using std::complex;
using std::stringstream;
using Vamp::RealTime;

// the channel we're gona use (plugin works only mono)
#define CHANNEL 0

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
    desc.description = "Set to 1 if you want Debug CSV Files to be written to the current working directory";
    desc.defaultValue = 0;
    desc.quantizeStep = 1.0f;
    desc.isQuantized = true;
    desc.minValue = 0;
    desc.maxValue = 1;
    desc.valueNames = std::vector<std::string>{"off", "on"};
    plist.push_back(desc);

    desc = ParameterDescriptor();
    desc.identifier = PEAK_DETECTION_TIME_ID;
    desc.name = "Peak Detection Time";
    desc.description = "Number of seconds from start during which new peaks are accepted";
    desc.defaultValue = PEAK_DETECTION_TIME;
    desc.minValue = 0;
    desc.maxValue = 10;
    desc.unit = "s";
    plist.push_back(desc);

    desc = ParameterDescriptor();
    desc.identifier = PEAK_DETECTION_HEIGHT_THRESHOLD_ID;
    desc.name = "Peak Detection Height Threshold";
    desc.description = "The height (in dB) a peak must have during peak detection time to be accepted";
    desc.defaultValue = PEAK_DETECTION_HEIGHT_THRESHOLD;
    desc.minValue = 0;
    desc.maxValue = 50;
    desc.unit = "dB";
    plist.push_back(desc);

    desc = ParameterDescriptor();
    desc.identifier = PEAK_TRACING_HEIGHT_THRESHOLD_ID;
    desc.name = "Peak Tracing Height Threshold";
    desc.description = "The height (in dB) a peak must have during peak tracing time to be accepted (i.e. after no new peaks are allowed any more)";
    desc.defaultValue = PEAK_TRACING_HEIGHT_THRESHOLD;
    desc.minValue = 0;
    desc.maxValue = 50;
    desc.unit = "dB";
    plist.push_back(desc);

    desc = ParameterDescriptor();
    desc.identifier = UPPER_THRESHOLD_FREQUENCY_ID;
    desc.name = "Upper Threshold Frequency";
    desc.description = "Only peaks below this threshold frequency will be allowed)";
    desc.defaultValue = UPPER_THRESHOLD_FREQUENCY;
    desc.minValue = 0;
    desc.maxValue = 20000;
    desc.unit = "Hz";
    plist.push_back(desc);

    desc = ParameterDescriptor();
    desc.identifier = MAX_BIN_JUMP_ID;
    desc.name = "Maximum Bin Jump";
    desc.description = "The largest allowed offset between the positions of two peak candidates for them to be considered the same.";
    desc.defaultValue = MAX_BIN_JUMP;
    desc.minValue = 1;
    desc.maxValue = 20;
    desc.isQuantized = true;
    desc.quantizeStep = 1.0;
    desc.unit = "bins";
    plist.push_back(desc);

    desc = ParameterDescriptor();
    desc.identifier = BROADEST_ALLOWED_INTERRUPTION_ID;
    desc.name = "Broadest allowed interruption";
    desc.description = "The largest allowed temporal gap between two detected peaks in bins";
    desc.defaultValue = BROADEST_ALLOWED_INTERRUPTION;
    desc.minValue = 0;
    desc.maxValue = 20;
    desc.isQuantized = true;
    desc.quantizeStep = 1.0;
    desc.unit = "bins";
    plist.push_back(desc);

    desc = ParameterDescriptor();
    desc.identifier = MOVING_FFT_AVERAGE_WIDTH_ID;
    desc.name = "Width of moving average";
    desc.description = "The number of steps whose data is averaged to detect the peaks afterwards";
    desc.defaultValue = MOVING_FFT_AVERAGE_WIDTH;
    desc.minValue = 1;
    desc.maxValue = 10;
    desc.isQuantized = true;
    desc.quantizeStep = 1.0;
    desc.unit = "steps";
    plist.push_back(desc);

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
    d.description = "Returns the speed of the noise-emitting source in km/h by calculating it directly from the frequency difference "
    "between the beginning and end of the event. This means it is negligent of the normal distance between measuring point and "
    "route of the source. Returns one feature with the duration of the detected feature, as given by 'dominating-frequency'";
    d.unit = "km/h";
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
    bool peakDectectionTime = timestamp < RealTime().fromSeconds(getParameter(PEAK_DETECTION_TIME_ID));
    int movingFFTAverageWidth = getParameter(MOVING_FFT_AVERAGE_WIDTH_ID);
    
    // 0 Hz term, equivalent to the average of all the samples in the window
    // complex<float> dcTerm = complex<float>(inputBuffer[0], inputBuffer[1]);
    
    // calculate the magnitudes and store them in fftData
    for (size_t i = 2; i < m_blockSize + 2; i+=2) {
        float curMag = std::abs(std::complex<float>(inputBuffer[i], inputBuffer[i+1]));
        currentData.push_back(curMag);
    }
    fftData.emplace_back(std::move(currentData));

    if (fftData.size() == movingFFTAverageWidth) {
        // sum up fftData to later calculate the average
        vector<float> summedData = vector<float>(m_blockSize / 2);
        for (auto fft : fftData) {
            for (size_t i = 0; i < fft.size(); i++) {
                summedData[i] += fft[i];
            }
        }
        
        // calc the average, normalize the magnitudes and store them again for peak finding
        vector<float> averagedData;
        for (auto val : summedData) {
            float avgMag = val / movingFFTAverageWidth;
            float avgNormMag = normalizeMagnitude(avgMag);
            csvfile << avgNormMag << ";";
            averagedData.push_back(avgNormMag);
        }
        csvfile << "\n";
        
        // find all peaks, where the threshold is dependent on whether we are before or after PEAK_DETECTION_TIME
        auto beginIt = averagedData.begin();
        auto endit = beginIt + getBinForFrequency(getParameter(UPPER_THRESHOLD_FREQUENCY_ID));
        float heightThreshold = peakDectectionTime ? getParameter(PEAK_DETECTION_HEIGHT_THRESHOLD_ID) : getParameter(PEAK_TRACING_HEIGHT_THRESHOLD_ID);
        std::vector<Peak<float>*> peaks = PeakFinder::findPeaksThreshold(beginIt, endit, heightThreshold, timestamp);
        this->peakMatrix.push_back(peaks);
        
        // trace the peaks
        this->tracePeaks(peaks, peakDectectionTime);
        
        // remove the oldest fft result from the fftData vector to achieve a moving average
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
    
    // parameter values
    auto maxBinJump = getParameter(MAX_BIN_JUMP_ID);
    size_t broadestAllowedInterruption = (size_t) getParameter(BROADEST_ALLOWED_INTERRUPTION_ID);
    
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
                if (lastDiff <= maxBinJump || currentDiff <= maxBinJump) {  // the peak is near enough to one of the already existing peaks
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
                    toInsert.emplace_back(peak, broadestAllowedInterruption);
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
            toInsert.emplace_back(peak, broadestAllowedInterruption);
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
    
    if (peakHistories.empty()) {
        return fs;
    }

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
            speed.hasDuration = true;
            speed.hasTimestamp = true;
            speed.timestamp = approaching->timestamp;
            speed.duration = leaving->timestamp - approaching->timestamp;
            speed.values.push_back(dopplerSpeedMovingSource(approaching->interpolatedPosition, leaving->interpolatedPosition));
            fs[m_outputNumbers["naive-speed-of-source"]].push_back(speed);
            break;
        }
        ++firstHist;
    }

    return fs;
}




//
//  wunderwelt-vamp-plugin.cpp
//  wunderwelt-vamp-plugin
//
//  Created by Johannes Vass on 21.01.17.
//  Copyright © 2017 Johannes Vass. All rights reserved.
//

#include "AmplitudeFollower.hpp"

using std::string;
using std::vector;

#include <cmath>
#include <string>
#include <vector>

AmplitudeFollower::AmplitudeFollower (float inputSampleRate) :
    Vamp::Plugin(inputSampleRate),
    stepSize(0),
    blockSize(0) {}

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
    return "Johannes Vass"";
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
    d.name = "Curve: FixedSampleRate/Timed";
    d.description = "A time series with a fixed sample rate (independent of process step size) but with timestamps on features";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    d.sampleRate = 2.5;
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
    m_n = 0;
}

static Vamp::Plugin::Feature
instant(RealTime r, int i, int n)
{
    stringstream s;
    Vamp::Plugin::Feature f;
    f.hasTimestamp = true;
    f.timestamp = r;
    f.hasDuration = false;
    s << i+1 << " of " << n << " at " << r.toText();
    f.label = s.str();
    return f;
}

static Vamp::Plugin::Feature
untimedCurveValue(RealTime r, int i, int n)
{
    stringstream s;
    Vamp::Plugin::Feature f;
    f.hasTimestamp = false;
    f.hasDuration = false;
    float v = float(i) / float(n);
    f.values.push_back(v);
    s << i+1 << " of " << n << ": " << v << " at " << r.toText();
    f.label = s.str();
    return f;
}

static Vamp::Plugin::Feature
timedCurveValue(RealTime r, int i, int n)
{
    stringstream s;
    Vamp::Plugin::Feature f;
    f.hasTimestamp = true;
    f.timestamp = r;
    f.hasDuration = false;
    float v = float(i) / float(n);
    f.values.push_back(v);
    s << i+1 << " of " << n << ": " << v << " at " << r.toText();
    f.label = s.str();
    return f;
}

static Vamp::Plugin::Feature
snappedCurveValue(RealTime r, RealTime sn, int i, int n)
{
    stringstream s;
    Vamp::Plugin::Feature f;
    f.hasTimestamp = true;
    f.timestamp = r;
    f.hasDuration = false;
    float v = float(i) / float(n);
    f.values.push_back(v);
    s << i+1 << " of " << n << ": " << v << " at " << r.toText() << " snap to " << sn.toText();
    f.label = s.str();
    return f;
}

static Vamp::Plugin::Feature
gridColumn(RealTime r, int i, int n)
{
    stringstream s;
    Vamp::Plugin::Feature f;
    f.hasTimestamp = false;
    f.hasDuration = false;
    for (int j = 0; j < 10; ++j) {
        float v = float(j + i + 2) / float(n + 10);
        f.values.push_back(v);
    }
    s << i+1 << " of " << n << " at " << r.toText();
    f.label = s.str();
    return f;
}

static Vamp::Plugin::Feature
noteOrRegion(RealTime r, RealTime d, int i, int n)
{
    stringstream s;
    Vamp::Plugin::Feature f;
    f.hasTimestamp = true;
    f.timestamp = r;
    f.hasDuration = true;
    f.duration = d;
    float v = float(i) / float(n);
    f.values.push_back(v);
    s << i+1 << " of " << n << ": " << v << " at " << r.toText() << " dur. " << d.toText();
    f.label = s.str();
    return f;
}

static
float snap(float x, float r)
{
    int n = int(x / r + 0.5);
    return n * r;
}

Vamp::Plugin::FeatureSet
AmplitudeFollower::featuresFrom(RealTime timestamp, bool final)
{
    FeatureSet fs;
    
    RealTime endTime = timestamp + RealTime::frame2RealTime
    (m_stepSize, m_inputSampleRate);
    
    for (int i = 0; i < (int)m_instants.size(); ++i) {
        
        if (m_instants[i] >= timestamp && (final || m_instants[i] < endTime)) {
            fs[m_outputNumbers["instants"]]
            .push_back(instant(m_instants[i], i, m_instants.size()));
        }
        
        RealTime variCurveTime = m_instants[i] / 2;
        if (variCurveTime >= timestamp && (final || variCurveTime < endTime)) {
            fs[m_outputNumbers["curve-vsr"]]
            .push_back(timedCurveValue(variCurveTime, i, m_instants.size()));
        }
        
        RealTime noteTime = (m_instants[i] + m_instants[i]) / 3;
        RealTime noteDuration = RealTime::fromSeconds((i % 2 == 0) ? 1.75 : 0.5);
        
        if (noteTime >= timestamp && (final || noteTime < endTime)) {
            fs[m_outputNumbers["notes-regions"]]
            .push_back(noteOrRegion(noteTime, noteDuration, i, m_instants.size()));
        }
    }
    
    if (!final) {
        
        if (m_n < 20) {
            fs[m_outputNumbers["curve-oss"]]
            .push_back(untimedCurveValue(timestamp, m_n, 20));
        }
        
        if (m_n < 5) {
            fs[m_outputNumbers["curve-fsr"]]
            .push_back(untimedCurveValue(RealTime::fromSeconds(m_n / 2.5), m_n, 10));
            
            float s = (m_n / 4) * 2;
            if ((m_n % 4) > 0) {
                s += float((m_n % 4) - 1) / 6.0;
            }
            fs[m_outputNumbers["curve-fsr-timed"]]
            .push_back(snappedCurveValue(RealTime::fromSeconds(s),
                                         RealTime::fromSeconds(snap(s, 0.4)),
                                         m_n, 10));
        }
        
        if (m_n < 20) {
            fs[m_outputNumbers["grid-oss"]]
            .push_back(gridColumn(timestamp, m_n, 20));
        }
        
    } else {
        
        for (int i = (m_n > 5 ? 5 : m_n); i < 10; ++i) {
            fs[m_outputNumbers["curve-fsr"]]
            .push_back(untimedCurveValue(RealTime::fromSeconds(i / 2.5), i, 10));
            
            float s = (i / 4) * 2;
            if ((i % 4) > 0) {
                s += float((i % 4) - 1) / 6.0;
            }
            fs[m_outputNumbers["curve-fsr-timed"]]
            .push_back(snappedCurveValue(RealTime::fromSeconds(s),
                                         RealTime::fromSeconds(snap(s, 0.4)),
                                         i, 10));
        }
        
        for (int i = 0; i < 10; ++i) {
            fs[m_outputNumbers["grid-fsr"]]
            .push_back(gridColumn(RealTime::fromSeconds(i / 2.5), i, 10));
        }
    }
    
    m_lastTime = endTime;
    m_n = m_n + 1;
    return fs;
}

AmplitudeFollower::FeatureSet
AmplitudeFollower::process(const float *const *inputBuffers, RealTime timestamp)
{
    if (!m_produceOutput) return FeatureSet();
    FeatureSet fs = featuresFrom(timestamp, false);
    
    Feature f;
    float eps = 1e-6f;
    
    for (int c = 0; c < m_channels; ++c) {
        if (!m_frequencyDomain) {
            // first value plus number of non-zero values
            float sum = inputBuffers[c][0];
            for (int i = 0; i < m_blockSize; ++i) {
                if (fabsf(inputBuffers[c][i]) >= eps) sum += 1;
            }
            f.values.push_back(sum);
        } else {
            // If we're in frequency-domain mode, we convert back to
            // time-domain to calculate the input-summary feature
            // output. That should help the caller check that
            // time-frequency conversion has gone more or less OK,
            // though they'll still have to bear in mind windowing and
            // FFT shift (i.e. phase shift which puts the first
            // element in the middle of the frame)
            vector<double> ri(m_blockSize, 0.0);
            vector<double> ii(m_blockSize, 0.0);
            vector<double> ro(m_blockSize, 0.0);
            vector<double> io(m_blockSize, 0.0);
            for (int i = 0; i <= m_blockSize/2; ++i) {
                ri[i] = inputBuffers[c][i*2];
                ii[i] = inputBuffers[c][i*2 + 1];
                if (i > 0) ri[m_blockSize-i] =  ri[i];
                if (i > 0) ii[m_blockSize-i] = -ii[i];
            }
            Vamp::FFT::inverse(m_blockSize, &ri[0], &ii[0], &ro[0], &io[0]);
            float sum = 0;
            for (int i = 0; i < m_blockSize; ++i) {
                if (fabs(ro[i]) >= eps) sum += 1;
            }
            sum += ro[0];
            f.values.push_back(sum);
        }
    }
    
    fs[m_outputNumbers["input-summary"]].push_back(f);
    
    f.values.clear();
    float frame = RealTime::realTime2Frame(timestamp, m_inputSampleRate);
    f.values.push_back(frame);
    fs[m_outputNumbers["input-timestamp"]].push_back(f);
    
    return fs;
}

AmplitudeFollower::FeatureSet
AmplitudeFollower::getRemainingFeatures()
{
    if (!m_produceOutput) return FeatureSet();
    FeatureSet fs = featuresFrom(m_lastTime, true);
    return fs;
}




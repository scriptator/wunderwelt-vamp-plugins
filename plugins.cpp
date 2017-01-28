

#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

#include "AmplitudeFollower.hpp"
#include "vamp-test-plugin.hpp"

class TestAdapter : public Vamp::PluginAdapterBase
{
public:
    TestAdapter(bool freq):
        PluginAdapterBase(),
        m_freq(freq) { }
    
    virtual ~TestAdapter() { }
    
protected:
    bool m_freq;
    
    Vamp::Plugin *createPlugin(float inputSampleRate) {
        return new VampTestPlugin(inputSampleRate, m_freq);
    }
};

class AmplitudeAdapter : public Vamp::PluginAdapterBase
{
public:
    AmplitudeAdapter():
    PluginAdapterBase() { }
    
    virtual ~AmplitudeAdapter() { }
    
protected:
    Vamp::Plugin *createPlugin(float inputSampleRate) {
        return new AmplitudeFollower(inputSampleRate);
    }
};

static AmplitudeAdapter amplitudeFollower;
static TestAdapter timeAdapter(false);
static TestAdapter freqAdapter(true);

const VampPluginDescriptor *
vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
    if (version < 1) return 0;
    
    // Return a different plugin adaptor's descriptor for each index,
    // and return 0 for the first index after you run out of plugins.
    // (That's how the host finds out how many plugins are in this
    // library.)
    
    switch (index) {
        case  0: return amplitudeFollower.getDescriptor();
        case  1: return timeAdapter.getDescriptor();
        case  2: return freqAdapter.getDescriptor();
        default: return 0;
    }
}



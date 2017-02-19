

#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

#include "AmplitudeFollower.hpp"
#include "DopplerSpeedCalculator.hpp"

class DopplerAdapter : public Vamp::PluginAdapterBase
{
public:
    DopplerAdapter():
        PluginAdapterBase() { }
    
    virtual ~DopplerAdapter() { }
    
protected:
    Vamp::Plugin *createPlugin(float inputSampleRate) {
        return new DopplerSpeedCalculator(inputSampleRate);
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
static DopplerAdapter speedCalculator;

const VampPluginDescriptor *
vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
    if (version < 1) return 0;
    
    // Return a different plugin adaptor's descriptor for each index,
    // and return 0 for the first index after you run out of plugins.
    // (That's how the host finds out how many plugins are in this
    // library.)
    
    switch (index) {
        case  0: return speedCalculator.getDescriptor();
        case  1: return amplitudeFollower.getDescriptor();
        default: return 0;
    }
}



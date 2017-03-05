
PLUGIN_LIBRARY_NAME := wunderwelt-vamp-plugin

PLUGIN_SOURCES 	    := AmplitudeFollower.cpp DopplerSpeedCalculator.cpp PeakFinder.cpp PeakHistory.cpp vamp-test-plugin.cpp

PLUGIN_HEADERS 	    := AmplitudeFollower.hpp DopplerSpeedCalculator.hpp PeakFinder.hpp PeakHistory.hpp vamp-test-plugin.hpp

SRC_DIR		:= .

CFLAGS		:= $(ARCHFLAGS) $(CFLAGS)
CXXFLAGS	:= $(CFLAGS) -I. -I$(VAMPSDK_DIR) $(CXXFLAGS)

LDFLAGS		:= $(ARCHFLAGS) $(LDFLAGS)
PLUGIN_LDFLAGS	:= $(LDFLAGS) $(PLUGIN_LDFLAGS)

VAMPSDK_DIR	?= ../vamp-plugin-sdk
PLUGIN_EXT	?= .so
CXX 		?= g++
CC 		?= gcc

PLUGIN 		:= $(PLUGIN_LIBRARY_NAME)$(PLUGIN_EXT)

PLUGIN_OBJECTS 	:= $(PLUGIN_SOURCES:.cpp=.o)
PLUGIN_OBJECTS 	:= $(PLUGIN_OBJECTS:.c=.o)

all: 		$(PLUGIN)

$(PLUGIN): 	$(PLUGIN_OBJECTS) 
		$(CXX) -o $@ $^ $(PLUGIN_LDFLAGS)

$(PLUGIN_OBJECTS): $(PLUGIN_HEADERS)

clean:
	rm -f $(PLUGIN_OBJECTS)

distclean:	clean
	rm -f $(PLUGIN)

depend:
	makedepend -Y -fMakefile.inc $(PLUGIN_SOURCES) $(PLUGIN_HEADERS)

# DO NOT DELETE

AmplitudeFollower.o: AmplitudeFollower.hpp
DopplerSpeedCalculator.o: DopplerSpeedCalculator.hpp
PeakFinder.o: PeakFinder.hpp
PeakHistory.o: PeakHistory.hpp
VampTestPlugin.o: vamp-test-plugin.hpp

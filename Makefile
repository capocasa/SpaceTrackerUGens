.PHONY : clean

SOURCES = SpaceTrackerUGens.cpp
OBJECTS=$(SOURCES:.cpp=.o)
SCDIR=../supercollider/include
TARGET=SpaceTrackerUGens.so

CPPFLAGS= -fPIC -g -I$(SCDIR)/common -I$(SCDIR)/plugin_interface
LDFLAGS= -shared

all:	$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CPPFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)


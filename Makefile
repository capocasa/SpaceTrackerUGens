.PHONY : clean

NAME=SpaceTracker
SCDIR=../supercollider/include

SOURCES = $(NAME)UGens.cpp
OBJECTS=$(SOURCES:.cpp=.o)


TARGET_BASENAME=$(NAME)UGens$(if $(SUPERNOVA),_supernova)

ifeq ($(PLATFORM), Windows)
EXTENSION=dll
CC := x86_64-w64-mingw32-g++
CXX := x86_64-w64-mingw32-g++
else ifeq ($(PLATFORM), OSX)
EXTENSION=so
CC := o64h-clang++-libc++
CXX := o64h-clang++-libc++
else
EXTENSION=so
endif

TARGET=$(TARGET_BASENAME).$(EXTENSION)

CPPFLAGS= -fPIC -g -I$(SCDIR) -I$(SCDIR)/common -I$(SCDIR)/plugin_interface $(if $(SUPERNOVA),-DSUPERNOVA=on)
LDFLAGS= -shared

all:	$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CPPFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

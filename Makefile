# Makefile for OS-X 10.9 + GCC 4.9

MAYA_LOCATION = /Applications/Autodesk/maya

PROJ1 = probeDeformer
PROJ2 = probeDeformerARAP
SOURCE1 = ./$(PROJ1)/$(PROJ1).cpp
SOURCE2 = ./$(PROJ2)/$(PROJ2).cpp
OBJECT1 = ./$(PROJ1).o
OBJECT2 = ./$(PROJ2).o

INCLUDES = -I$(MAYA_LOCATION)/devkit/include/ -I./ -I../ -I/usr/local/include/eigen3/
LIBS = -L$(MAYA_LOCATION)/Maya.app/Contents/MacOS -lOpenMaya -lOpenMayaAnim -lOpenMayaRender -lOpenMayaUI -lFoundation

#compiler options
CC = /usr/local/bin/g++
CFLAGS = -DCC_GNU_ -DOSMac_ -DOSMacOSX_  \
	-DOSMac_MachO_ -D_LANGUAGE_C_PLUS_PLUS \
	-include "$(MAYA_LOCATION)/devkit/include/maya/OpenMayaMac.h" \
	-DMAC_PLUGIN -D_BOOL -DREQUIRE_IOSTREAM -fopenmp -O3
C++FLAGS = $(CFLAGS) $(WARNFLAGS) $(ERROR_FLAGS) -fno-gnu-keywords
LDFLAGS = $(C++FLAGS)
DYNLIB_LOCATION	= $(MAYA_LOCATION)/Maya.app/Contents/MacOS

LDFLAGS	+= -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk \
				$(ARCH_FLAGS) -headerpad_max_install_names -bundle

LDFLAGS += -Wl,-exported_symbol,__Z16initializePlugin7MObject \
          -Wl,-exported_symbol,__Z18uninitializePlugin7MObject

LREMAP = -Wl,-executable_path,"$(DYNLIB_LOCATION)"
LDFLAGS += -L"$(DYNLIB_LOCATION)" $(LREMAP)

.PHONY: all install clean

all: $(PROJ1) $(PROJ2)

$(PROJ1): $(OBJECT1)
	        $(CC) $(LDFLAGS) $(LIBS) $^ -o $@.bundle

$(PROJ2): $(OBJECT2)
	        $(CC) $(LDFLAGS) $(LIBS) $^ -o $@.bundle

$(OBJECT1): $(SOURCE1)
	        $(CC) -c $(INCLUDES) $(C++FLAGS) $^

$(OBJECT2): $(SOURCE2)
	        $(CC) -c $(INCLUDES) $(C++FLAGS) $^

install:
	        mv $(PROJ1).bundle $(PROJ2).bundle /Users/Shared/Autodesk/maya/plug-ins/

clean:
		rm -f $(OBJECT1) $(OBJECT2)

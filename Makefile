IDIR = include/unitig_distance
BUILDDIR = bin
OBJDIR = build
SRCDIR = src
EXECNAME = ud

CXX = g++
CXXFLAGS = -std=c++11 -pthread -O2 -pedantic -I$(IDIR) -DDEBUG

SOURCES = $(shell find $(SRCDIR) -type f -name *.cpp)
OBJECTS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.cpp=.o))
DEPENDS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.cpp=.d))

.PHONY: all clean

all: $(EXECNAME)

clean:
	\rm $(OBJDIR)/*.o $(OBJDIR)/*.d $(EXECNAME)

-include $(DEPENDS)

$(EXECNAME): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(BUILDDIR)/$(EXECNAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

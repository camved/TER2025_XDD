CXXFLAGS += $(shell otawa-config --cflags)
LIBS += $(shell otawa-config --libs --rpath)

all: first

first: first.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
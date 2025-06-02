CXXFLAGS += $(shell otawa-config --cflags)
LIBS += $(shell otawa-config --libs --rpath)

all: display

display: display.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
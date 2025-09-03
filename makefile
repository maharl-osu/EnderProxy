CXX = g++
CXXFLAGS = -Wall -Isrc
LDFLAGS =

SRCDIRS = src src/config src/networkmanager src/packet
SRCS = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))

OBJDIR = build/obj
OBJS = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))

TARGET = EnderProxy

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
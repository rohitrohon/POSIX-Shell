
CXX := g++
CXXFLAGS := -std=c++17 -Iinclude
LDFLAGS  = -L$(shell brew --prefix readline)/lib -lreadline
SRCS := $(wildcard src/*.cpp)
OBJDIR = build
OBJS = $(SRCS:src/%.cpp=$(OBJDIR)/%.o)
TARGET = mysh


all: $(OBJDIR) $(TARGET)


$(OBJDIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)


$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./mysh
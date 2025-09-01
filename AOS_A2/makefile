CXX = g++
CXXFLAGS = -std=c++17 -Iinclude

SRCS = src/main.cpp src/prompt.cpp src/utils.cpp src/parser.cpp \
       src/builtins.cpp src/exec.cpp src/pinfo.cpp src/history.cpp src/jobs.cpp src/signals.cpp \
       src/search.cpp src/redir.cpp

OBJDIR = build
OBJS = $(SRCS:src/%.cpp=$(OBJDIR)/%.o)
TARGET = mysh


all: $(OBJDIR) $(TARGET)


$(OBJDIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^


$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./mysh
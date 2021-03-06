# This file doesn't contain any changes specific to the local
# installation (eg. any specific library paths etc.). See the
# .config file for these.
#

-include .config
-include */*/*.d
-include */*.d

# Compilers and mandatory libraries
CXX ?= g++
CC ?= $(CXX)
LDLIBS += -lstdc++

# Boost specific flags
LDFLAGS_BOOST ?= -L/opt/local/lib
CPPFLAGS_BOOST ?= -I/opt/local/include
LDLIBS_BOOST ?= -lboost_system -lboost_serialization -lboost_thread

# Compiler flags and optional additional Boost flags too!
CXXFLAGS += -Wall -pedantic -ansi -Wno-long-long -MMD -MP -pthread
CPPFLAGS += -I. $(CPPFLAGS_BOOST) -pthread
LDFLAGS += $(LDFLAGS_BOOST) -pthread
LDLIBS += $(LDLIBS_BOOST)

# Suffix rules
%.o:	%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

# Compile oodles object files
NET_CORE_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard net/core/*.cpp))
NET_HTTP_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard net/http/*.cpp))
NET_OOP_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard net/oop/*.cpp))
SCHEDULER_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard sched/*.cpp))
CRAWLER_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard crawl/*.cpp))
UTILITY_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard utility/*.cpp))
COMMON_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard common/*.cpp))
URL_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard url/*.cpp))

# Compile unit test object files
TEST_OBJECTS := $(patsubst %.cpp,%.o,$(wildcard test/*.cpp))

# Binary targets
TESTS = test/html-parser \
	test/word-indexer \
	test/url-parser \
	test/url-tree \
	test/url-scheduler \
	test/allocator \
	test/events \
	test/protocol-handler \
	test/oop-messages \
	test/http-transfer

# Production system components
PROGRAMS = prog/scheduler \
	prog/crawler

test/html-parser: test/html-parser.o \
	$(COMMON_OBJECTS) \
	$(UTILITY_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/word-indexer: test/word-indexer.o \
	$(COMMON_OBJECTS) \
	$(UTILITY_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/url-parser: test/url-parser.o \
	$(COMMON_OBJECTS) \
	$(URL_OBJECTS) \
	$(UTILITY_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/url-tree: test/url-tree.o \
	$(COMMON_OBJECTS) \
	$(URL_OBJECTS) \
	$(UTILITY_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)
 
test/url-scheduler: test/url-scheduler.o \
	$(COMMON_OBJECTS) \
	$(URL_OBJECTS) \
	$(UTILITY_OBJECTS) \
	$(NET_CORE_OBJECTS) \
	$(NET_OOP_OBJECTS) \
	$(SCHEDULER_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/allocator: test/allocator.o \
	$(COMMON_OBJECTS) \
	$(UTILITY_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/events: test/events.o \
	$(COMMON_OBJECTS) \
	$(UTILITY_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/protocol-handler: test/protocol-handler.o \
	$(COMMON_OBJECTS) \
	$(UTILITY_OBJECTS) \
	$(NET_CORE_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/oop-messages: test/oop-messages.o \
	$(COMMON_OBJECTS) \
	$(URL_OBJECTS) \
	$(UTILITY_OBJECTS) \
	$(NET_CORE_OBJECTS) \
	$(NET_OOP_OBJECTS) \
	$(SCHEDULER_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

test/http-transfer: test/http-transfer.o \
	$(COMMON_OBJECTS) \
	$(URL_OBJECTS) \
	$(UTILITY_OBJECTS) \
	$(NET_CORE_OBJECTS) \
	$(NET_HTTP_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

prog/scheduler: prog/scheduler.o \
	$(COMMON_OBJECTS) \
	$(URL_OBJECTS) \
	$(UTILITY_OBJECTS) \
	$(NET_CORE_OBJECTS) \
	$(NET_OOP_OBJECTS) \
	$(SCHEDULER_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

prog/crawler: prog/crawler.o \
	$(COMMON_OBJECTS) \
	$(URL_OBJECTS) \
	$(UTILITY_OBJECTS) \
	$(NET_CORE_OBJECTS) \
	$(NET_OOP_OBJECTS) \
	$(SCHEDULER_OBJECTS) \
	$(CRAWLER_OBJECTS) ;\
	$(CXX) $(LDFLAGS) -o bin/$@ $^ $(LDLIBS)

# Phony targets
.PHONY: default all clean

# Make targets
default: all

all: $(TESTS) $(PROGRAMS)

clean:
	-find `pwd` -depth -type f -name '*.[od]' -prune \
		\! -path "`pwd`[/].git/*" -exec rm -f {} \;
	-rm -f bin/*/*

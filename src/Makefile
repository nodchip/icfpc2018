CXXFLAGS=-Wall
CXXFLAGS+=$(pkg-config --libs --cflags boost)
CXXFLAGS+=-I .
CXXFLAGS+=-I ../libs/
CXXFLAGS+=-I ../libs/googletest/googletest/include
CXXFLAGS+=-I ../libs/googletest/googletest
LDFLAGS+=-lpthread -lboost_program_options

BUILD_PATH=build

SRCS=nmms.cpp
OBJS=$(SRCS:%.cpp=$(BUILD_PATH)/%.o)

TEST_SRCS=../libs/googletest/googletest/src/gtest-all.cc
TEST_OBJS=$(TEST_SRCS:%.cpp=$(BUILD_PATH)/%.o)

.PHONY: all
all: dirs test stupid_engine

######################
# ADD YOUR ENGINE HERE
stupid_engine: engine_main.cpp engines/stupid_solver.cpp $(OBJS) $(TEST_OBJS)
	g++ $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)


.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_PATH)

test: test_main.cpp $(OBJS) $(TEST_OBJS)
	g++ $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

$(BUILD_PATH)/%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f test


CXXFLAGS=-Wall
CXXFLAGS+=$(pkg-config --libs --cflags boost)
CXXFLAGS+=-I .
CXXFLAGS+=-I ../libs/
CXXFLAGS+=-I ../libs/googletest/googletest/include
CXXFLAGS+=-I ../libs/googletest/googletest
LDFLAGS+=-lpthread -lboost_program_options

BUILD_PATH=build

SRCS=nmms.cpp
SRCS+=vec3.cpp
SRCS+=engine.cpp
OBJS=$(SRCS:%.cpp=$(BUILD_PATH)/%.o)

TEST_SRCS=../libs/googletest/googletest/src/gtest-all.cc
TEST_OBJS=$(TEST_SRCS:%.cpp=$(BUILD_PATH)/%.o)

.PHONY: all
all: dirs test engines

######################
# ADD YOUR ENGINE TARGET HERE
.PHONY: engines
engines: stupid_engine

######################
# ADD YOUR ENGINE HERE
stupid_engine: engine_main.cpp engines/stupid_solver.cpp $(OBJS) $(TEST_OBJS)
	g++ $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)


.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_PATH)
	@mkdir -p $(BUILD_PATH)/engines

test: test_main.cpp engines/stupid_solver.cpp $(OBJS) $(TEST_OBJS)
	g++ $(CXXFLAGS) -Wall -Wno-sign-compare $^ -o $@ $(LDFLAGS)

$(BUILD_PATH)/%.o: %.cpp %.h
	g++ $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(BUILD_PATH)/*.o
	rm -f $(BUILD_PATH)/engines/*.o
	rm -f test

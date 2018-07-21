CXX=g++
CXXFLAGS=-O2 -Wall -Wno-sign-compare
CXXFLAGS+=$(pkg-config --libs --cflags boost)
CXXFLAGS+=-I .
CXXFLAGS+=-I ../libs/
CXXFLAGS+=-I ../libs/googletest/googletest/
CXXFLAGS+=-I ../libs/googletest/googletest/include
CXXFLAGS+= -std=c++17
LDFLAGS+=-lpthread

BUILD_PATH=build

SRCS=command.cpp matrix.cpp nanobot.cpp nmms.cpp region.cpp vec3.cpp system.cpp trace.cpp
SRCS+=engine.cpp

OBJS=$(SRCS:%.cpp=$(BUILD_PATH)/%.o)

TEST_SRCS=../libs/googletest/googletest/src/gtest-all.cc
TEST_OBJS=$(TEST_SRCS:%.cpp=$(BUILD_PATH)/%.o)

.PHONY: all
all: dirs test engines

######################
# ADD YOUR ENGINE TARGET HERE
.PHONY: engines
engines: stupid_engine stupid_engine_v2

######################
# ADD YOUR ENGINE HERE
stupid_engine: engine_main.cpp engines/stupid_solver.cpp $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

stupid_engine_v2: engine_main.cpp engines/stupid_solver_v2.cpp $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

small: small.cpp engines/stupid_solver.cpp $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)


.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_PATH)
	@mkdir -p $(BUILD_PATH)/engines

test: test_main.cpp engines/stupid_solver.cpp engines/stupid_solver_v2.cpp $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_PATH)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(BUILD_PATH)/*.o
	rm -f $(BUILD_PATH)/engines/*.o
	rm -f test

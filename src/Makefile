CXX=g++
CXXFLAGS=-O2 -Wall -Wno-sign-compare -std=c++17
CXXFLAGS+=$(pkg-config --libs --cflags boost)
CXXFLAGS+=-I .
CXXFLAGS+=-I ../libs
CXXFLAGS+=-I ../libs/googletest/googletest/
CXXFLAGS+=-I ../libs/googletest/googletest/include
LDFLAGS+=-lpthread

BUILD_PATH=build
LIB_PATH=../libs

SRCS=command.cpp matrix.cpp nanobot.cpp nmms.cpp region.cpp vec3.cpp state.cpp system.cpp trace.cpp bounding_box.cpp
SRCS+=engine.cpp engines/naive_converter.cpp

OBJS=$(SRCS:%.cpp=$(BUILD_PATH)/%.o)

TEST_SRCS=$(LIB_PATH)/googletest/googletest/src/gtest-all.cc
TEST_OBJS=$(TEST_SRCS:%.cpp=$(BUILD_PATH)/%.o)

.PHONY: all
all: dirs test engines evaluate

######################
# ADD YOUR ENGINE TARGET HERE
.PHONY: engines
engines: stupid_engine stupid_engine_v2 parallel_stupid_engine parallel_stupid_engine_v2 parallel_stupid_engine_v3_1 3x3_solver

######################
# ADD YOUR ENGINE HERE
stupid_engine: engine_main.cpp engines/stupid_solver.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

stupid_engine_v2: engine_main.cpp engines/stupid_solver_v2.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

parallel_stupid_engine: engine_main.cpp engines/parallel_stupid_solver.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

parallel_stupid_engine_v2: engine_main.cpp engines/parallel_stupid_solver_v2.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

parallel_stupid_engine_v3_1: engine_main.cpp engines/parallel_stupid_solver_v3_1.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

3x3_solver: engine_main.cpp engines/3x3_solver.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -Wall $^ -o $@ $(LDFLAGS)

.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_PATH)
	@mkdir -p $(BUILD_PATH)/engines

test: test_main.cpp engines/stupid_solver.cpp engines/stupid_solver_v2.cpp engines/parallel_stupid_solver.cpp engines/parallel_stupid_solver_v2.cpp $(OBJS) $(TEST_OBJS)
	$(CXX) -DTEST_PROJECT $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

evaluate: evaluate_main.cpp $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

trace_sandbox: trace_sandbox_main.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_PATH)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean-results
clean-results:
	rm -f *.mdl *.nbt *.json
	rm -rf tmp-quick-info
	rm -rf tmp-quick-traces

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(BUILD_PATH)/*.o
	rm -f $(BUILD_PATH)/engines/*.o
	rm -f test

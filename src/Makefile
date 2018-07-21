CXXFLAGS=-Wall
CXXFLAGS+=$(pkg-config --libs --cflags boost)
CXXFLAGS+=-I ../libs/googletest/googletest/include
CXXFLAGS+=-I ../libs/googletest/googletest
LDFLAGS+=-lpthread

BUILD_PATH=build

SRCS=nmms.cpp stupid_solver.cpp
SRCS+=vec3.cpp
OBJS=$(SRCS:%.cpp=$(BUILD_PATH)/%.o)

TEST_SRCS=../libs/googletest/googletest/src/gtest-all.cc
TEST_OBJS=$(TEST_SRCS:%.cpp=$(BUILD_PATH)/%.o)

.PHONY: all
all: dirs test

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

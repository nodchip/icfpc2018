CXXFLAGS=-Wall
CXXFLAGS+=$(pkg-config --libs --cflags boost)

BUILD_PATH=build

SRCS=nmms.cpp
OBJS=$(SRCS:%.cpp=$(BUILD_PATH)/%.o)

.PHONY: all
all: dirs test

.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_PATH)

test: test_main.cpp $(OBJS)
	g++ $(CXXFLAGS) -Wall $< $(OBJS) -o $@

$(BUILD_PATH)/%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f test


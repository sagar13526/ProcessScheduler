CXX      := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -I./include
SRCS     := src/Process.cpp src/Scheduler.cpp src/Utils.cpp
OBJS     := $(SRCS:src/%.cpp=obj/%.o)
TARGET   := bin/scheduler
TEST     := bin/run_tests

all: $(TARGET)

$(TARGET): $(OBJS) obj/main.o | bin
	$(CXX) $^ -o $@

$(TEST): $(OBJS) tests/test_scheduler.cpp | bin
	$(CXX) $(CXXFLAGS) $^ -o $@

obj/%.o: src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj bin:
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

test: $(TEST)
	./$(TEST)

web: $(TARGET)
	python3 server.py

clean:
	rm -rf obj bin

rebuild: clean all

help:
	@echo "make          build bin/scheduler"
	@echo "make run      run the console simulator"
	@echo "make test     build and run the tests"
	@echo "make web      build, then start the web UI at http://localhost:8000"
	@echo "make clean    remove build output"

.PHONY: all run test web clean rebuild help

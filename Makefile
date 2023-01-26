# compiler
CXX 			:= g++
MKDIR			:= mkdir -p
RM				:= rm
MAKE			:= make

# flags
FLAGS			:= -Iinclude -MMD -MP
FLAGS 			+= -std=c++17

# warnings
FLAGS 			+= -Wall -Wextra

DBG_FLAGS		:= -Og -ggdb
REL_FLAGS		:= -O2

# libraries
LDFLAGS			+= -lfmt -lsfml-graphics -lsfml-window -lsfml-system

# names
TARGET			:= chess
BUILD			:= build
SRC				:= src

ifeq ($(BUILD_TYPE), RELEASE)
FLAGS			+= $(REL_FLAGS)
else
FLAGS			+= $(DBG_FLAGS)
endif

SRCS 			:= $(shell find $(SRC) -name *.cpp)
OBJS 			:= $(SRCS:%=$(BUILD)/%.o)
DEPS 			:= $(OBJS:.o=.d)

$(BUILD)/$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/%.cpp.o: %.cpp
	$(MKDIR) $(dir $@)
	$(CXX) $(FLAGS) -c $< -o $@

.PHONY: clean release

release:
	@$(MAKE) BUILD_TYPE=RELEASE

clean:
ifndef BUILD
$(error no BUILD directory set)
endif
	$(RM) -r $(BUILD)

-include $(DEPS)
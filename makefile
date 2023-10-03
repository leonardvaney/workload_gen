PROGRAM := workload
SRC_DIR := source
OBJ_DIR := obj

SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CPPFLAGS := -Iinclude -MMD -MP
LDLIBS := -lpthread

.PHONY: all clean

all: clean
all: $(PROGRAM)

local: CPPFLAGS += -DLOCAL
local: clean
local: all

$(PROGRAM) : $(OBJ)
	g++ $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	g++ $(CPPFLAGS) $(LDLIBS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@
	
clean:
	@$(RM) -rv $(OBJ_DIR) $(PROGRAM)	
	
-include $(OBJ:.o=.d)

CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra
READLINE_PREFIX = /opt/homebrew/opt/readline

INCLUDES = -I$(READLINE_PREFIX)/include
LIBS = -L$(READLINE_PREFIX)/lib -lreadline

TARGET = shell
SRC = shell.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(INCLUDES) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

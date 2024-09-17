# Compiler and flags
COMP = g++
FLAGS = -std=c++11 -g

# Directories and libraries
LIBS = -lportaudio -lfftw3 -lblas -lsndfile -lasound -lmp3lame -ldl -lpthread -lm -lGL -lGLU -lglfw -lGLEW -laubio -lmpg123 -lportaudio

# Source files and objects
SRC = main.cpp audioAnalyzer.cpp
OBJ = main.o audioAnalyzer.o

# Output executable
EXEC = ./fractal

# Default target
all: $(EXEC)

# # Compile sound_analysis.cpp to sound_analysis.o
# sound_analysis.o: sound_analysis.cpp
# 	$(COMP) $(FLAGS) -c sound_analysis.cpp -o sound_analysis.o
audioAnalyzer.o: audioAnalyzer.cpp
	$(COMP) $(FLAGS) -c audioAnalyzer.cpp -o audioAnalyzer.o

# Compile main.cpp to main.o
main.o: main.cpp
	$(COMP) $(FLAGS) -c main.cpp -o main.o

# Link the object files to create the executable
$(EXEC): $(OBJ)
	$(COMP) -g $(OBJ) -o $(EXEC) $(LIBS)

# Clean command to remove object files and the executable
clean:
	rm -f $(OBJ) $(EXEC)

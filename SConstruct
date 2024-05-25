# Import the SCons environment module
from SCons.Environment import Environment

# Create an environment for building
env = Environment()

# Define the source files for your Flipper app
source_files = ['notepad.c']

# Compile the source files into an executable
env.Program(target='notepad', source=source_files)

# Specify default target
Default(target='notepad')

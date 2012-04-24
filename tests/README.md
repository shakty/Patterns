# Patterns Tests

  Automated tests make sure that the actual behavior of a piece of software is consistent with its design.
  Patterns module comes with a large battery of automated tests which covers the main functionalities offered.
  
## Patterns Testing API

  Patterns is shipped with a `PatternTestCase` abstract class that aims at helping developers of [SimpleTest](http://drupal.org/node/291740).
  
  Here are some methods that are worth knowing:
  
    - `getPatternsTestDir()`: Returns the path to the tests directory inside the Patterns module.
    - `setUp()`: Loads the patterns and libraries modules plus all those that are passed as parameters; skips the Patterns splash screen.
    - `runFile()`: Loads a pattern file and runs it through the quick run interface.
    - `quickRun()`: Takes a string representing a pattern as input and runs it through the quick run interface.
    - `runDir()`: Scans a directory for patterns files, and executes them.
    - `callbackOnDir()`: Loads all the pattern files from a directory and executes a callback on each parsed pattern. 
    

## TODO

The following parts of the Patterns module still need a test. Feel free to contribute! 

  - Check if all the pages are loaded without errors
  - Add specific tests for Lab page, Export page, Public Page
  - YAML parser
  - XML parser
  - PHP parser
  - First Install screen
  - Patterns D2D
	- How tagmodules and modulestag indexes are built

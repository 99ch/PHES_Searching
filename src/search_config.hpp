#ifndef SEARCH_CONFIG_H
#define SEARCH_CONFIG_H

#include <string>
#include <iostream>
#include "coordinates.h"


class SearchType {
  public:
    // Enum for different search types
    enum type {GREENFIELD, OCEAN, SINGLE_EXISTING, BULK_EXISTING, BULK_PIT, SINGLE_PIT};
    // Constructor to initialize search type
    constexpr SearchType(type search_type) : value(search_type){}
    // Operator to return the search type
    constexpr operator type() const { return value; }

    // Check if the search type is for existing reservoirs
    bool existing(){
      return value == SINGLE_EXISTING || value == BULK_EXISTING || value == BULK_PIT || value == SINGLE_PIT;
    }
    // Check if the search type is not for existing reservoirs
    bool not_existing(){
      return value == GREENFIELD || value == OCEAN;
    }
    // Check if the search type is for grid cells
    bool grid_cell(){
      return value == GREENFIELD || value == OCEAN || value == BULK_EXISTING || value == BULK_PIT;
    }
    // Check if the search type is for a single reservoir
    bool single(){
      return value == SINGLE_EXISTING || value == SINGLE_PIT;
    }

    // Filename prefix
    string prefix(){
      switch(value){
        case OCEAN:
          return "ocean_";
        case SINGLE_PIT:
          return "single_pit_";
        case BULK_PIT:
          return "pit_";
        case BULK_EXISTING:
          return "existing_";
        default:
          return "";
      }
    }

    // Used when searching for 8 neighbouring input cells in pairing (i.e. when
    // doing an ocean search we want to read ocean lowers, in all other cases
    // regular neighbours)
    std::string lowers_prefix() {
      switch (value) {
      case OCEAN:
        return "ocean_";
      case BULK_EXISTING:
        return "existing_";
      default:
        return "";
      }
    }

  private:
    // Variable to store the search type
    type value;
};

// Class to handle logging
class Logger {
  public:
    // Enum for logging levels
    enum level {DEBUG, ERROR};
    // Constructor to initialize logging level
    constexpr Logger(level logging_level) : logging_level(logging_level){}
    // Constructor to initialize logging level from a character
    Logger(char* c){
      if (atoi(c))
        logging_level = DEBUG;
      else
        logging_level = ERROR;
    }
    // Operator to return the logging level
    constexpr operator level() const { return logging_level; }

    // Log an error message
    void error(std::string message){
      std::cout << message << std::endl;
    }

    // Check if debug output is enabled
    bool output_debug(){
      return logging_level == DEBUG;
    }

    // Log a debug message
    void debug(std::string message){
      if (this->output_debug())
        std::cout << message << std::endl;
    }
    // Log a warning message
    void warning(std::string message){
      if (this->output_debug())
        std::cout << message << std::endl;
    }

  private:
    // Variable to store the logging level
    level logging_level = DEBUG;
};

// Function to format a string for use as a filename
string format_for_filename(string s);

// Class to handle search configuration
class SearchConfig {
  public:
    // Variables to store search type, grid square, name, and logger
    SearchType search_type;
    GridSquare grid_square;
    std::string name;
    Logger logger;

    // Default constructor
    SearchConfig() : search_type(SearchType::GREENFIELD), logger(Logger::ERROR){}
    // Constructor to initialize search configuration from command line arguments
    SearchConfig(int nargs, char **argv) : search_type(SearchType::GREENFIELD), logger(Logger::ERROR) {
      std::string arg1(argv[1]);
      int adj = 0;
      if (arg1.compare("ocean") == 0) {
        search_type = SearchType::OCEAN;
        adj = 1;
        arg1 = argv[1 + adj];
      }
      if (arg1.compare("bulk_existing") == 0) {
        search_type = SearchType::BULK_EXISTING;
        adj = 1;
        arg1 = argv[1 + adj];
      } else if (arg1.compare("bulk_pit") == 0) {
        search_type = SearchType::BULK_PIT;
        adj = 1;
        arg1 = argv[1 + adj];
      }
      if (arg1.compare("pit") == 0) {
        search_type = SearchType::SINGLE_PIT;
        adj = 1;
        arg1 = argv[1 + adj];
        name = arg1;
        if (nargs > 2 + adj)
          logger = Logger(argv[2 + adj]);
      } else if (arg1.compare("reservoir") == 0) {
        search_type = SearchType::SINGLE_EXISTING;
        adj = 1;
        arg1 = argv[1 + adj];
        name = arg1;
        if (nargs > 2 + adj)
          logger = Logger(argv[2 + adj]);
      } else {
        try {
          int lon = stoi(arg1);
          grid_square = GridSquare_init(atoi(argv[2 + adj]), lon);
          if (nargs > 3 + adj)
            logger = Logger(argv[3 + adj]);
        } catch (exception &e) {
          search_type = SearchType::SINGLE_EXISTING;
          name = arg1;
          if (nargs > 2 + adj)
            logger = Logger(argv[2 + adj]);
        }
      }
    }

    // Get the filename based on the search type and name or grid square
    std::string filename(){
      if(search_type.grid_cell())
        return search_type.prefix() + str(grid_square);
      return search_type.prefix() + format_for_filename(name);
    }
};

// Declare global instance of SearchConfig
extern SearchConfig search_config;

#endif

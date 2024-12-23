#ifndef CSV_H
#define CSV_H

#include "phes_base.h"
#include <string>
#include <vector>

// Function to write a vector of strings to a CSV file
void write_to_csv_file(FILE *csv_file, vector<string> cols);

// Function to read a line from a CSV file and split it into a vector of strings
vector<string> read_from_csv_file(string line);

// Function to read a line from a CSV file with a specified delimiter and split it into a vector of strings
vector<string> read_from_csv_file(string line, char delimeter);

// Function to read existing reservoir data from a CSV file
vector<ExistingReservoir> read_existing_reservoir_data(const char* filename);

// Function to read existing pit data from a CSV file
vector<ExistingPit> read_existing_pit_data(char* filename);

// Function to read names from a CSV file
vector<string> read_names(char* filename);

// Function to write the header for rough reservoir data to a CSV file
void write_rough_reservoir_csv_header(FILE *csv_file);

// Function to write the header for rough reservoir data with additional fields to a CSV file
void write_rough_reservoir_data_header(FILE *csv_file);

// Function to write rough reservoir data to a CSV file
void write_rough_reservoir_csv(FILE *csv_file, RoughReservoir reservoir);

// Function to write rough reservoir data to a CSV file with additional fields
void write_rough_reservoir_data(FILE *csv_file, RoughReservoir *reservoir);

// Function to read rough reservoir data from a CSV file
vector<unique_ptr<RoughReservoir>> read_rough_reservoir_data(char *filename);

// Function to write the header for rough pair data to a CSV file
void write_rough_pair_csv_header(FILE *csv_file);

// Function to write the header for rough pair data with additional fields to a CSV file
void write_rough_pair_data_header(FILE *csv_file);

// Function to write rough pair data to a CSV file
void write_rough_pair_csv(FILE *csv_file, Pair *pair);

// Function to write rough pair data to a CSV file with additional fields
void write_rough_pair_data(FILE *csv_file, Pair *pair);
vector<vector<Pair> > read_rough_pair_data(char* filename);

// Function to write the header for pair data to a CSV file
void write_pair_csv_header(FILE *csv_file, bool output_FOM);

// Function to write pair data to a CSV file
void write_pair_csv(FILE *csv_file, Pair *pair, bool output_FOM);

// Function to write the header for summary data to a CSV file
void write_summary_csv_header(FILE *csv_file);

// Function to write summary data to a CSV file
void write_summary_csv(FILE *csv_file, string square_name, string test,
                      int non_overlapping_sites, int num_sites,
                      int energy_capacity);

#endif

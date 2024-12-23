#ifndef KML_H
#define KML_H

#include "phes_base.h"

// Structure to hold KML data for different elements
struct KML_Holder{
	vector<string> uppers;
	vector<string> lowers;
	vector<string> upper_dams;
	vector<string> lower_dams;
	vector<string> lines;
	vector<string> points;
};

// Structure to hold KML coordinates for a reservoir
struct Reservoir_KML_Coordinates{
	string reservoir;
	vector<string> dam;
	bool is_turkeys_nest;
};

// Structure to hold KML data for a pair
struct Pair_KML{
	Reservoir_KML_Coordinates upper;
	Reservoir_KML_Coordinates lower;
	string point;
	string line;
};

// Function to generate KML output
string output_kml(KML_Holder* kml_holder, string square, Test test);

// Function to update the KML holder with new data
void update_kml_holder(KML_Holder* kml_holder, Pair* pair, Pair_KML* pair_kml, bool keep_upper, bool keep_lower);

// Function to generate KML geometry for a reservoir
string get_reservoir_geometry(Reservoir_KML_Coordinates coordinates);

// Function to generate KML geometry for a dam
string get_dam_geometry(Reservoir_KML_Coordinates coordinates);

// Function to generate KML for a dam
string get_dam_kml(Reservoir* reservoir, Reservoir_KML_Coordinates coordinates);

// Function to generate KML for a reservoir
string get_reservoir_kml(Reservoir* reservoir, string colour, Reservoir_KML_Coordinates coordinates, Pair* pair);

// External KML start string
extern string kml_start;

// External KML end string
extern string kml_end;

// void write_fusion_csv_header(FILE *csv_file);
// void write_fusion_csv(FILE *csv_file, Pair *pair, Pair_KML* pair_kml);

#endif

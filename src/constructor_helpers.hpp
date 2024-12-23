#ifndef CONSTRUCTOR_HELPER_H
#define CONSTRUCTOR_HELPER_H

#include "phes_base.h"
#include "kml.h"

// Finds intersections of a polygon with a given latitude
vector<double> find_polygon_intersections(double lat, vector<GeographicCoordinate> &polygon);

// Checks if a point is within any of the given polygons
bool check_within(GeographicCoordinate point, vector<vector<GeographicCoordinate>> polygons);

// Reads country data from a file and populates the country names
vector<vector<vector<GeographicCoordinate>>> read_countries(string filename, vector<string>& country_names);

// Gets adjacent cells between two points
ArrayCoordinate* get_adjacent_cells(ArrayCoordinate point1, ArrayCoordinate point2);

// Checks if the edge between two points meets a certain threshold
bool is_edge(ArrayCoordinate point1, ArrayCoordinate point2, Model<char>* model, ArrayCoordinate offset, int threshold);

// Checks if the edge between two points is a dam wall
bool is_dam_wall(ArrayCoordinate point1, ArrayCoordinate point2, Model<short>* DEM, ArrayCoordinate offset, double wall_elevation);

// Converts a model to a polygon based on a threshold
vector<ArrayCoordinate> convert_to_polygon(Model<char>* model, ArrayCoordinate offset, ArrayCoordinate pour_point, int threshold);

// Converts a polygon of array coordinates to geographic coordinates
vector<GeographicCoordinate> convert_poly(vector<ArrayCoordinate> polygon);

// Applies a corner-cutting algorithm to a polygon
vector<GeographicCoordinate> corner_cut_poly(vector<GeographicCoordinate> polygon);

// Compresses a polygon to reduce the number of points
vector<GeographicCoordinate> compress_poly(vector<GeographicCoordinate> polygon);

// Converts a polygon to a string representation with elevation
string str(vector<GeographicCoordinate> polygon, double elevation);

// Models a reservoir and updates its properties
bool model_reservoir(Reservoir *reservoir,
                     Reservoir_KML_Coordinates *coordinates, Model<bool> *seen,
                     bool *non_overlap, vector<ArrayCoordinate> *used_points,
                     BigModel big_model, Model<char> *full_cur_model,
                     vector<vector<vector<GeographicCoordinate>>> &countries,
                     vector<string> &country_names);

#endif

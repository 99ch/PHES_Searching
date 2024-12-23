#ifndef POLYGON_H
#define POLYGON_H

#include "phes_base.h"

// Function to find the intersections of a polygon with a given row in a raster.
// Returns a vector of longitudes where the polygon intersects the row.
vector<double> find_polygon_intersections(int row, vector<GeographicCoordinate> &polygon, Model<bool>* filter);

// Function to convert a polygon to a raster representation.
// Sets the raster cells to true for the area within the polygon.
void polygon_to_raster(vector<GeographicCoordinate> &polygon, Model<bool>* raster);

// Function to read a shapefile and filter polygons based on the provided filter model.
// Converts the relevant polygons to raster representation.
void read_shp_filter(string filename, Model<bool>* filter);

// Function to calculate the area of a geographic polygon.
// Returns the area in hectares.
double geographic_polygon_area(vector<GeographicCoordinate> polygon);

#endif
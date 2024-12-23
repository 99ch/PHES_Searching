#include <bits/stdc++.h>
using namespace std;

#include "phes_base.h"

// Function to update the reservoir boundary based on the elevation above the pour point
void update_reservoir_boundary(
    vector<array<ArrayCoordinate, directions.size()>> &dam_shape_bounds,
    ArrayCoordinate point, int elevation_above_pp) {
  for (uint ih = 0; ih < dam_wall_heights.size(); ih++) {
    int dam_height = dam_wall_heights[ih];
    if (dam_height >= elevation_above_pp)
      for (uint i = 0; i < directions.size(); i++) {
        if ((directions[i].row * point.row + directions[i].col * point.col) >
            (directions[i].row * dam_shape_bounds[ih][i].row +
             directions[i].col * dam_shape_bounds[ih][i].col)) {
          dam_shape_bounds[ih][i] = point;
        }
      }
  }
}

// Function to update the reservoir boundary without considering elevation
void update_reservoir_boundary(
    vector<ArrayCoordinate> &dam_shape_bounds,
    ArrayCoordinate point) {
  for (uint i = 0; i < directions.size(); i++) {
    if ((directions[i].row * point.row + directions[i].col * point.col) >
        (directions[i].row * dam_shape_bounds[i].row +
         directions[i].col * dam_shape_bounds[i].col)) {
      dam_shape_bounds[i].row = point.row;
      dam_shape_bounds[i].col = point.col;
    }
  }
}

// Function to initialize an ExistingReservoir object
ExistingReservoir ExistingReservoir_init(string identifier, double latitude,
                                         double longitude, int elevation,
                                         double volume) {
  ExistingReservoir reservoir;
  reservoir.identifier = identifier;
  reservoir.elevation = elevation;
  reservoir.latitude = latitude;
  reservoir.longitude = longitude;
  reservoir.volume = volume;
  return reservoir;
}

// Function to initialize an ExistingPit object
ExistingPit ExistingPit_init(ExistingReservoir reservoir) {
  ExistingPit pit;
  pit.reservoir = reservoir;
  return pit;
}

// Function to get the grid square coordinate of an ExistingReservoir
GridSquare get_square_coordinate(ExistingReservoir reservoir) {
  return GridSquare_init(convert_to_int(FLOOR(reservoir.latitude)),
                         convert_to_int(FLOOR(reservoir.longitude)));
}

// Function to initialize a Reservoir object
Reservoir Reservoir_init(ArrayCoordinate pour_point, int elevation) {
  Reservoir reservoir;
  reservoir.brownfield = false;
  reservoir.pit = false;
  reservoir.elevation = elevation;
  GeographicCoordinate geo_coordinate = convert_coordinates(pour_point);
  reservoir.latitude = geo_coordinate.lat;
  reservoir.longitude = geo_coordinate.lon;
  reservoir.pour_point = pour_point;
  for (uint idir = 0; idir < directions.size(); idir++)
    reservoir.shape_bound.push_back(pour_point);
  return reservoir;
}

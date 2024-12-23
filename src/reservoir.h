#ifndef RESERVOIR_H
#define RESERVOIR_H

#include "phes_base.h"

// Class representing a rough reservoir
class RoughReservoir{
  public:
    string identifier;
    bool brownfield = false;
    bool river = false;
    bool ocean = false;
    bool pit = false;
    bool turkey = false;
    double latitude = 0;
    double longitude = 0;
    int elevation = INT_MIN;
    ArrayCoordinate pour_point;
    vector<double> volumes;
    vector<double> dam_volumes;
    vector<double> areas;
    vector<double> water_rocks;
    double watershed_area = 0;
    double max_dam_height = 0;
    int bottom_elevation;

    // Default constructor
    RoughReservoir() {};
    // Virtual destructor
    virtual ~RoughReservoir() = default;
    // Constructor with pour point and elevation
    RoughReservoir(const ArrayCoordinate &pour_point, int elevation)
        : brownfield(false), ocean(false), pit(false), turkey(false), elevation(elevation),
          pour_point(pour_point), watershed_area(0), max_dam_height(max_wall_height),
          bottom_elevation(elevation) {
      GeographicCoordinate geo_coordinate = convert_coordinates(pour_point);
      this->latitude = geo_coordinate.lat;
      this->longitude = geo_coordinate.lon;
    }

    bool operator<(const RoughReservoir &o) const
        {
      return elevation > o.elevation;
        }
  private:
};

// Class representing a rough greenfield reservoir
class RoughGreenfieldReservoir : public RoughReservoir {
public:
  vector<array<ArrayCoordinate, directions.size()>> shape_bound;

  // Constructor initializing from a RoughReservoir object
  explicit RoughGreenfieldReservoir(const RoughReservoir& r)
      : RoughReservoir(r) {
    for (uint ih = 0; ih < dam_wall_heights.size(); ih++) {
      array<ArrayCoordinate, directions.size()> temp_array;
      for (uint idir = 0; idir < directions.size(); idir++) {
        temp_array[idir].row = pour_point.row;
        temp_array[idir].col = pour_point.col;
      }
      this->shape_bound.push_back(temp_array);
    }
  }
};

// Class representing a rough brownfield reservoir
class RoughBfieldReservoir : public RoughReservoir {
public:
  vector<ArrayCoordinate> shape_bound;
  vector<int> elevations;
  // Default constructor
  RoughBfieldReservoir() {};
  // Constructor initializing from a RoughReservoir object
  explicit RoughBfieldReservoir(const RoughReservoir &r) : RoughReservoir(r) {}
};

// Struct representing an existing reservoir
struct ExistingReservoir {
  string identifier;
  double latitude;
  double longitude;
  int elevation;
  int bottom_elevation;
  double volume;
  double area;
  bool river = false;
  vector<GeographicCoordinate> polygon;
};

// Struct representing an altitude-volume pair
struct AltitudeVolumePair {
  int altitude;
  double volume;
  // Comparison operator for sorting by altitude
  bool operator<(const AltitudeVolumePair &o) const { return altitude < o.altitude; }
};

// Struct representing an existing pit
struct ExistingPit {
  ExistingReservoir reservoir;
  vector<AltitudeVolumePair> volumes;
};

// Class representing a reservoir
class Reservoir {
  public:
    string identifier;
    bool brownfield;
    bool river;
    bool pit;
    bool ocean;
    bool turkey = false;
    double latitude;
    double longitude;
    int elevation;
    ArrayCoordinate pour_point;
    double volume;
    double dam_volume;
    double dam_length;
    double area;
    double water_rock;
    double watershed_area;
    double average_water_depth;
    double dam_height;
    double max_dam_height;
    string country;
    vector<ArrayCoordinate> shape_bound;
    // Comparison operator for sorting by elevation
    bool operator<(const Reservoir &o) const { return elevation > o.elevation; }
};

// Struct representing a pair of reservoirs

struct Pair {
  Reservoir upper;
  Reservoir lower;
  string identifier;
  double distance;
  double pp_distance;
  double slope;
  double required_volume;
  double volume;
  double FOM;
  char category;
  double water_rock;
  double energy_capacity;
  int storage_time;
  int head;
  int non_overlap;
  string country;
  // Comparison operator for sorting by FOM
  bool operator<(const Pair &o) const { return FOM < o.FOM; }
};

// Function to update the reservoir boundary based on elevation above pour point
void update_reservoir_boundary(vector<array<ArrayCoordinate, directions.size()>> &dam_shape_bounds,
                               ArrayCoordinate point, int elevation_above_pp);
// Function to update the reservoir boundary without considering elevation
void update_reservoir_boundary(vector<ArrayCoordinate> &dam_shape_bounds,
                               ArrayCoordinate point);
// Function to initialize a Reservoir object
Reservoir Reservoir_init(ArrayCoordinate pour_point, int elevation);
// Function to initialize an ExistingReservoir object
ExistingReservoir ExistingReservoir_init(string identifier, double latitude, double longitude,
                                         int elevation, double volume);
// Function to initialize an ExistingPit object
ExistingPit ExistingPit_init(ExistingReservoir reservoir);
// Function to get the grid square coordinate of an ExistingReservoir
GridSquare get_square_coordinate(ExistingReservoir reservoir);

#endif

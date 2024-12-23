#ifndef COORDINATES_H
#define COORDINATES_H

#include <string>

using namespace std;

// Forward declaration of structures and template class
struct GeographicCoordinate;
struct ArrayCoordinate;
template <class T> class Model;

// Structure representing a grid square with latitude and longitude
struct GridSquare {
  int lat, lon;
};

// Structure representing an array coordinate with height
struct ArrayCoordinateWithHeight {
  short row, col;
  double h;
  // Comparison operator to sort by height in descending order
  bool operator<(const ArrayCoordinateWithHeight &o) const { return h > o.h; }
};

// Function to initialize a GeographicCoordinate with latitude and longitude
GeographicCoordinate GeographicCoordinate_init(double latitude,
                                               double longitude);

// Function to initialize an ArrayCoordinate with row and column
ArrayCoordinate ArrayCoordinate_init(int row, int col);

// Function to initialize an ArrayCoordinate with row, column, and origin
ArrayCoordinate ArrayCoordinate_init(int row, int col,
                                     GeographicCoordinate origin);

// Function to initialize a GridSquare with latitude and longitude
GridSquare GridSquare_init(int latitude, int longitude);

// Function to initialize an ArrayCoordinateWithHeight with row, column, and height
ArrayCoordinateWithHeight ArrayCoordinateWithHeight_init(int row, int col,
                                                         double h);

// Function to calculate the origin of a GridSquare with a given border
GeographicCoordinate get_origin(GridSquare square, int border);

// Function to check if an ArrayCoordinateWithHeight is within the given shape
bool check_within(ArrayCoordinateWithHeight c, int shape[2]);

// Function to check if an ArrayCoordinate is within the given shape
bool check_within(ArrayCoordinate c, int shape[2]);

// Function to check if a GeographicCoordinate is within a GridSquare
bool check_within(GeographicCoordinate gc, GridSquare gs);

// Function to convert a GridSquare to a string representation
string str(GridSquare square);

// Function to calculate the area of a single cell in hectares
double find_area(ArrayCoordinate c);

// Function to calculate the distance between two ArrayCoordinates
double find_distance(ArrayCoordinate c1, ArrayCoordinate c2);

// Function to calculate the distance between two ArrayCoordinates with a given cosine latitude
double find_distance(ArrayCoordinate c1, ArrayCoordinate c2, double coslat);

// Function to calculate the squared distance between two ArrayCoordinates
double find_distance_sqd(ArrayCoordinate c1, ArrayCoordinate c2);

// Function to calculate the squared distance between two ArrayCoordinates with a given cosine latitude
double find_distance_sqd(ArrayCoordinate c1, ArrayCoordinate c2, double coslat);

// Function to calculate the distance between two GeographicCoordinates
double find_distance(GeographicCoordinate c1, GeographicCoordinate c2);

// Function to calculate the distance between two GeographicCoordinates with a given cosine latitude
double find_distance(GeographicCoordinate c1, GeographicCoordinate c2,
                     double coslat);

// Function to calculate the squared distance between two GeographicCoordinates
double find_distance_sqd(GeographicCoordinate c1, GeographicCoordinate c2);

// Function to calculate the squared distance between two GeographicCoordinates with a given cosine latitude
double find_distance_sqd(GeographicCoordinate c1, GeographicCoordinate c2,
                         double coslat);

// Function to convert an ArrayCoordinate to a GeographicCoordinate with a given offset
GeographicCoordinate convert_coordinates(ArrayCoordinate c, double offset=0.5);

// Function to convert a GeographicCoordinate to an ArrayCoordinate with a given origin
ArrayCoordinate convert_coordinates(GeographicCoordinate c,
                                    GeographicCoordinate origin);

// Function to convert a GeographicCoordinate to an ArrayCoordinate with a given origin, latitude resolution, and longitude resolution
ArrayCoordinate convert_coordinates(GeographicCoordinate c,
                                    GeographicCoordinate origin, double lat_res,
                                    double lon_res);

// Function to find the orthogonal nearest neighbor distance between two ArrayCoordinates
double find_orthogonal_nn_distance(ArrayCoordinate c1, ArrayCoordinate c2);

#endif

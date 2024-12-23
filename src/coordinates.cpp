#include "phes_base.h"

// Initializes a GeographicCoordinate with given latitude and longitude
GeographicCoordinate GeographicCoordinate_init(double latitude, double longitude)
{
	GeographicCoordinate geographic_coordinate;
	geographic_coordinate.lat = latitude;
	geographic_coordinate.lon = longitude;
	return geographic_coordinate;
}

// Calculates the origin of a GridSquare with a given border
GeographicCoordinate get_origin(GridSquare square, int border)
{
	GeographicCoordinate geographic_coordinate;
	geographic_coordinate.lat = square.lat+(1+((border-1)+0.5)/3600.0);
	geographic_coordinate.lon = square.lon-((border+0.5)/3600.0);
	return geographic_coordinate;
}

// Initializes an ArrayCoordinate with given row, column, and origin
ArrayCoordinate ArrayCoordinate_init(int row, int col, GeographicCoordinate origin) //Ditch at some point?
{
	ArrayCoordinate array_coordinate;
	array_coordinate.row = row;
	array_coordinate.col = col;
	array_coordinate.origin = origin;
	return array_coordinate;
}

// Initializes an ArrayCoordinateWithHeight with given row, column, and height
ArrayCoordinateWithHeight ArrayCoordinateWithHeight_init(int row, int col, double h)
{
	ArrayCoordinateWithHeight array_coordinate;
	array_coordinate.row = (short)row;
	array_coordinate.col = (short)col;
	array_coordinate.h = h;
	return array_coordinate;
}

// Checks if an ArrayCoordinateWithHeight is within the given shape
bool check_within(ArrayCoordinateWithHeight c, int shape[2])
{
	if(c.row>=0 && c.col>=0 && c.row<shape[0] && c.col<shape[1]){
        	return true;
	}
        return false;
}

// Checks if an ArrayCoordinate is within the given shape
bool check_within(ArrayCoordinate c, int shape[2])
{
	if(c.row>=0 && c.col>=0 && c.row<shape[0] && c.col<shape[1]){
        	return true;
	}
        return false;
}

// Checks if a GeographicCoordinate is within a GridSquare
bool check_within(GeographicCoordinate gc, GridSquare gs){
  return convert_to_int(FLOOR(gc.lat)) == gs.lat && convert_to_int(FLOOR(gc.lon)) == gs.lon;
}

// Checks if an ArrayCoordinate is strictly within the given shape
bool check_strictly_within(ArrayCoordinate c, int shape[2])
{
	if(c.row>0 && c.col>0 && c.row<shape[0]-1 && c.col<shape[1]-1){
        	return true;
	}
        return false;
}

// Initializes a GridSquare with given latitude and longitude
GridSquare GridSquare_init(int latitude, int longitude)
{
	GridSquare grid_square;
	grid_square.lat = latitude;
	grid_square.lon = longitude;
	return grid_square;
}

// Converts a GridSquare to a string representation
string str(GridSquare square)
{
	char buf[24];
	square.lon = (square.lon+180)%360-180;
	char c1 = (square.lat<0)?'s':'n';
	int lat = abs(square.lat);
	char c2 = (square.lon<0)?'w':'e';
	int lon = abs(square.lon);
	sprintf(buf, "%c%02d_%c%03d", c1, lat, c2, lon);
	string to_return(buf);
	return to_return;
}

// Calculates the area of a single cell in hectares
double find_area(ArrayCoordinate c)
{
	GeographicCoordinate p = convert_coordinates(c);
	return (0.0001*resolution*resolution)*COS(RADIANS(p.lat));
}

// Calculates the distance between two ArrayCoordinates
double find_distance(ArrayCoordinate c1, ArrayCoordinate c2)
{
	GeographicCoordinate p1 = convert_coordinates(c1);
	GeographicCoordinate p2 = convert_coordinates(c2);
	return find_distance(p1, p2);
}

// Calculates the distance between two ArrayCoordinates with a given cosine latitude
double find_distance(ArrayCoordinate c1, ArrayCoordinate c2, double coslat)
{
	GeographicCoordinate p1 = convert_coordinates(c1);
	GeographicCoordinate p2 = convert_coordinates(c2);
	return find_distance(p1, p2, coslat);
}

// Calculates the squared distance between two ArrayCoordinates
double find_distance_sqd(ArrayCoordinate c1, ArrayCoordinate c2)
{
	GeographicCoordinate p1 = convert_coordinates(c1);
	GeographicCoordinate p2 = convert_coordinates(c2);
	return find_distance_sqd(p1, p2);
}

// Calculates the squared distance between two ArrayCoordinates with a given cosine latitude
double find_distance_sqd(ArrayCoordinate c1, ArrayCoordinate c2, double coslat)
{
  if(c1.origin.lat==c2.origin.lat && c1.origin.lon==c2.origin.lon)
    return (SQ(c2.row-c1.row) + SQ((c2.col-c1.col)*coslat))*SQ(resolution*0.001);
	GeographicCoordinate p1 = convert_coordinates(c1);
	GeographicCoordinate p2 = convert_coordinates(c2);
	return find_distance_sqd(p1, p2, coslat);
}

// Calculates the distance between two GeographicCoordinates
double find_distance(GeographicCoordinate c1, GeographicCoordinate c2)
{
	return SQRT(find_distance_sqd(c1, c2));
}

// Calculates the distance between two GeographicCoordinates with a given cosine latitude
double find_distance(GeographicCoordinate c1, GeographicCoordinate c2, double coslat)
{
	return SQRT(find_distance_sqd(c1, c2, coslat));
}

// Calculates the squared distance between two GeographicCoordinates
double find_distance_sqd(GeographicCoordinate c1, GeographicCoordinate c2)
{
	return (SQ(c2.lat-c1.lat)+SQ((c2.lon-c1.lon)*COS(RADIANS(0.5*(c1.lat+c2.lat)))))*SQ(3600*resolution*0.001);
}

// Calculates the squared distance between two GeographicCoordinates with a given cosine latitude
double find_distance_sqd(GeographicCoordinate c1, GeographicCoordinate c2, double coslat)
{
	return (SQ(c2.lat-c1.lat)+SQ((c2.lon-c1.lon)*coslat))*SQ(3600*resolution*0.001);
}

// Converts a GeographicCoordinate to an ArrayCoordinate with a given origin
ArrayCoordinate convert_coordinates(GeographicCoordinate c, GeographicCoordinate origin)
{
	return ArrayCoordinate_init(convert_to_int((origin.lat-c.lat)*3600-0.5), convert_to_int((c.lon-origin.lon)*3600-0.5), origin);
}

// Converts a GeographicCoordinate to an ArrayCoordinate with a given origin, latitude resolution, and longitude resolution
ArrayCoordinate convert_coordinates(GeographicCoordinate c, GeographicCoordinate origin, double lat_res, double lon_res){
	return ArrayCoordinate_init(convert_to_int((c.lat-origin.lat)/lat_res-0.5), convert_to_int((c.lon-origin.lon)/lon_res-0.5), origin);
}

// Converts an ArrayCoordinate to a GeographicCoordinate with a given offset
GeographicCoordinate convert_coordinates(ArrayCoordinate c, double offset)
{
	return GeographicCoordinate_init(c.origin.lat-(c.row+offset)/3600.0, c.origin.lon+(c.col+offset)/3600.0);
}

// Finds the orthogonal nearest neighbor distance between two ArrayCoordinates
double find_orthogonal_nn_distance(ArrayCoordinate c1, ArrayCoordinate c2)
{
	if (c1.col == c2.col)
		return resolution;

	GeographicCoordinate p1 = convert_coordinates(c1);
	GeographicCoordinate p2 = convert_coordinates(c2);
	return (COS(RADIANS(0.5*(p1.lat+p2.lat)))*resolution);
}
#include "polygons.h"
#include "model2D.h"

// find_polygon_intersections returns an array containing the longitude of all line. Assumes last coordinate is same as first
vector<double> find_polygon_intersections(int row, vector<GeographicCoordinate> &polygon, Model<bool>* filter){
    vector<double> to_return;
    double lat = filter->get_coordinate(row, 0).lat;
    for(uint i = 0; i<polygon.size()-1; i++){
        GeographicCoordinate line[2] = {polygon[i], polygon[(i+1)]};
        // Check if the line crosses the latitude of the current row
        if((line[0].lat < lat && line[1].lat>=lat) || (line[0].lat >= lat && line[1].lat < lat)){
            to_return.push_back(line[0].lon+(lat-line[0].lat)/(line[1].lat-line[0].lat)*(line[1].lon-line[0].lon));
        }
    }
    // Sort the intersections by longitude
    sort(to_return.begin(), to_return.end());
    return to_return;
}

// polygon_to_raster converts a polygon to a raster representation.
void polygon_to_raster(vector<GeographicCoordinate> &polygon, Model<bool>* raster){
    for(int row =0; row<raster->nrows(); row++){
        // Find intersections of the polygon with the current row
        vector<double> polygon_intersections = find_polygon_intersections(row, polygon, raster);
        // Convert geographic coordinates to raster coordinates
        for(uint j = 0; j<polygon_intersections.size();j++)
            polygon_intersections[j] = (convert_coordinates(GeographicCoordinate_init(0, polygon_intersections[j]),raster->get_origin()).col);
        // Set raster cells to true for the area within the polygon
        for(uint j = 0; j<polygon_intersections.size()/2;j++)
            for(int col=polygon_intersections[2*j];col<polygon_intersections[2*j+1];col++)
                if(raster->check_within(row, col))
                    raster->set(row,col,true);
    }
}

// read_shp_filter reads a shapefile and filters polygons based on the provided filter model.
void read_shp_filter(string filename, Model<bool>* filter){
	char *shp_filename = new char[filename.length() + 1];
	strcpy(shp_filename, filename.c_str());
  if(!file_exists(shp_filename)){
		search_config.logger.error("No file: "+filename);
    throw(1);
	}
	SHPHandle SHP = SHPOpen(convert_string(filename), "rb" );
	if(SHP != NULL ){
    	int	nEntities;
    	vector<vector<GeographicCoordinate>> relevant_polygons;
    	SHPGetInfo(SHP, &nEntities, NULL, NULL, NULL );
	    for( int i = 0; i < nEntities; i++ )
	    {
	        SHPObject	*shape;
	        shape = SHPReadObject( SHP, i );
	        if( shape == NULL ){
	            fprintf( stderr,"Unable to read shape %d, terminating object reading.\n",i);
	            break;
	        }
	        vector<GeographicCoordinate> temp_poly;
	        bool to_keep = false;
	        for(int j = 0, iPart = 1; j < shape->nVertices; j++ )
	        {
	            if( iPart < shape->nParts && shape->panPartStart[iPart] == j )
	            {
	            	if(to_keep)
	        			relevant_polygons.push_back(temp_poly);
	            	to_keep = false;
	            	temp_poly.clear();
	                iPart++;
	            }
	            GeographicCoordinate temp_point = GeographicCoordinate_init(shape->padfY[j], shape->padfX[j]);
	            to_keep = (to_keep || filter->check_within(temp_point));
	            temp_poly.push_back(temp_point);
	        }
	        if(to_keep)
	        	relevant_polygons.push_back(temp_poly);
	        SHPDestroyObject( shape );
	    }
	    search_config.logger.debug(to_string((int)relevant_polygons.size()) + " polygons imported from " + filename);
	    for(uint i = 0; i<relevant_polygons.size(); i++){
            polygon_to_raster(relevant_polygons[i], filter);
	    }
    }else{
    	throw(1);
    }
    SHPClose(SHP);
}

// geographic_polygon_area calculates the area of a geographic polygon.
double geographic_polygon_area(vector<GeographicCoordinate> polygon) {
    double area = 0;
    for (size_t i = 0; i < polygon.size() - 1; i++) {
        GeographicCoordinate g1 = polygon[i];
        GeographicCoordinate g2 = polygon[i + 1];
        area += RADIANS(g2.lon - g1.lon) * (sin(RADIANS(g1.lat)) + sin(RADIANS(g2.lat)));
    }
    area = 0.5 * area * EARTH_RADIUS_KM * EARTH_RADIUS_KM * SQ_KM_TO_HA;
    return abs(area);
}

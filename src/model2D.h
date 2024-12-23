#ifndef MODEL_H
#define MODEL_H

#include "search_config.hpp"
#include <gdal/gdal.h>
#include <gdal/gdal_priv.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

#define MODEL_UNSET 0
#define MODEL_SET_ZERO 1

// Structure to hold geospatial data
struct Geodata {
  double geotransform[6];
  char *geoprojection;

  // Function to return the projection as a string
  string projection_str(){
    return to_string(geotransform[0]) + " " + to_string(geotransform[1]) + " " +
        to_string(geotransform[3]) + " " + to_string(geotransform[5]) + " " +
        string(geoprojection);

  }
};

// Structure to hold geographic coordinates
struct GeographicCoordinate {
  double lat, lon;
};

// Structure to hold array coordinates
struct ArrayCoordinate {
  int row, col;
  GeographicCoordinate origin;
};

#include "phes_base.h"

// Template class for Model
template <class T> class Model {
public:
  // Constructor to initialize model from a file
  Model(std::string filename, GDALDataType data_type);
  // Function to write model data to a file
  void write(std::string filename, GDALDataType data_type);
  // Function to print model data
  void print();
  // Constructor to initialize model with rows and columns
  Model(int rows, int cols) {
    this->rows = rows;
    this->cols = cols;
    data = new T[rows * cols];
  }
  // Constructor to initialize model with rows, columns, and zero flag
  Model(int rows, int cols, int zero) {
    this->rows = rows;
    this->cols = cols;
    if (zero && rows * cols != 0) {
      data = new T[rows * cols]{0};
    } else {
      data = new T[rows * cols];
    }
  }
  // Destructor to delete model data
  ~Model() { delete[] data; }
  // Function to get the number of rows
  int nrows() { return rows; }
  // Function to get the number of columns
  int ncols() { return cols; }
  // Function to get the value at a specific row and column
  T get(int row, int col) { return data[row * cols + col]; }
  // Function to get a pointer to the value at a specific row and column
  T *get_pointer(int row, int col) { return &data[row * cols + col]; }
  // Function to set the value at a specific row and column
  void set(int row, int col, T value) { data[row * cols + col] = value; }
  // Function to set all values in the model to a specific value
  void set(T value) {
    for (int row = 0; row < rows; row++)
      for (int col = 0; col < cols; col++)
        data[row * cols + col] = value;
  }
  // Function to get the geodata
  Geodata get_geodata() { return geodata; }
  // Function to set the geodata
  void set_geodata(Geodata geodata) { this->geodata = geodata; }
  // Function to set the origin of the geodata
  void set_origin(double lat, double lon) {
    geodata.geotransform[0] = lon;
    geodata.geotransform[3] = lat;
  }
  // Function to check if a specific row and column are within the model bounds
  bool check_within(int row, int col) {
    return (row >= 0 && col >= 0 && row < nrows() && col < ncols());
  }
  // Function to check if a geographic coordinate is within the model bounds
  bool check_within(GeographicCoordinate &g) {
    return check_within(floor((g.lat - geodata.geotransform[3]) / geodata.geotransform[5]),
                        floor((g.lon - geodata.geotransform[0]) / geodata.geotransform[1]));
  }
  // Function to get the value at a specific geographic coordinate
  T get(GeographicCoordinate g) {
    return get(floor((g.lat - geodata.geotransform[3]) / geodata.geotransform[5]),
               floor((g.lon - geodata.geotransform[0]) / geodata.geotransform[1]));
  }
  // Function to set the value at a specific geographic coordinate
  void set(GeographicCoordinate &g, T value) {
    set(floor((g.lat - geodata.geotransform[3]) / geodata.geotransform[5]),
        floor((g.lon - geodata.geotransform[0]) / geodata.geotransform[1]), value);
  }
  // Function to get the origin of the geodata
  GeographicCoordinate get_origin() {
    GeographicCoordinate to_return = {geodata.geotransform[3], geodata.geotransform[0]};
    return to_return;
  }
  // Function to get the geographic coordinate at a specific row and column
  GeographicCoordinate get_coordinate(int row, int col) {
    GeographicCoordinate to_return = {
        geodata.geotransform[3] + ((double)row + 0.5) * geodata.geotransform[5],
        geodata.geotransform[0] + ((double)col + 0.5) * geodata.geotransform[1]};
    return to_return;
  }
  // Function to get the corners of the model
  std::vector<GeographicCoordinate> get_corners() {
    std::vector<GeographicCoordinate> to_return = {get_origin(), get_coordinate(0, ncols()),
                                              get_coordinate(nrows(), ncols()),
                                              get_coordinate(nrows(), 0)};
    return to_return;
  }

  // Function to check if one coordinate flows to another
  bool flows_to(ArrayCoordinate c1, ArrayCoordinate c2);
private:
  int rows;
  int cols;
  T *data;
  Geodata geodata;
};

// Template function to initialize model from a file
template <typename T> Model<T>::Model(std::string filename, GDALDataType data_type) {
  char *tif_filename = new char[filename.length() + 1];
  strcpy(tif_filename, filename.c_str());
  if (!file_exists(tif_filename)) {
    search_config.logger.warning("No file: " + filename);
    throw(1);
  }
  GDALDataset *Dataset = (GDALDataset *)GDALOpen(tif_filename, GA_ReadOnly);
  if (Dataset == NULL) {
    search_config.logger.error("Cannot open: " + filename);
    throw(1);
  }
  if (Dataset->GetProjectionRef() != NULL) {
    geodata.geoprojection = const_cast<char *>(Dataset->GetProjectionRef());
  } else {
    search_config.logger.error("Cannot get projection from: " + filename);
    throw(1);
  }
  if (Dataset->GetGeoTransform(geodata.geotransform) != CE_None) {
    search_config.logger.error("Cannot get transform from: " + filename);
    throw(1);
  }

  GDALRasterBand *Band = Dataset->GetRasterBand(1);
  rows = Band->GetYSize();
  cols = Band->GetXSize();
  int temp_cols = cols;
  if (cols == 1801) {
    cols = 3601;
    geodata.geotransform[1] = geodata.geotransform[1] / 2.0;
  }
  data = new T[rows * cols];
  T temp_arr[temp_cols];

  for (int row = 0; row < rows; row++) {
    CPLErr err =
        Band->RasterIO(GF_Read, 0, row, temp_cols, 1, temp_arr, temp_cols, 1, data_type, 0, 0);
    if (err != CPLE_None)
      exit(1);
    if (temp_cols == 1801) {
      for (int col = 0; col < temp_cols - 1; col++) {
        set(row, col * 2, (T)temp_arr[col]);
        set(row, col * 2 + 1, (T)temp_arr[col]);
      }
      set(row, 3600, (T)temp_arr[1800]);
    } else {
      for (int col = 0; col < temp_cols; col++)
        set(row, col, (T)temp_arr[col]);
    }
  }
}


template <typename T> void Model<T>::write(string filename, GDALDataType data_type) {
  char *tif_filename = new char[filename.length() + 1];
  strcpy(tif_filename, filename.c_str());
  const char *pszFormat = "GTiff";
  GDALDriver *Driver = GetGDALDriverManager()->GetDriverByName(pszFormat);
  if (Driver == NULL)
    exit(1);
  GDALDataset *OutDS = Driver->Create(tif_filename, cols, rows, 1, data_type, NULL);
  OutDS->SetGeoTransform(geodata.geotransform);
  OutDS->SetProjection(geodata.geoprojection);
  GDALRasterBand *Band = OutDS->GetRasterBand(1);
  T temp_arr[cols];
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      temp_arr[col] = get(row, col);
    }
    CPLErr err = Band->RasterIO(GF_Write, 0, row, cols, 1, temp_arr, cols, 1, data_type, 0, 0);
    if (err != CPLE_None)
      exit(1);
  }
  GDALClose((GDALDatasetH)OutDS);
}


template <typename T> void Model<T>::print() {
  int nx16 = cols >> 4;
  int nx32 = cols >> 5;
  int ny16 = rows >> 4;
  int ny32 = rows >> 5;
  cout << "       ";
  for (int i = 0; i < 16; i++) {
    cout << " " << std::setw(8) << nx32 + i * nx16 << " ";
  }
  cout << "\n";

  for (int j = 0; j < 16; j++) {
    int iy = ny32 + j * ny16;
    cout << std::setw(4) << iy << ":  ";
    for (int i = 0; i < 16; i++) {
      int ix = nx32 + i * nx16;
      cout << " " << std::setw(8) << +get(iy, ix) << " ";
    }
    cout << "\n";
  }
}

#endif

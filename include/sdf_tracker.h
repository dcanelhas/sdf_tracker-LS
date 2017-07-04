#ifndef SDF_TRACKER
#define SDF_TRACKER

#include <boost/thread/mutex.hpp>

#include <fstream>
#include <iostream>
#include <Eigen/Core>
#include <unsupported/Eigen/MatrixFunctions>

#include <Eigen/StdVector>
#include <time.h>

#include "CImg.h"
// #include "libsdf.h"
#include "principal_components.h"
#include "vector_functions.h"
#include "neural_network.h"

#define EIGEN_USE_NEW_STDVECTOR


typedef Eigen::Matrix<double,6,1> Vector6d;

  /// Finds the inverse projection of a 3D point to the image plane, given camera parameters
  Eigen::Vector2d To2D(const Eigen::Vector4d &location, double fx, double fy, double cx, double cy);

  /// Projects a depth map pixel into a 3D point, given camera parameters.
  Eigen::Vector4d To3D(int row, int column, double depth, double fx, double fy, double cx, double cy);

  /// Computes an antisymmetric matrix based on the pose change vector. To get a transformation matrix from this, you have to call Twist(xi).exp().
  Eigen::Matrix4d Twist(const Vector6d &xi);

  Eigen::Matrix4d GetMat4_rodrigues_smallangle(const Vector6d &xi);


struct gridCell
{
  // cell_type_t contents;
  bool active;
  thrust::device_vector<float> descriptor;
};

class SDF_Parameters
{
public:
  bool interactive_mode;
  int XSize;
  int YSize;
  int ZSize;
  int raycast_steps;
  int image_height;
  int image_width;
  double fx;
  double fy;
  double cx;
  double cy;
  double Wmax;
  double resolution;
  double Dmax;
  double Dmin;
  Eigen::Matrix4d pose_offset;
  double robust_statistic_coefficient;
  double regularization;
  double min_parameter_update;
  double min_pose_change;
  std::string render_window;

  SDF_Parameters();
  virtual ~SDF_Parameters();
};

class hyperGrid
{

public:

  principal_components PCA;
  neural_network NN;
  /// Checks the validity of the gradient of the SDF at the current point
  bool ValidGradient(const Eigen::Vector4d &location);

  hyperGrid();

  hyperGrid(uint X, uint Y, uint Z, uint x, uint y, uint z, float Wmax, float Dmax, float Dmin, float cellSize)
  : hyper_XSize_(X), hyper_YSize_(Y), hyper_ZSize_(Z), active_XSize_(x), active_YSize_(y), active_ZSize_(z), Wmax_(Wmax), Dmax_(Dmax), Dmin_(Dmin), cellSize_(cellSize)
  {
    for (int i = 0; i < 3; ++i) block_shift_[i] =0;
    this->Init();
  };

  ~hyperGrid()
  {this->Clear();};

  void shiftActiveGrid(int X, int Y, int Z);

  double SDF(const Eigen::Vector4d &location);
  double SDF_R(const Eigen::Vector4d &location); // for rendering only, creates fake geometry!

  /// Computes the gradient of the SDF at the location, along dimension dim, with central differences. stepSize chooses how far away from the central cell the samples should be taken before computing the difference
  double SDFGradient(const Eigen::Vector4d &location, int dim, int stepSize);

  /// For rendering only, may return fake gradients
  double SDFGradient_R(const Eigen::Vector4d &location, int dim, int stepSize);

  /// Saves the current volume as a VTK image.
  void SaveSDF(const std::string &filename = std::string("sdf_volume.vti"));

  /// Loads a volume from a VTK image. The grid is resized to fit the loaded volume
  void LoadSDF(const std::string &filename);

  // cell_type_t GetType(const thrust::device_vector<float> &input);

  void Init();

  void Clear();

  void FuseDepth(Eigen::Matrix4d &worldToCam, cimg_library::CImg<float> *depthImage, bool** validityMask, float fx, float fy, float cx, float cy);
  int block_shift_[3];

protected:



  int active_offset_[3];
  double Wmax_;
  double Dmax_;
  double Dmin_;
  float cellSize_;
  static const uint hyperCellSize_ = 16;
  uint hyper_XSize_, hyper_YSize_, hyper_ZSize_;
  uint active_XSize_, active_YSize_, active_ZSize_;

  float &activeVolume(int x, int y, int z);
  float &activeVolume_w(int x, int y, int z);

  float*** activeVolume_;
  gridCell*** hGrid_;


  gridCell *Floor_;
  gridCell *Walls_[4];


};

class SDFTracker
{
  protected:
  // variables

  cimg_library::CImgDisplay* display_window_;
  Eigen::Matrix4d Transformation_;
  Vector6d Pose_;
  Eigen::Vector3d translationMonitor_;
  cimg_library::CImg<float> *depthImage_;
  cimg_library::CImg<unsigned char> *preview_;
  unsigned long int frame_count_;

  boost::mutex transformation_mutex_;
  boost::mutex depth_mutex_;
  std::string camera_name_;

  hyperGrid* myGrid_;
  bool** validityMask_;
  bool first_frame_;
  bool quit_;
  SDF_Parameters parameters_;
  // functions
  virtual void Init(SDF_Parameters &parameters);
  virtual void DeleteGrids(void);

  public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  std::vector<Eigen::Vector4d> triangles_;

  /// Sets the current depth map
  virtual void UpdateDepth(const cimg_library::CImg<float> &depth);

  /// Estimates the incremental pose change vector from the current pose, relative to the current depth map
  virtual Vector6d EstimatePoseFromDepth(void);

  // /// Fuses the current depth map into the TSDF volume, the current depth map is set using UpdateDepth
  virtual void FuseDepth(void);

  // /// Fuses the current depth map into the TSDF volume, the current depth map is set using UpdateDepth
  virtual void checkTranslation(int tolerance);

  /// Render a virtual depth map. If interactive mode is true (see class SDF_Parameters) it will also display a preview window with estimated normal vectors
  virtual void Render(void);

    /// Saves the current volume as a VTK image.
  virtual void SaveSDF(const std::string &filename = std::string("sdf_volume.vti")){myGrid_->SaveSDF(filename);};

  /// gets the current transformation matrix
  Eigen::Matrix4d GetCurrentTransformation(void);

  /// sets the current transformation to the given matrix
  void SetCurrentTransformation(const Eigen::Matrix4d &T);

  /// In interactive sessions, this function returns true at any point after a user has pressed "q" or <ESC> in the render window.
  bool Quit(void);

  /// Constructor with default parameters
  SDFTracker();

  /// Constructor with custom parameters
  SDFTracker(SDF_Parameters &parameters);
  virtual ~SDFTracker();

};

#endif

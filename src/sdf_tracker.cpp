#include <cmath>
#include <iostream>
#include <limits>
#include <cstddef>

#include <Eigen/Core>
#include <Eigen/StdVector>
#include <Eigen/Geometry>
#include <unsupported/Eigen/MatrixFunctions>

// #include <vtkImageData.h>
// #include <vtkFloatArray.h>
// #include <vtkXMLImageDataWriter.h>
// #include <vtkXMLImageDataReader.h>
// #include <vtkPointData.h>
// #include <vtkSmartPointer.h>


#include <time.h>
#include "sdf_tracker.h"
#include "CImg.h"
#include "config.h"

int mod (int a, int b)
{
   if(b < 0) //you can check for b == 0 separately and do what you want
     return mod(-a, -b);
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}

SDF_Parameters::SDF_Parameters()
{
  image_width = 640;
  image_height = 480;
  interactive_mode = true;
  Wmax = 64.0;
  resolution = 0.01;
  XSize = 256;
  YSize = 256;
  ZSize = 256;
  Dmax = 0.1;
  Dmin = -0.04;
  pose_offset = Eigen::MatrixXd::Identity(4,4);
  robust_statistic_coefficient = 0.04;
  regularization = 0.01;
  min_pose_change = 0.01;
  min_parameter_update = 0.0001;
  raycast_steps = 12;
  fx = 520.0;
  fy = 520.0;
  cx = 319.5;
  cy = 239.5;
  render_window = "Render";
}

SDF_Parameters::~SDF_Parameters()
{}

SDFTracker::SDFTracker() {
    SDF_Parameters myparams = SDF_Parameters();
    this->Init(myparams);
}

SDFTracker::SDFTracker(SDF_Parameters &parameters)
{
  this->Init(parameters);
}

SDFTracker::~SDFTracker()
{

  this->DeleteGrids();

  for (int i = 0; i < parameters_.image_height; ++i)
  {
    if ( validityMask_[i]!=NULL)
    delete[] validityMask_[i];
  }
  delete[] validityMask_;

  if(depthImage_!=NULL)
  delete depthImage_;

};

void SDFTracker::checkTranslation(int tolerance)
{


  Eigen::Matrix4d T = GetCurrentTransformation();

  int shift[3] = {int(  translationMonitor_(0)/(parameters_.resolution*tolerance*16.0)),
                  int(  translationMonitor_(1)/(parameters_.resolution*tolerance*16.0)),
                  int(  translationMonitor_(2)/(parameters_.resolution*tolerance*16.0))};


  if(shift[0] || shift[1] || shift[2]){

    myGrid_->shiftActiveGrid(shift[0],shift[1],shift[2]);

    T(0,3) -= shift[0]*16*parameters_.resolution;
    T(1,3) -= shift[1]*16*parameters_.resolution;
    T(2,3) -= shift[2]*16*parameters_.resolution;
    translationMonitor_(0) -= shift[0]*16*parameters_.resolution;
    translationMonitor_(1) -= shift[1]*16*parameters_.resolution;
    translationMonitor_(2) -= shift[2]*16*parameters_.resolution;
    SetCurrentTransformation(T);

  }

}

void SDFTracker::Init(SDF_Parameters &parameters)
{
  parameters_ = parameters;
  frame_count_ = 0;
  int downsample=1;
  switch(parameters_.image_height)
  {
  case 480: downsample = 1; break; //VGA
  case 240: downsample = 2; break; //QVGA
  case 120: downsample = 4; break; //QQVGA
  }
  parameters_.fx /= downsample;
  parameters_.fy /= downsample;
  parameters_.cx /= downsample;
  parameters_.cy /= downsample;

  depthImage_ = new cimg_library::CImg<float>(parameters_.image_width,parameters_.image_height,1,1);

  validityMask_ = new bool*[parameters_.image_height];
  for (int i = 0; i < parameters_.image_height; ++i)
  {
    validityMask_[i] = new bool[parameters_.image_width];
    memset(validityMask_[i],0,parameters_.image_width);
  }

  quit_ = false;
  first_frame_ = true;
  Pose_ << 0.0,0.0,0.0,0.0,0.0,0.0;
  translationMonitor_ << 0.0,0.0,0.0;
  Transformation_=parameters_.pose_offset*Eigen::MatrixXd::Identity(4,4);

  if(parameters_.interactive_mode)
  {

    preview_ = new cimg_library::CImg<unsigned char>(parameters_.image_width,parameters_.image_height,1,3);
    display_window_ = new cimg_library::CImgDisplay(*preview_);
  
  //  cv::namedWindow( parameters_.render_window, 0 );
  }

   myGrid_ = new hyperGrid( parameters_.XSize/2,  parameters_.YSize/2,  parameters_.ZSize/2, parameters_.XSize, parameters_.YSize, parameters_.ZSize,
                          parameters_.Wmax, parameters_.Dmax, parameters_.Dmin, parameters_.resolution);

};

void SDFTracker::DeleteGrids(void)
{
  myGrid_->Clear();
}

Eigen::Matrix4d
Twist(const Vector6d &xi)
{
  Eigen::Matrix4d M;

  M << 0.0  , -xi(2),  xi(1), xi(3),
       xi(2), 0.0   , -xi(0), xi(4),
      -xi(1), xi(0) , 0.0   , xi(5),
       0.0,   0.0   , 0.0   , 0.0  ;

  return M;
};

Eigen::Matrix4d GetMat4_rodrigues_smallangle(const Vector6d &xi)
{
  double nanguard = 1e-6;
  Eigen::Vector3d Euler = Eigen::Vector3d(xi(0),xi(1),xi(2));
  const double theta = Euler.norm();
  const Eigen::Vector3d Euler_n = Euler/theta;
  Eigen::Matrix3d S_n, R;
  Eigen::Matrix4d T;

  S_n << 	0,				-Euler_n(2),	Euler_n(1),
          Euler_n(2),		0,				-Euler_n(0),
          -Euler_n(1),	Euler_n(0),		0;


  //sin(theta) = theta ,
  //cos(theta) = (1 - theta*theta*0.5)

  if(theta > nanguard)
  {
      R = Eigen::MatrixXd::Identity(3,3) * (1 - theta*theta*0.5) +
      (1.0-(1 - theta*theta*0.5))*Euler_n*Euler_n.transpose()+
      S_n*theta;
  }
  else
  {
      R = Eigen::MatrixXd::Identity(3,3);
  }

  T << 	R(0,0), R(0,1), R(0,2), xi(3),
        R(1,0), R(1,1), R(1,2), xi(4),
        R(2,0), R(2,1), R(2,2), xi(5),
        0,		0,		0,		1.0;

  return T;
};


Eigen::Vector4d
To3D(int row, int column, double depth, double fx, double fy, double cx, double cy)
{

  Eigen::Vector4d ret(double(column-cx)*depth/(fx),
                      double(row-cy)*depth/(fy),
                      double(depth),
                      1.0f);
  return ret;
};

Eigen::Vector2d
To2D(const Eigen::Vector4d &location, double fx, double fy, double cx, double cy)
{
  Eigen::Vector2d pixel(0,0);
  if(location(2) != 0)
  {
     pixel(0) = (cx) + location(0)/location(2)*(fx);
     pixel(1) = (cy) + location(1)/location(2)*(fy);
  }

  return pixel;
};

bool
hyperGrid::ValidGradient(const Eigen::Vector4d &location)
{
 /*
 The function tests the current location and its adjacent
 voxels for valid values (not truncated) to
 determine if derivatives at this location are
 computable in all three directions.

 Since the function SDF(Eigen::Vector4d &location) is a
 trilinear interpolation between neighbours, testing the
 validity of the gradient involves looking at all the
 values that would contribute to the final  gradient.
 If any of these  are truncated, the result is false.
                      X--------X
                    /        / |
                  X--------X   ----X
                  |        |   | / |
              X----        |   X-------X
            /     |        | /       / |
          X-------X--------X-------X   |
          |     /        / |       |   |
          |   X--------X   |       |   X
     J    |   |        |   |       | /
     ^    X---|        |   X-------X
     |        |        | / |  |
      --->I   X--------X   |  X
    /             |        | /
   v              X--------X
  K                                                */

  float eps = 10e-9;

  double i,j,k;
  modf(location(0)/cellSize_ + active_XSize_/2, &i);
  modf(location(1)/cellSize_ + active_YSize_/2, &j);
  modf(location(2)/cellSize_ + active_ZSize_/2, &k);

  if(std::isnan(i) || std::isnan(j) || std::isnan(k)) return false;

  int I = int(i)-1; int J = int(j)-1;   int K = int(k)-1;

  if(I>=active_XSize_-4 || J>=active_YSize_-3 || K>=active_ZSize_-3 || I<=1 || J<=1 || K<=1)return false;

  // 2* because weights and distances are packed into the same array
  float D10 = activeVolume(I+1, J+0, 2*(K+1) );   float D10_1 = activeVolume(I+1, J+0, 2*(K+2) );
  float D20 = activeVolume(I+2, J+0, 2*(K+1) );   float D20_1 = activeVolume(I+2, J+0, 2*(K+2) );

  float D01 = activeVolume(I+0, J+1, 2*(K+1) );   float D01_1 = activeVolume(I+0, J+1, 2*(K+2) );
  float D11 = activeVolume(I+1, J+1, 2*(K+0) );   float D11_1 = activeVolume(I+1, J+1, 2*(K+1) ); float D11_2 = activeVolume(I+1, J+1, 2*(K+2) ); float D11_3 = activeVolume(I+1, J+1, 2*(K+3) );
  float D21 = activeVolume(I+2, J+1, 2*(K+0) );   float D21_1 = activeVolume(I+2, J+1, 2*(K+1) ); float D21_2 = activeVolume(I+2, J+1, 2*(K+2) ); float D21_3 = activeVolume(I+2, J+1, 2*(K+3) );
  float D31 = activeVolume(I+3, J+1, 2*(K+1) );   float D31_1 = activeVolume(I+3, J+1, 2*(K+2) );

  float D02 = activeVolume(I+0, J+2, 2*(K+1) );   float D02_1 = activeVolume(I+0, J+2, 2*(K+2) );
  float D12 = activeVolume(I+1, J+2, 2*(K+0) );   float D12_1 = activeVolume(I+1, J+2, 2*(K+1) ); float D12_2 = activeVolume(I+1, J+2, 2*(K+2) ); float D12_3 = activeVolume(I+1, J+2, 2*(K+3) );
  float D22 = activeVolume(I+2, J+2, 2*(K+0) );   float D22_1 = activeVolume(I+2, J+2, 2*(K+1) ); float D22_2 = activeVolume(I+2, J+2, 2*(K+2) ); float D22_3 = activeVolume(I+2, J+2, 2*(K+3) );
  float D32 = activeVolume(I+3, J+2, 2*(K+1) );   float D32_1 = activeVolume(I+3, J+2, 2*(K+3) );

  float D13 = activeVolume(I+1, J+3, 2*(K+1) );   float D13_1 = activeVolume(I+1, J+3, 2*(K+2) );
  float D23 = activeVolume(I+2, J+3, 2*(K+1) );   float D23_1 = activeVolume(I+2, J+3, 2*(K+2) );

  if( D10 > Dmax_-eps || D10_1 > Dmax_-eps ||
      D20 > Dmax_-eps || D20_1 > Dmax_-eps ||

      D01 > Dmax_-eps || D01_1 > Dmax_-eps ||
      D11 > Dmax_-eps || D11_1 > Dmax_-eps || D11_2 > Dmax_-eps || D11_3 > Dmax_-eps ||
      D21 > Dmax_-eps || D21_1 > Dmax_-eps || D21_2 > Dmax_-eps || D21_3 > Dmax_-eps ||
      D31 > Dmax_-eps || D31_1 > Dmax_-eps ||

      D02 > Dmax_-eps || D02_1 > Dmax_-eps ||
      D12 > Dmax_-eps || D12_1 > Dmax_-eps || D12_2 > Dmax_-eps || D12_3 > Dmax_-eps ||
      D22 > Dmax_-eps || D22_1 > Dmax_-eps || D22_2 > Dmax_-eps || D22_3 > Dmax_-eps ||
      D32 > Dmax_-eps || D32_1 > Dmax_-eps ||

      D13 > Dmax_-eps || D13_1 > Dmax_-eps ||
      D23 > Dmax_-eps || D23_1 > Dmax_-eps
      ) return false;
  else return true;
}


double
hyperGrid::SDFGradient(const Eigen::Vector4d &location, int stepSize, int dim )
{
  double delta=cellSize_*stepSize;
  Eigen::Vector4d location_offset = Eigen::Vector4d(0,0,0,1);
  location_offset(dim) = delta;

  return ((SDF(location+location_offset)) - (SDF(location-location_offset)))/(2.0*delta);
};


void
SDFTracker::UpdateDepth(const cimg_library::CImg<float> &depth)
{
  // depth_mutex_.lock();
  *depthImage_= depth;
  // depth_mutex_.unlock();

  #pragma omp parallel for collapse(2) schedule(static)
  for(int row=0; row<depthImage_->height()-0; ++row)
  for(int col=0; col<depthImage_->width()-0; ++col)
      validityMask_[row][col] = !std::isnan(depth(col,row));
}

void
SDFTracker::FuseDepth(void)
{
  Eigen::Matrix4d wtc = Transformation_.inverse();
  myGrid_->FuseDepth(wtc, depthImage_, validityMask_, parameters_.fx, parameters_.fy, parameters_.cx, parameters_.cy);
}


Vector6d
SDFTracker::EstimatePoseFromDepth(void)
{
  Vector6d xi;
  xi<<0.0,0.0,0.0,0.0,0.0,0.0; // + (Pose-previousPose)*0.1;
  Vector6d xi_prev = xi;
  const float eps = 10e-9;
  const float c = parameters_.robust_statistic_coefficient*parameters_.Dmax;

  const int iterations[3]={12, 8, 2};
  const int stepSize[3] = {4, 2, 1};

  for(int lvl=0; lvl < 3; ++lvl)
  {
    for(int k=0; k<iterations[lvl]; ++k)
    {

      const Eigen::Matrix4d camToWorld = GetMat4_rodrigues_smallangle(xi)*Transformation_;

      float A00=0.0;//,A01=0.0,A02=0.0,A03=0.0,A04=0.0,A05=0.0;
      float A10=0.0,A11=0.0;//,A12=0.0,A13=0.0,A14=0.0,A15=0.0;
      float A20=0.0,A21=0.0,A22=0.0;//,A23=0.0,A24=0.0,A25=0.0;
      float A30=0.0,A31=0.0,A32=0.0,A33=0.0;//,A34=0.0,A35=0.0;
      float A40=0.0,A41=0.0,A42=0.0,A43=0.0,A44=0.0;//,A45=0.0;
      float A50=0.0,A51=0.0,A52=0.0,A53=0.0,A54=0.0,A55=0.0;

      float g0=0.0, g1=0.0, g2=0.0, g3=0.0, g4=0.0, g5=0.0;

      #pragma omp parallel for schedule(static) \
      default(shared) collapse(2) \
      reduction(+:g0,g1,g2,g3,g4,g5,A00,A10,A11,A20,A21,A22,A30,A31,A32,A33,A40,A41,A42,A43,A44,A50,A51,A52,A53,A54,A55)
      for(int row=0; row<depthImage_->height()-0; row+=stepSize[lvl])
      {
        for(int col=0; col<depthImage_->width()-0; col+=stepSize[lvl])
        {
          if(!validityMask_[row][col]) continue;
          float depth = (*depthImage_)(col,row);
          Eigen::Vector4d currentPoint = camToWorld*To3D(row,col,depth,parameters_.fx,parameters_.fy,parameters_.cx,parameters_.cy);

          if(!myGrid_->ValidGradient(currentPoint)) continue;
          float D = (myGrid_->SDF(currentPoint));
          float Dabs = fabsf(D);
          if(D > parameters_.Dmax - eps || D < parameters_.Dmin + eps) continue;

          //partial derivative of SDF wrt position
          Eigen::Matrix<double,1,3> dSDF_dx(myGrid_->SDFGradient(currentPoint,1,0),
                                            myGrid_->SDFGradient(currentPoint,1,1),
                                            myGrid_->SDFGradient(currentPoint,1,2)
                                            );
          //partial derivative of position wrt optimizaiton parameters
          Eigen::Matrix<double,3,6> dx_dxi;
          dx_dxi << 0, currentPoint(2), -currentPoint(1), 1, 0, 0,
                    -currentPoint(2), 0, currentPoint(0), 0, 1, 0,
                    currentPoint(1), -currentPoint(0), 0, 0, 0, 1;

          //jacobian = derivative of SDF wrt xi (chain rule)
          Eigen::Matrix<double,1,6> J = dSDF_dx*dx_dxi;

          //double tukey = (1-(Dabs/c)*(Dabs/c))*(1-(Dabs/c)*(Dabs/c));
          double huber = Dabs < c ? 1.0 : c/Dabs;

          //Gauss - Newton approximation to hessian
          Eigen::Matrix<double,6,6> T1 = huber * J.transpose() * J;
          Eigen::Matrix<double,1,6> T2 = huber * J.transpose() * D;

          g0 = g0 + T2(0); g1 = g1 + T2(1); g2 = g2 + T2(2);
          g3 = g3 + T2(3); g4 = g4 + T2(4); g5 = g5 + T2(5);

          A00+=T1(0,0);//A01+=T1(0,1);A02+=T1(0,2);A03+=T1(0,3);A04+=T1(0,4);A05+=T1(0,5);
          A10+=T1(1,0);A11+=T1(1,1);//A12+=T1(1,2);A13+=T1(1,3);A14+=T1(1,4);A15+=T1(1,5);
          A20+=T1(2,0);A21+=T1(2,1);A22+=T1(2,2);//A23+=T1(2,3);A24+=T1(2,4);A25+=T1(2,5);
          A30+=T1(3,0);A31+=T1(3,1);A32+=T1(3,2);A33+=T1(3,3);//A34+=T1(3,4);A35+=T1(3,5);
          A40+=T1(4,0);A41+=T1(4,1);A42+=T1(4,2);A43+=T1(4,3);A44+=T1(4,4);//A45+=T1(4,5);
          A50+=T1(5,0);A51+=T1(5,1);A52+=T1(5,2);A53+=T1(5,3);A54+=T1(5,4);A55+=T1(5,5);

        }//col
      }//row

      Eigen::Matrix<double,6,6> A;

      A<< A00,A10,A20,A30,A40,A50,
          A10,A11,A21,A31,A41,A51,
          A20,A21,A22,A32,A42,A52,
          A30,A31,A32,A33,A43,A53,
          A40,A41,A42,A43,A44,A54,
          A50,A51,A52,A53,A54,A55;
     double scaling = 1/A.maxCoeff();

      Vector6d g;
      g<< g0, g1, g2, g3, g4, g5;

      g *= scaling;
      A *= scaling;

      A = A + (parameters_.regularization)*Eigen::MatrixXd::Identity(6,6);
      xi = xi - A.ldlt().solve(g);
      Vector6d Change = xi-xi_prev;
      double Cnorm = Change.norm();
      xi_prev = xi;
      if(Cnorm < parameters_.min_parameter_update) break;
    }//k
  }//level
  if(std::isnan(xi.sum())) xi << 0.0,0.0,0.0,0.0,0.0,0.0;
  return xi;
};//function


//check rendering function for u v transposition errors.
void
SDFTracker::Render(void)
{
  //double minStep = parameters_.resolution/4;
  //cimg_library::CImg<unsigned char> preview(parameters_.image_height,parameters_.image_width,3);

  const Eigen::Matrix4d camToWorld = Transformation_;
  const Eigen::Vector4d camera = camToWorld * Eigen::Vector4d(0.0,0.0,0.0,1.0);
  const double max_ray_length = 5.0;

  // Rendering loop
 #pragma omp parallel for schedule(static) collapse(2)
  for(int u = 0; u < parameters_.image_height; ++u)
  {
    for(int v = 0; v < parameters_.image_width; ++v)
    {
      bool hit = false;

      Eigen::Vector4d p = camToWorld*To3D(u,v,1.0,parameters_.fx,parameters_.fy,parameters_.cx,parameters_.cy) - camera;
      p.normalize();

      double scaling = validityMask_[u][v] ? double((*depthImage_)(v,u))*0.8 : parameters_.Dmax;

      double scaling_prev=0;
      int steps=0;
      double D = parameters_.resolution;
      while(steps<parameters_.raycast_steps && scaling < max_ray_length && !hit)
      {

        double D_prev = D;
        D = myGrid_->SDF(camera + p*scaling);

        if(D < 0.0)
        {
          scaling = scaling_prev + (scaling-scaling_prev)*D_prev/(D_prev - D);

          hit = true;
          Eigen::Vector4d normal_vector = Eigen::Vector4d::Zero();

          if(parameters_.interactive_mode)
          {
    //        for(int ii=0; ii<3; ++ii)
    //        {
            float light = std::min(0.99, std::max(-0.99,myGrid_->SDFGradient(camera + p*scaling,1,0)));
    //          normal_vector(ii) = myGrid_->SDFGradient(camera + p*scaling,1,ii);
    //        }
            normal_vector.normalize();

            (*preview_)(v,u,0,0) = 128 - rint(light*127);
            (*preview_)(v,u,0,1) = 128 - rint(light*127);
            (*preview_)(v,u,0,2) = 128 - rint(light*127);
          }

          break;
        }
        scaling_prev = scaling;
        scaling += std::max(parameters_.resolution,D);
        ++steps;
      }//ray
      if(!hit)
      {
        if(parameters_.interactive_mode)
        {
          (*preview_)(v,u,0,0) = (unsigned char) 60;
          (*preview_)(v,u,0,1) = (unsigned char) 30;
          (*preview_)(v,u,0,2) = (unsigned char) 30;
        }
      }//no hit
    }//col
  }//row

  if(parameters_.interactive_mode)
  {
    // std::ostringstream ss;
    // ss << std::setw(5) << std::setfill('0') << frame_count_;
    // std::string imfile("render_"+ss.str()+".png");
    // ++frame_count_;
    // cv::imwrite(imfile, preview);//depthImage_denoised);
    
    (*preview_).display(*display_window_);
    // cv::imshow(parameters_.render_window, preview);//depthImage_denoised);
    // char q = cv::waitKey(1);
    if(display_window_->is_closed()) { quit_ = true; }//int(key)
  }
  return;
};

bool SDFTracker::Quit(void)
{
  return quit_;
}

Eigen::Matrix4d
SDFTracker::GetCurrentTransformation(void)
{
  Eigen::Matrix4d T;
  transformation_mutex_.lock();
  T = Transformation_;
  transformation_mutex_.unlock();
  return T;
}

void
SDFTracker::SetCurrentTransformation(const Eigen::Matrix4d &T)
{

  transformation_mutex_.lock();
  Transformation_= T;
  transformation_mutex_.unlock();
  translationMonitor_ += Eigen::Vector3d(T(0,3),T(1,3),T(2,3)) - Eigen::Vector3d(translationMonitor_(0),translationMonitor_(1),translationMonitor_(2));
}

void hyperGrid::SaveSDF(const std::string &filename)
{

// http://www.vtk.org/Wiki/VTK/Examples/Cxx/IO/WriteVTI

  //vtkImageData *sdf_volume = vtkImageData::New();

  // vtkSmartPointer<vtkImageData> sdf_volume = vtkSmartPointer<vtkImageData>::New();


  // //these are the initial bounds, which are the 'opposite' of the expected final values
  // int bounds_inactive[6]={hyper_XSize_,
  //                         hyper_YSize_,
  //                         hyper_ZSize_,
  //                         0, 0, 0};
  // //these are the correct active bounds.
  // int bounds_active[6]={  active_offset_[0]+block_shift_[0],
  //                         active_offset_[1]+block_shift_[1],
  //                         active_offset_[2]+block_shift_[2],
  //                         active_offset_[0]+block_shift_[0] + active_XSize_/16,
  //                         active_offset_[1]+block_shift_[1] + active_YSize_/16,
  //                         active_offset_[2]+block_shift_[2] + active_ZSize_/16
  //                         };

  // for(int k=0; k<hyper_ZSize_; ++k)
  // for(int j=0; j<hyper_YSize_; ++j)
  // for(int i=0; i<hyper_XSize_; ++i)
  // {
  //   if (hGrid_[i][j][k].descriptor.size() > 0)
  //   {
  //     if(i<bounds_inactive[0]) bounds_inactive[0]=i;
  //     if(j<bounds_inactive[1]) bounds_inactive[1]=j;
  //     if(k<bounds_inactive[2]) bounds_inactive[2]=k;
  //     if(i>bounds_inactive[3]) bounds_inactive[3]=i;
  //     if(j>bounds_inactive[4]) bounds_inactive[4]=j;
  //     if(k>bounds_inactive[5]) bounds_inactive[5]=k;
  //   }
  // }

  // //if no dynamically adjusted bounds were possible, wrap tightly around active
  // if(bounds_inactive[0] == hyper_XSize_) bounds_inactive[0]=active_offset_[0];
  // if(bounds_inactive[1] == hyper_YSize_) bounds_inactive[1]=active_offset_[1];
  // if(bounds_inactive[2] == hyper_ZSize_) bounds_inactive[2]=active_offset_[2];
  // if(bounds_inactive[3] == 0) bounds_inactive[3] = active_offset_[0]+active_XSize_/16;
  // if(bounds_inactive[4] == 0) bounds_inactive[4] = active_offset_[1]+active_YSize_/16;
  // if(bounds_inactive[5] == 0) bounds_inactive[5] = active_offset_[2]+active_ZSize_/16;

  // std::cout << "The affected part of the grid appears to be bounded by ["<<
  //             bounds_inactive[0] <<", "<<
  //             bounds_inactive[1] <<", "<<
  //             bounds_inactive[2]
  //             <<"] and ["<<
  //             bounds_inactive[3] <<", "<<
  //             bounds_inactive[4] <<", "<<
  //             bounds_inactive[5] <<"]" << std::endl;

  // int rangeX = bounds_inactive[3]-bounds_inactive[0];
  // int rangeY = bounds_inactive[4]-bounds_inactive[1];
  // int rangeZ = bounds_inactive[5]-bounds_inactive[2];

  // sdf_volume->SetDimensions(rangeX*hyperCellSize_, rangeY*hyperCellSize_, rangeZ*hyperCellSize_);
  // sdf_volume->SetOrigin(  cellSize_*rangeX*hyperCellSize_/2.0,
  //                         cellSize_*rangeY*hyperCellSize_/2.0,
  //                         cellSize_*rangeZ*hyperCellSize_/2.0);

  // float spc = cellSize_;
  // sdf_volume->SetSpacing(spc,spc,spc);

  // vtkSmartPointer<vtkFloatArray> distance = vtkSmartPointer<vtkFloatArray>::New();
  // vtkSmartPointer<vtkFloatArray> weight = vtkSmartPointer<vtkFloatArray>::New();
  // // vtkSmartPointer<vtkFloatArray> color = vtkSmartPointer<vtkFloatArray>::New();

  // long int numCells = rangeX*hyperCellSize_ * rangeY*hyperCellSize_ * rangeZ*hyperCellSize_;

  // distance->SetNumberOfTuples(numCells);
  // weight->SetNumberOfTuples(numCells);
  // // color->SetNumberOfComponents(3);
  // // color->SetNumberOfTuples(numCells);

  // long int i, j, k, offset_k, offset_j;
  // for(k=bounds_inactive[2]; k<bounds_inactive[5]; ++k)
  // for(j=bounds_inactive[1]; j<bounds_inactive[4]; ++j)
  // for(i=bounds_inactive[0]; i<bounds_inactive[3]; ++i)
  // {
  //   thrust::host_vector<float> host_voxels;
  //   thrust::device_vector<float> device_voxels;
  //   //i j and k point to absolute locations in the original grid
  //   //offsets are relative to first non-emptry hypercell
  //   bool has_code = (hGrid_[i][j][k].descriptor.size() > 0);

  //   bool is_active =(i>=bounds_active[0] && i < bounds_active[3] &&
  //                    j>=bounds_active[1] && j < bounds_active[4] &&
  //                    k>=bounds_active[2] && k < bounds_active[5]);

  //   bool decode = has_code && !is_active;
  //   bool untouched = !has_code && !is_active;
  //   thrust::host_vector<float> descriptor_host;

  //   // cell_type_t contents = OTHER;

  //   if(decode)
  //   {
  //     PCA.decode(hGrid_[i][j][k].descriptor,device_voxels);
  //     host_voxels = device_voxels;
  //     // contents = hGrid_[i][j][k].contents;
  //     descriptor_host = hGrid_[i][j][k].descriptor;
  //   }

  //   int idx = 0;
  //   for (int kk = 0; kk < hyperCellSize_; ++kk)
  //   {
  //     offset_k = ((k-bounds_inactive[2])*hyperCellSize_ + kk)*rangeX*rangeY*hyperCellSize_*hyperCellSize_;
  //     for (int jj = 0; jj < hyperCellSize_; ++jj)
  //     {
  //       offset_j = ((j-bounds_inactive[1])*hyperCellSize_ + jj)*rangeX*hyperCellSize_;
  //       for (int ii = 0; ii < hyperCellSize_; ++ii)
  //       {
  //         long int offset = (i-bounds_inactive[0])*hyperCellSize_ +ii + offset_j + offset_k;
  //         float w = decode  ? 0 :
  //                   untouched ? -1.0f :
  //                   is_active ? -2.0f : 99.0f;

  //         if(decode)
  //           distance->SetValue(offset, host_voxels[idx]*(Dmax_- Dmin_) + Dmin_);
  //         else if(untouched)
  //           distance->SetValue(offset, Dmax_);
  //         else
  //         {
  //           distance->SetValue(offset, activeVolume((i-active_offset_[0]-block_shift_[0])*hyperCellSize_ + ii ,
  //                                                   (j-active_offset_[1]-block_shift_[1])*hyperCellSize_ + jj ,
  //                                                2*((k-active_offset_[2]-block_shift_[2])*hyperCellSize_ + kk )) );
  //           // weight->SetValue(offset, 13.0f);//activeVolume_w(i*hyperCellSize_ + ii ,j*hyperCellSize_ + jj ,2*(k*hyperCellSize_ + kk)));
  //         }

  //       //   float c[3];
  //       //   if(decode)
  //       //   {
  //           //   for(int c_id=0; c_id<3; ++c_id)
  //               // c[c_id]=descriptor_host[c_id];
  //       //   }
  //       //   else
  //       //   {
  //           //   for(int c_id=0; c_id<3; ++c_id)
  //               // c[c_id]=0;
  //       //   }


  //         weight->SetValue(offset, w);
  //       //   color->SetComponent(offset,0, c[0]);
  //       //   color->SetComponent(offset,1, c[1]);
  //       //   color->SetComponent(offset,2, c[2]);

  //         ++idx;
  //       }
  //     }
  //   }



  // }

  // // sdf_volume->GetPointData()->AddArray(color);
  // // color->SetName("Color");

  // sdf_volume->GetPointData()->AddArray(distance);
  // distance->SetName("Distance");

  // sdf_volume->GetPointData()->AddArray(weight);
  // weight->SetName("Weight");

  // vtkSmartPointer<vtkXMLImageDataWriter> writer =
  // vtkSmartPointer<vtkXMLImageDataWriter>::New();
  // writer->SetFileName(filename.c_str());

  // writer->SetInputData(sdf_volume);
  // writer->Write();
}

void hyperGrid::LoadSDF(const std::string &filename)
{

  // // //double valuerange[2];
  // vtkXMLImageDataReader *reader  = vtkXMLImageDataReader::New();
  // reader->SetFileName(filename.c_str());
  // // //reader->GetOutput()->GetScalarRange(valuerange);
  // reader->Update();
  // reader->UpdateWholeExtent();
  // reader->UpdateInformation();

  // vtkSmartPointer<vtkImageData> sdf_volume = vtkSmartPointer<vtkImageData>::New();
  // sdf_volume = reader->GetOutput();
  // this->Clear();

  // //will segfault if not specified
  // int* sizes = sdf_volume->GetDimensions();
  // active_XSize_ = sizes[0];
  // active_YSize_ = sizes[1];
  // active_ZSize_ = sizes[2];

  // double* cell_sizes = sdf_volume->GetSpacing();
  // cellSize_ = float(cell_sizes[0]);  //TODO add support for different scalings along x,y,z.

  // this->Init();

  // vtkFloatArray *distance =vtkFloatArray::New();
  // vtkFloatArray *weight =vtkFloatArray::New();

  // distance = (vtkFloatArray *)reader->GetOutput()->GetPointData()->GetScalars("Distance");
  // weight = (vtkFloatArray *)reader->GetOutput()->GetPointData()->GetScalars("Weight");

  // int i, j, k, offset_k, offset_j;
  // for(k=0;k < active_ZSize_; ++k)
  // {
  //   offset_k = k*active_XSize_*active_YSize_;
  //   for(j=0; j<active_YSize_; ++j)
  //   {
  //     offset_j = j*active_XSize_;
  //     for(i=0; i<active_XSize_; ++i)
  //     {
  //       int offset = i + offset_j + offset_k;
  //       activeVolume_[i][j][k*2] = distance->GetValue(offset);
  //       activeVolume_[i][j][k*2+1] = weight->GetValue(offset);
  //     }
  //   }
  // }
}


void hyperGrid::shiftActiveGrid(int X, int Y, int Z)
{


  float decoded_w = 1.0f;
  float empty_tolerance = 1e-9f;
  if (X>0)
  {

    for (int k = 0; k < active_ZSize_/16 ; ++k)
    for (int j = 0; j < active_YSize_/16 ; ++j)
    for (int i = 0; i < 1; ++i)
    {
      thrust::host_vector<float> host_voxels_encode;
      thrust::host_vector<float> host_voxels_decode;
      thrust::device_vector<float> dev_voxels;
      int DEC_X = active_XSize_/16 + active_offset_[0] + block_shift_[0];
      int DEC_Y = active_offset_[1] + j+block_shift_[1];
      int DEC_Z = active_offset_[2] + k+block_shift_[2];

      int ENC_X = active_offset_[0] + i+block_shift_[0];
      int ENC_Y = active_offset_[1] + j+block_shift_[1];
      int ENC_Z = active_offset_[2] + k+block_shift_[2];

      //check if this block should be decoded
      bool decode = (hGrid_ [DEC_X][DEC_Y][DEC_Z].descriptor.size() > 0);

      //then decode it
      if(decode)
      {
          PCA.decode(hGrid_[DEC_X][DEC_Y][DEC_Z].descriptor, dev_voxels);
          host_voxels_decode = dev_voxels;
      }

      //go through the block
      int idx = 0;
      for (int kk = 0; kk < 16; ++kk)
      for (int jj = 0; jj < 16; ++jj)
      for (int ii = 0; ii < 16; ++ii)
      {
        host_voxels_encode.push_back( (activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) - Dmin_)/(Dmax_-Dmin_) );

        activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ?  host_voxels_decode[idx]*(Dmax_- Dmin_) + Dmin_ : (Dmax_);
        activeVolume_w(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ? decoded_w : 0.0f;
        ++idx;
      }
      dev_voxels = host_voxels_encode;

      PCA.encode( dev_voxels, hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
      // hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].contents = GetType(hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
    }
  }
  else if (X<0)
  {
    for (int k = 0; k < active_ZSize_/16 ; ++k)
    for (int j = 0; j < active_YSize_/16 ; ++j)
    for (int i = active_XSize_/16-1; i < active_XSize_/16 ; ++i)
    {
      thrust::host_vector<float> host_voxels_encode;
      thrust::host_vector<float> host_voxels_decode;
      thrust::device_vector<float> dev_voxels;
      int DEC_X = active_offset_[0] - 1+block_shift_[0];
      int DEC_Y = active_offset_[1] + j+block_shift_[1];
      int DEC_Z = active_offset_[2] + k+block_shift_[2];

      int ENC_X = active_XSize_/16 + active_offset_[0] - 1+block_shift_[0];
      int ENC_Y =                    active_offset_[1] + j+block_shift_[1];
      int ENC_Z =                    active_offset_[2] + k+block_shift_[2];

      //check if this block should be decoded
      bool decode = (hGrid_ [DEC_X][DEC_Y][DEC_Z].descriptor.size() > 0);

      //then decode it
      if(decode)
      {
          PCA.decode(hGrid_[DEC_X][DEC_Y][DEC_Z].descriptor, dev_voxels);
          host_voxels_decode = dev_voxels;
      }

      //go through the block
      int idx = 0;
      for (int kk = 0; kk < 16; ++kk)
      for (int jj = 0; jj < 16; ++jj)
      for (int ii = 0; ii < 16; ++ii)
      {
        host_voxels_encode.push_back( (activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) - Dmin_)/(Dmax_-Dmin_) );
        activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ?  host_voxels_decode[idx]*(Dmax_- Dmin_) + Dmin_ : (Dmax_);
        activeVolume_w(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ? decoded_w : 0.0f;
        ++idx;
      }
      dev_voxels = host_voxels_encode;
      PCA.encode( dev_voxels, hGrid_[ENC_X][ENC_Y][ENC_Z].descriptor);
      // hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].contents = GetType(hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
    }
  }

  if (Y>0)
  {

    for (int k = 0; k < active_ZSize_/16 ; ++k)
    for (int j = 0; j < 1; ++j)
    for (int i = 0; i < active_XSize_/16 ; ++i)
    {
      thrust::host_vector<float> host_voxels_encode;
      thrust::host_vector<float> host_voxels_decode;
      thrust::device_vector<float> dev_voxels;
      int DEC_X = active_offset_[0] + i+block_shift_[0];
      int DEC_Y = active_YSize_/16 + active_offset_[1] + block_shift_[1];
      int DEC_Z = active_offset_[2] + k+block_shift_[2];

      int ENC_X = active_offset_[0] + i+block_shift_[0];
      int ENC_Y = active_offset_[1] + j+block_shift_[1];
      int ENC_Z = active_offset_[2] + k+block_shift_[2];

      //check if this block should be decoded
      bool decode = (hGrid_ [DEC_X][DEC_Y][DEC_Z].descriptor.size() > 0);

      //then decode it
      if(decode)
      {
          PCA.decode(hGrid_[DEC_X][DEC_Y][DEC_Z].descriptor, dev_voxels);
          host_voxels_decode = dev_voxels;
        // }
      }

      //go through the block
      int idx = 0;
      for (int kk = 0; kk < 16; ++kk)
      for (int jj = 0; jj < 16; ++jj)
      for (int ii = 0; ii < 16; ++ii)
      {
        host_voxels_encode.push_back( (activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) - Dmin_)/(Dmax_-Dmin_) );

        activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ?  host_voxels_decode[idx]*(Dmax_- Dmin_) + Dmin_ : (Dmax_);
        activeVolume_w(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ? decoded_w : 0.0f;
        ++idx;
      }
      dev_voxels = host_voxels_encode;

      PCA.encode( dev_voxels, hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
      // hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].contents = GetType(hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
    }
  }
  else if (Y<0)
  {

    for (int k = 0; k < active_ZSize_/16 ; ++k)
    for (int j = active_YSize_/16-1; j < active_YSize_/16 ; ++j)
    for (int i = 0; i < active_XSize_/16 ; ++i)
    {
      thrust::host_vector<float> host_voxels_encode;
      thrust::host_vector<float> host_voxels_decode;
      thrust::device_vector<float> dev_voxels;
      int DEC_X = active_offset_[0] + i+block_shift_[0];
      int DEC_Y = active_offset_[1] - 1+block_shift_[1];
      int DEC_Z = active_offset_[2] + k+block_shift_[2];

      int ENC_X =                    active_offset_[0] + i+block_shift_[0];
      int ENC_Y = active_YSize_/16 + active_offset_[1] - 1+block_shift_[1];
      int ENC_Z =                    active_offset_[2] + k+block_shift_[2];

      //check if this block should be decoded
      bool decode = (hGrid_ [DEC_X][DEC_Y][DEC_Z].descriptor.size() > 0);

      //then decode it
      if(decode)
      {
          PCA.decode(hGrid_[DEC_X][DEC_Y][DEC_Z].descriptor, dev_voxels);
          host_voxels_decode = dev_voxels;
      }

      //go through the block
      int idx = 0;
      for (int kk = 0; kk < 16; ++kk)
      for (int jj = 0; jj < 16; ++jj)
      for (int ii = 0; ii < 16; ++ii)
      {
        host_voxels_encode.push_back( (activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) - Dmin_)/(Dmax_-Dmin_) );
        activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ?  host_voxels_decode[idx]*(Dmax_- Dmin_) + Dmin_ : (Dmax_);
        activeVolume_w(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ? decoded_w : 0.0f;
        ++idx;
      }
      dev_voxels = host_voxels_encode;
      PCA.encode( dev_voxels, hGrid_[ENC_X][ENC_Y][ENC_Z].descriptor);
      // hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].contents = GetType(hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
    }
  }

  if (Z>0)
  {

    for (int k = 0; k < 1; ++k)
    for (int j = 0; j < active_YSize_/16 ; ++j)
    for (int i = 0; i < active_XSize_/16 ; ++i)
    {
      thrust::host_vector<float> host_voxels_encode;
      thrust::host_vector<float> host_voxels_decode;
      thrust::device_vector<float> dev_voxels;

      int DEC_X = active_offset_[0] + i+block_shift_[0];
      int DEC_Y = active_offset_[1] + j+block_shift_[1];
      int DEC_Z = active_ZSize_/16 + active_offset_[2] + block_shift_[2];

      int ENC_X = active_offset_[0] + i+block_shift_[0];
      int ENC_Y = active_offset_[1] + j+block_shift_[1];
      int ENC_Z = active_offset_[2] + k+block_shift_[2];

      //check if this block should be decoded
      bool decode = (hGrid_ [DEC_X][DEC_Y][DEC_Z].descriptor.size() > 0);

      //then decode it
      if(decode)
      {
          PCA.decode(hGrid_[DEC_X][DEC_Y][DEC_Z].descriptor, dev_voxels);
          host_voxels_decode = dev_voxels;
        // }
      }

      //go through the block
      int idx = 0;
      for (int kk = 0; kk < 16; ++kk)
      for (int jj = 0; jj < 16; ++jj)
      for (int ii = 0; ii < 16; ++ii)
      {
        host_voxels_encode.push_back( (activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) - Dmin_)/(Dmax_-Dmin_) );

        activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ?  host_voxels_decode[idx]*(Dmax_- Dmin_) + Dmin_ : (Dmax_);
        activeVolume_w(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ? decoded_w : 0.0f;
        ++idx;
      }
      dev_voxels = host_voxels_encode;

      PCA.encode( dev_voxels, hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
      // hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].contents = GetType(hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);

    }

  }
  else if (Z<0)
  {

    for (int k = active_ZSize_/16-1; k < active_ZSize_/16 ; ++k)
    for (int j = 0; j < active_YSize_/16 ; ++j)
    for (int i = 0; i < active_XSize_/16 ; ++i)
    {
      thrust::host_vector<float> host_voxels_encode;
      thrust::host_vector<float> host_voxels_decode;
      thrust::device_vector<float> dev_voxels;
      int DEC_X = active_offset_[0] + i + block_shift_[0];
      int DEC_Y = active_offset_[1] + j + block_shift_[1];
      int DEC_Z = active_offset_[2] - 1 + block_shift_[2];

      int ENC_X =                    active_offset_[0] + i+block_shift_[0];
      int ENC_Y =                    active_offset_[1] + j+block_shift_[1];
      int ENC_Z = active_ZSize_/16 + active_offset_[2] - 1+block_shift_[2];

      //check if this block should be decoded
      bool decode = (hGrid_ [DEC_X][DEC_Y][DEC_Z].descriptor.size() > 0);

      //then decode it
      if(decode)
      {
          PCA.decode(hGrid_[DEC_X][DEC_Y][DEC_Z].descriptor, dev_voxels);
          host_voxels_decode = dev_voxels;
      }

      //go through the block
      int idx = 0;
      for (int kk = 0; kk < 16; ++kk)
      for (int jj = 0; jj < 16; ++jj)
      for (int ii = 0; ii < 16; ++ii)
      {
        host_voxels_encode.push_back( (activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) - Dmin_)/(Dmax_-Dmin_) );
        activeVolume(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ?  host_voxels_decode[idx]*(Dmax_- Dmin_) + Dmin_ : (Dmax_);
        activeVolume_w(i*16+ii, j*16+jj, 2*(k*16+kk)) = decode ? decoded_w : 0.0f;
        ++idx;
      }
      dev_voxels = host_voxels_encode;
      PCA.encode( dev_voxels, hGrid_[ENC_X][ENC_Y][ENC_Z].descriptor);
      // hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].contents = GetType(hGrid_[ ENC_X ][ ENC_Y ][ ENC_Z ].descriptor);
    }
  }
  block_shift_[0] += X;
  block_shift_[1] += Y;
  block_shift_[2] += Z;
}


void hyperGrid::Clear(){

    for (int i = 0; i < active_XSize_; ++i)
    {
      for (int j = 0; j < active_YSize_; ++j)
      {
        if (activeVolume_[i][j]!=NULL)
        delete[] activeVolume_[i][j];
      }

      if (activeVolume_[i]!=NULL)
      delete[] activeVolume_[i];
    }
    if(activeVolume_!=NULL)
    delete[] activeVolume_;
  }


void hyperGrid::Init()
{
  
  std::string dictionary_file = pca_dictionary_path+"/pca_64.pickle";
  std::string mean_file = pca_dictionary_path+"/mean.pickle";
  PCA.load_mean(mean_file);
  PCA.load_dictionary(dictionary_file);
  
  //std::string file = nn_definitions_path + "/network_definitions/nn64/";
  //NN.load_network(file, 4);

  hGrid_ = new gridCell**[hyper_XSize_];
  for (int i = 0; i < hyper_XSize_; ++i){
    hGrid_[i] = new gridCell*[hyper_YSize_];
    for (int j = 0; j < hyper_YSize_; ++j){
      hGrid_[i][j] = new gridCell[hyper_ZSize_];
    }
  }

  int lbx = hyper_XSize_/2.0 - (active_XSize_/2.0)/16.0;
  int ubx = lbx + active_XSize_/16.0;
  int lby = hyper_YSize_/2.0 - (active_YSize_/2.0)/16.0;
  int uby = lby + active_YSize_/16.0;
  int lbz = hyper_ZSize_/2.0 - (active_ZSize_/2.0)/16.0;
  int ubz = lbz + active_ZSize_/16.0;

  active_offset_[0]=lbx;
  active_offset_[1]=lby;
  active_offset_[2]=lbz;

  // for (int x = 0; x < hyper_XSize_; ++x)
  //   for (int y = 0; y < hyper_YSize_; ++y)
  //     for (int z = 0; z < hyper_ZSize_; ++z)
  //       // hGrid_[x][y][z].contents = EMPTY;

  activeVolume_ = new float**[active_XSize_];
  for (int i = 0; i < active_XSize_; ++i){
    activeVolume_[i] = new float*[active_YSize_];
    for (int j = 0; j < active_YSize_; ++j){
      activeVolume_[i][j] = new float[active_ZSize_*2];
    }
  }
  for (int x = 0; x < active_XSize_; ++x){
    for (int y = 0; y < active_YSize_; ++y){
      for (int z = 0; z < active_ZSize_; ++z){
        activeVolume_[x][y][z*2]=Dmax_;
        activeVolume_[x][y][z*2+1]=0.0f;
      }
    }
  }

};

void
hyperGrid::FuseDepth(Eigen::Matrix4d &worldToCam, cimg_library::CImg<float> *depthImage, bool** validityMask, float fx, float fy, float cx, float cy)
{

  const float Wslope = 1/(Dmax_ - Dmin_);
  Eigen::Vector4d camera = worldToCam * Eigen::Vector4d(0.0,0.0,0.0,1.0);

  //Main 3D reconstruction loop
  for(int x = 0; x<active_XSize_; ++x)
  {
   #pragma omp parallel for \
  shared(x)
    for(int y = 0; y<active_YSize_;++y)
    {

      for(int z = 0; z<active_ZSize_; ++z)
      {
        //define a ray and point it into the center of a node
        Eigen::Vector4d ray((x-active_XSize_/2.0)*cellSize_, (y- active_YSize_/2.0)*cellSize_ , (z- active_ZSize_/2.0)*cellSize_, 1);
        ray = worldToCam*ray;
        if(ray(2)-camera(2) < 0) continue;

        Eigen::Vector2d uv;
        uv=To2D(ray,fx,fy,cx,cy );

        int j=floor(uv(0)+0.5);
        int i=floor(uv(1)+0.5);

        //if the projected coordinate is within image bounds
        if(i>0 && i<depthImage->height()-1 && j>0 && j <depthImage->width()-1 && validityMask[i][j] &&
            validityMask[i-1][j] && validityMask[i][j-1])
        {
          // const float* Di = *depthImage(j,i);
          double Eta;
          // const float W=1/((1+*depthImage(j,i))*(1+*depthImage(j,i)));

          Eta=double((*depthImage)(j,i))-ray(2);

          if(Eta >= Dmin_)
          {

            double D = std::min(Eta,Dmax_);

            float W = ((D - 1e-6) < Dmax_) ? (1.0f) : (Wslope*(D - Dmin_) + 1e-6);

            activeVolume(x, y, z*2) = (activeVolume(x, y, z*2)* activeVolume(x, y, 1+z*2) + float(D) * W) /
                      (activeVolume(x, y, 1+z*2) + W);
            activeVolume(x, y, 1+z*2) = std::min(activeVolume(x, y, 1+z*2) + W , float(Wmax_));

          }//within visible region
        }//within bounds
      }//z
    }//y
  }//x
  return;
};


double
hyperGrid::SDF(const Eigen::Vector4d &location)
{
  double i,j,k;
  double x,y,z;

  if(std::isnan(location(0)+location(1)+location(2))) return Dmax_;

  x = modf((location(0)/cellSize_)/hyperCellSize_ + hyper_XSize_/2.0, &i);
  y = modf((location(1)/cellSize_)/hyperCellSize_ + hyper_YSize_/2.0, &j);
  z = modf((location(2)/cellSize_)/hyperCellSize_ + hyper_ZSize_/2.0, &k);

  if(i>=hyper_XSize_-1 || j>=hyper_YSize_-1 || k>=hyper_ZSize_-1 || i<0 || j<0 || k<0)return Dmax_;

  int I = int(i); int J = int(j); int K = int(k);

  x = modf(location(0)/cellSize_ + active_XSize_/2.0, &i);
  y = modf(location(1)/cellSize_ + active_YSize_/2.0, &j);
  z = modf(location(2)/cellSize_ + active_ZSize_/2.0, &k);

  if(i>=0 + active_XSize_-1 ||
     j>=0 + active_YSize_-1 ||
     k>=0 + active_ZSize_-1 ||
     i <0 ||
     j <0 ||
     k <0)return Dmax_;


  I = int(i); J = int(j); K = int(k);

  float N1 = activeVolume(I,    J,    K*2); float N1_1 = activeVolume(I,    J,    K*2+2);
  float N2 = activeVolume(I,    J+1,  K*2); float N2_1 = activeVolume(I,    J+1,  K*2+2);
  float N3 = activeVolume(I+1,  J,    K*2); float N3_1 = activeVolume(I+1,  J,    K*2+2);
  float N4 = activeVolume(I+1,  J+1,  K*2); float N4_1 = activeVolume(I+1,  J+1,  K*2+2);

  double a1,a2,b1,b2;
  a1 = double(N1*(1-z)+N1_1*z);
  a2 = double(N2*(1-z)+N2_1*z);
  b1 = double(N3*(1-z)+N3_1*z);
  b2 = double(N4*(1-z)+N4_1*z);

  return double((a1*(1-y)+a2*y)*(1-x) + (b1*(1-y)+b2*y)*x);
};


double
hyperGrid::SDF_R(const Eigen::Vector4d &location)
{
  double i,j,k;
  double x,xh,y,yh,z,zh;

  if(std::isnan(location(0)+location(1)+location(2))) return Dmax_;

  xh = modf((location(0)/cellSize_)/hyperCellSize_ + hyper_XSize_/2.0, &i);
  yh = modf((location(1)/cellSize_)/hyperCellSize_ + hyper_YSize_/2.0, &j);
  zh = modf((location(2)/cellSize_)/hyperCellSize_ + hyper_ZSize_/2.0, &k);

  if(i>=hyper_XSize_-1 || j>=hyper_YSize_-1 || k>=hyper_ZSize_-1 || i<0 || j<0 || k<0)return Dmax_;

  int I = int(i); int J = int(j); int K = int(k);

  x = modf(location(0)/cellSize_ + active_XSize_/2.0, &i);
  y = modf(location(1)/cellSize_ + active_YSize_/2.0, &j);
  z = modf(location(2)/cellSize_ + active_ZSize_/2.0, &k);

  if(i>=0 + active_XSize_-1 ||
     j>=0 + active_YSize_-1 ||
     k>=0 + active_ZSize_-1 ||
     i <0 ||
     j <0 ||
     k <0)
    {
      if (hGrid_[I][J][K].descriptor.size() > 0)
      {
        return -0.0001;
      }
      else return Dmax_;
    }


  I = int(i); J = int(j); K = int(k);

  float N1 = activeVolume(I,    J,    K*2); float N1_1 = activeVolume(I,    J,    K*2+2);
  float N2 = activeVolume(I,    J+1,  K*2); float N2_1 = activeVolume(I,    J+1,  K*2+2);
  float N3 = activeVolume(I+1,  J,    K*2); float N3_1 = activeVolume(I+1,  J,    K*2+2);
  float N4 = activeVolume(I+1,  J+1,  K*2); float N4_1 = activeVolume(I+1,  J+1,  K*2+2);

  double a1,a2,b1,b2;
  a1 = double(N1*(1-z)+N1_1*z);
  a2 = double(N2*(1-z)+N2_1*z);
  b1 = double(N3*(1-z)+N3_1*z);
  b2 = double(N4*(1-z)+N4_1*z);

  return double((a1*(1-y)+a2*y)*(1-x) + (b1*(1-y)+b2*y)*x);
};

double
hyperGrid::SDFGradient_R(const Eigen::Vector4d &location, int stepSize, int dim )
{
  double c[3] = {1.0,0.0,0.0};
  if(SDF_R(location) < -1.0) return (c[dim]);

  double delta=cellSize_*stepSize;
  Eigen::Vector4d location_offset = Eigen::Vector4d(0,0,0,1);
  location_offset(dim) = delta;

  return ((SDF(location+location_offset)) - (SDF(location-location_offset)))/(2.0*delta);
};


float& hyperGrid::activeVolume(int x, int y, int z)
{
  int idx_x = mod(x + block_shift_[0]*hyperCellSize_, active_XSize_);
  int idx_y = mod(y + block_shift_[1]*hyperCellSize_, active_YSize_);
  int idx_z = mod(z + 2*block_shift_[2]*hyperCellSize_, active_ZSize_*2);
  return activeVolume_[idx_x][idx_y][idx_z];
}

float& hyperGrid::activeVolume_w(int x, int y, int z)
{
  int idx_x = mod(x + block_shift_[0]*hyperCellSize_, active_XSize_);
  int idx_y = mod(y + block_shift_[1]*hyperCellSize_, active_YSize_);
  int idx_z = mod(z + 2*block_shift_[2]*hyperCellSize_, active_ZSize_*2);
  return activeVolume_[idx_x][idx_y][idx_z +1];
}

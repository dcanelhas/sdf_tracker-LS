#include <OpenNI.h>
#include <sdf_tracker.h>

#include <chrono>
#include <sstream>
#include <iomanip>

class FrameGrabber
{
  public:
    FrameGrabber(int width, int height, int fps=0);
    ~FrameGrabber();

    int getNumberOfFrames(void);
    void getColor(cimg_library::CImg<unsigned char> &color);
    void getDepth(cimg_library::CImg<float> &depth);
  private:

    openni::PlaybackControl* pbc;
    openni::Device* rgbd_camera_;
    openni::VideoStream *depth_stream_;
    openni::VideoStream *color_stream_;
    openni::VideoFrameRef depth_FrameRef_;
    openni::VideoFrameRef color_FrameRef_;
};


int main(int argc, char* argv[])
{
  //Parameters for an SDFtracker object
  SDF_Parameters myParameters;
  Vector6d poseVector;

  myParameters.interactive_mode = true;
  myParameters.resolution = 0.02;
  myParameters.Dmax = 0.1;
  myParameters.Dmin = -0.1;
  myParameters.raycast_steps = 12;

  // The sizes can be different from each other
  // +Y is up +Z is forward.
  myParameters.XSize = 128;
  myParameters.YSize = 128;
  myParameters.ZSize = 128;

  myParameters.image_width = 320;
  myParameters.image_height = 240;
  int fps = 60;

  //Pose Offset as a transformation matrix
  Eigen::Matrix4d currentTransformation =
  Eigen::MatrixXd::Identity(4,4);
  
  //create the tracker object
  SDFTracker* myTracker = new SDFTracker(myParameters);
  myTracker->SetCurrentTransformation(currentTransformation);

  //create the camera object
  FrameGrabber* myCamera = new FrameGrabber(myParameters.image_width, myParameters.image_height, fps);
  cimg_library::CImg<float> depth_img(myParameters.image_width, myParameters.image_height,1,1);
  cimg_library::CImg<unsigned char> rgb_img(myParameters.image_width, myParameters.image_height,1,3);

  unsigned int frame_nr = 0;
  unsigned int frame_limit = (myCamera->getNumberOfFrames()) ? myCamera->getNumberOfFrames() : 100000;
  

  do
  {
    // auto tic = std::chrono::high_resolution_clock::now();
    myCamera->getDepth(depth_img);

    myTracker->UpdateDepth(depth_img);

    poseVector = myTracker->EstimatePoseFromDepth();
    
    currentTransformation = GetMat4_rodrigues_smallangle(poseVector)*myTracker->GetCurrentTransformation();
    // Twist(poseVector).exp()*myTracker->GetCurrentTransformation();

    myTracker->SetCurrentTransformation(currentTransformation);
    myTracker->FuseDepth();
    myTracker->Render();    
    // toc = std::chrono::high_resolution_clock::now();
    // std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic).count() << "ms \n";

    if(++frame_nr%10 == 0)
    {
      myTracker->checkTranslation(1);
    }
  }while(!myTracker->Quit() && frame_nr < frame_limit);
  delete myCamera;
  delete myTracker;
  exit(0);
}

int FrameGrabber::getNumberOfFrames(void){return pbc->getNumberOfFrames(*depth_stream_);};

FrameGrabber::FrameGrabber(int width, int height,  int fps)
{
  if (fps==0)
  {
    if (width==640 && height ==480) fps = 30;
    else if (width==320 && height ==240) fps = 60;
    else{ fps=25; }
    std::cout << "fps not set, assuming " << fps << " to match resolution of "<< width << "x" << height << std::endl;
  }
  int rc = openni::OpenNI::initialize();
  if (rc != openni::STATUS_OK)
  {
    printf("Initialize failed\n%s\n", openni::OpenNI::getExtendedError());
    exit(1);
  }
  std::cout << "OpenNI initialized" << std::endl;

  rgbd_camera_ = new openni::Device();
  color_stream_ = new openni::VideoStream;
  depth_stream_ = new openni::VideoStream;
  openni::VideoMode vm;

  rc = rgbd_camera_->open(openni::ANY_DEVICE);
  // rc = rgbd_camera_->open("Captured.oni"); pbc = rgbd_camera_->getPlaybackControl();  pbc->setSpeed(-1);

  if (rc != openni::STATUS_OK)
  {
    printf("Couldn't open device\n%s\n", openni::OpenNI::getExtendedError());
    exit(2);
  }
  std::cout << "Opened device" << std::endl;

  rc = depth_stream_->create(*rgbd_camera_, openni::SENSOR_DEPTH);
  if (rc != openni::STATUS_OK)
  {
    printf("Couldn't create depth stream\n%s\n", openni::OpenNI::getExtendedError());
    exit(3);
  }
  std::cout << "Created depth stream" << std::endl;

  for(int idx = 0; idx< depth_stream_->getSensorInfo().getSupportedVideoModes().getSize(); idx++)
  {
    vm = depth_stream_->getSensorInfo().getSupportedVideoModes()[idx];
    if( (height == vm.getResolutionY()) && (width == vm.getResolutionX()) && (fps == vm.getFps()) && (vm.getPixelFormat()==openni::PIXEL_FORMAT_DEPTH_1_MM))
    {
      std::cout << "matching depth mode :" << vm.getResolutionX()<<"x"<<vm.getResolutionY()<<"@"<<vm.getFps()<<"Hz using format "<< vm.getPixelFormat() <<std::endl;
      break;
    }
  }
  std::cout << "setting depth mode :" << vm.getResolutionX()<<"x"<<vm.getResolutionY()<<"@"<<vm.getFps()<<"Hz using format "<< vm.getPixelFormat() <<std::endl;

  rc = depth_stream_->setVideoMode(vm);
  if (rc != openni::STATUS_OK) printf("Couldn't set desired video mode \n%s\n", openni::OpenNI::getExtendedError());

  rc = depth_stream_->setMirroringEnabled(TRUE);
  if (rc != openni::STATUS_OK) printf("Couldn't set mirroring \n%s\n", openni::OpenNI::getExtendedError());

  rc = color_stream_->create(*rgbd_camera_, openni::SENSOR_COLOR);
  if (rc != openni::STATUS_OK)
  {
    printf("Couldn't create color stream\n%s\n", openni::OpenNI::getExtendedError());
    exit(5);
  }
  std::cout << "Created color stream" << std::endl;

  rc = rgbd_camera_->setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
  if (rc != openni::STATUS_OK) printf("Couldn't enable depth registration \n%s\n", openni::OpenNI::getExtendedError());

  rc = rgbd_camera_->setDepthColorSyncEnabled(true);
  if (rc != openni::STATUS_OK) printf("Couldn't enable frame syncronization \n%s\n", openni::OpenNI::getExtendedError());

  for(int idx = 0; idx< color_stream_->getSensorInfo().getSupportedVideoModes().getSize(); idx++)
  {
    vm = color_stream_->getSensorInfo().getSupportedVideoModes()[idx];
    if( (height == vm.getResolutionY()) && (width == vm.getResolutionX()) && (fps == vm.getFps()) && (vm.getPixelFormat()==openni::PIXEL_FORMAT_RGB888))
    {
      std::cout << "found matching color mode :" << vm.getResolutionX()<<"x"<<vm.getResolutionY()<<"@"<<vm.getFps()<<"Hz using format "<< vm.getPixelFormat() <<std::endl;
      break;
    }
  }

  std::cout << "setting color mode :" << vm.getResolutionX()<<"x"<<vm.getResolutionY()<<"@"<<vm.getFps()<<"Hz using format "<< vm.getPixelFormat() <<std::endl;

  rc = color_stream_->setVideoMode(vm);
  if (rc != openni::STATUS_OK) printf("Couldn't set desired video mode \n%s\n", openni::OpenNI::getExtendedError());

  rc = color_stream_->setMirroringEnabled(TRUE);
  if (rc != openni::STATUS_OK) printf("Couldn't set mirroring \n%s\n", openni::OpenNI::getExtendedError());

  rc = depth_stream_->start();
  if (rc != openni::STATUS_OK)
  {
    printf("Couldn't start the depth stream\n%s\n", openni::OpenNI::getExtendedError());
    exit(4);
  }
  std::cout << "Started depth stream" << std::endl;

  rc = color_stream_->start();
  if (rc != openni::STATUS_OK)
  {
    printf("Couldn't start the color stream\n%s\n", openni::OpenNI::getExtendedError());
    exit(6);
  }
  std::cout << "Started color stream" << std::endl;
  std::cout << "Finished setting up camera " << std::endl;
};

FrameGrabber::~FrameGrabber()
{

  // depth_stream_->removeNewFrameListener(this);
  depth_stream_->stop();
  depth_stream_->destroy();
  delete depth_stream_;

  // color_stream_->removeNewFrameListener(this);
  color_stream_->stop();
  color_stream_->destroy();
  delete color_stream_;

  rgbd_camera_->close();

  delete rgbd_camera_;
  openni::OpenNI::shutdown();
};

void FrameGrabber::getDepth(cimg_library::CImg<float> &depthmap)
{

  depth_stream_->readFrame(&depth_FrameRef_);
  const openni::DepthPixel* pDepth = (const openni::DepthPixel*)depth_FrameRef_.getData();

  // if(depthmap._data == NULL)
  //   depthmap = cimg_library::CImg<float>(depth_FrameRef_.getHeight(), depth_FrameRef_.getWidth(),1);

    #pragma omp parallel for collapse(2) schedule(static)
  for(int i=0; i<depth_FrameRef_.getHeight(); ++i)
  for(int j=0; j<depth_FrameRef_.getWidth(); ++j)
  {
    // depthmap(j,i,0,1) = 1.0f;
    depthmap(depth_FrameRef_.getWidth()-(j+1),i,0,0) = (pDepth[i*depth_FrameRef_.getWidth() + j] == 0) ? std::numeric_limits<float>::quiet_NaN() : float(pDepth[i*depth_FrameRef_.getWidth() + j])/1000;
    // depthmap(j,i) = 10*(pDepth[i*depth_FrameRef_.getWidth() + j] == 0) ? std::numeric_limits<float>::quiet_NaN() : float(pDepth[i*depth_FrameRef_.getWidth() + j])/1000;
  }
};

void FrameGrabber::getColor(cimg_library::CImg<unsigned char> &rgbImage)
{
  color_stream_->readFrame(&color_FrameRef_);
  const openni::RGB888Pixel* pColor = (const openni::RGB888Pixel*)color_FrameRef_.getData();

  // if(rgbImage._data == NULL)
  //   rgbImage = cimg_library::CImg<unsigned char>(color_FrameRef_.getWidth(), color_FrameRef_.getHeight(), 1);

    #pragma omp parallel for collapse(2) schedule(static)
  for(int i=0; i<color_FrameRef_.getHeight(); ++i)
  for(int j=0; j<color_FrameRef_.getWidth(); ++j)
  {
    rgbImage(color_FrameRef_.getWidth()-(j+1),i,0,2) = pColor[i*color_FrameRef_.getWidth() + j ].b;
    rgbImage(color_FrameRef_.getWidth()-(j+1),i,0,1) = pColor[i*color_FrameRef_.getWidth() + j ].g;
    rgbImage(color_FrameRef_.getWidth()-(j+1),i,0,0) = pColor[i*color_FrameRef_.getWidth() + j ].r;
  }
};

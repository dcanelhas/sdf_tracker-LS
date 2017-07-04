#include <sdf_tracker.h>
#include <OpenNI.h>
#include <opencv2/opencv.hpp>

class FrameGrabber
{
  public:
    FrameGrabber(int width, int height, int fps=0); 
    ~FrameGrabber(); 
    
    void getColor(cv::Mat &color);
    void getDepth(cv::Mat &depth);
  private:
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

  myParameters.interactive_mode = true;
  myParameters.resolution = 0.02;
  myParameters.Dmax = 0.2;
  myParameters.Dmin = -0.1;

  // The sizes can be different from each other 
  // +Y is up +Z is forward.
  myParameters.XSize = 250; 
  myParameters.YSize = 250;
  myParameters.ZSize = 250;

  //QVGA for slow computers 
  myParameters.image_width = 320;
  myParameters.image_height = 240;
  int fps = 60;

  //Pose Offset as a transformation matrix
  Eigen::Matrix4d initialTransformation = 
  Eigen::MatrixXd::Identity(4,4);

  //define translation offsets in x y z
  // initialTransformation(0,3) = 0.0;  //x 
  // initialTransformation(1,3) = 0.0;  //y
  // initialTransformation(2,3) = -0.5*myParameters.ZSize*myParameters.resolution - 0.4; //z
  myParameters.pose_offset = initialTransformation;

  //create the tracker object
  SDFTracker myTracker(myParameters);

    //create the tracker object
  FrameGrabber myCamera(myParameters.image_width, myParameters.image_height, fps);
  
  cv::Mat depth;
  // cv::Mat color;
  do
  { 
    myCamera.getDepth(depth);
    myTracker.FuseDepth(depth);

  }while(!myTracker.Quit());

  printf("Dumping volume and exiting.\n");
  myTracker.SaveSDF();

  return 0;
}

FrameGrabber::FrameGrabber(int width, int height,  int fps)
{
  if (fps==0) 
  {
    if (width==640 && height ==480) fps = 30;
    else if (width==320 && height ==240) fps = 60;
    else{ fps=25; }
    std::cout << "fps not set, assuming " << fps << " to match resolution of "<< width << "x" << height << std::endl;
  }
  openni::Status rc = openni::OpenNI::initialize();
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
}

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
}

void FrameGrabber::getDepth(cv::Mat &depthmap)
{

  depth_stream_->readFrame(&depth_FrameRef_);
  const openni::DepthPixel* pDepth = (const openni::DepthPixel*)depth_FrameRef_.getData();

  if(depthmap.data == NULL)
    depthmap = cv::Mat(depth_FrameRef_.getHeight(), depth_FrameRef_.getWidth(), CV_32FC1, cv::Scalar(0.0));

  for(int i=0; i<depth_FrameRef_.getHeight(); ++i)
  #pragma omp parallel
  for(int j=0; j<depth_FrameRef_.getWidth(); ++j) 
  {        
    depthmap.at<float>(i,depth_FrameRef_.getWidth()-(j+1)) = (pDepth[i*depth_FrameRef_.getWidth() + j] == 0) ? std::numeric_limits<float>::quiet_NaN() : float(pDepth[i*depth_FrameRef_.getWidth() + j])/1000;
  }
}

void FrameGrabber::getColor(cv::Mat &rgbImage)
{
  color_stream_->readFrame(&color_FrameRef_);
  const openni::RGB888Pixel* pColor = (const openni::RGB888Pixel*)color_FrameRef_.getData();
  
  if(rgbImage.data == NULL)
    rgbImage = cv::Mat(color_FrameRef_.getHeight(), color_FrameRef_.getWidth(), CV_8UC3, cv::Scalar(0));

  for(int i=0; i<color_FrameRef_.getHeight(); ++i)
  #pragma omp parallel
  for(int j=0; j<color_FrameRef_.getWidth(); ++j) 
  {
    rgbImage.at<cv::Vec3b>(i,color_FrameRef_.getWidth()-(j+1))[0] = pColor[i*color_FrameRef_.getWidth() + j ].b;
    rgbImage.at<cv::Vec3b>(i,color_FrameRef_.getWidth()-(j+1))[1] = pColor[i*color_FrameRef_.getWidth() + j ].g;
    rgbImage.at<cv::Vec3b>(i,color_FrameRef_.getWidth()-(j+1))[2] = pColor[i*color_FrameRef_.getWidth() + j ].r;
  }
}
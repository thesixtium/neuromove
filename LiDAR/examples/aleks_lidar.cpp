#include "unitree_lidar_sdk.h"
#include "udp_handler.h"
#include <cstring>

using namespace unitree_lidar_sdk;

int main(){

  // Initialize Lidar Object
  UnitreeLidarReader* lreader = createUnitreeLidarReader();
  int cloud_scan_num = 18;
  std::string port_name = "/dev/ttyUSB0";

  if ( lreader->initialize(cloud_scan_num, port_name) ){
    printf("Unilidar initialization failed! Exit here!\n");
    exit(-1);
  }else{
    printf("Unilidar initialization succeed!\n");
  }

  // Set Lidar Working Mode
  printf("Set Lidar working mode to: STANDBY ... \n");
  lreader->setLidarWorkingMode(STANDBY);
  sleep(1);

  printf("Set Lidar working mode to: NORMAL ... \n");
  lreader->setLidarWorkingMode(NORMAL);
  sleep(1);

  printf("\n");

  // Print Lidar Version
  while(true){
    if (lreader->runParse() == VERSION){
      printf("lidar firmware version = %s\n", lreader->getVersionOfFirmware().c_str() );
      break;
    }
    usleep(500);
  }
  printf("lidar sdk version = %s\n\n", lreader->getVersionOfSDK().c_str());
  sleep(2);

  // Check lidar dirty percentange
  int count_percentage = 0;
  while(true){
    if( lreader->runParse() == AUXILIARY){
      printf("Dirty Percentage = %f %%\n", lreader->getDirtyPercentage());
      if (++count_percentage > 2){
        break;
      }
      if (lreader->getDirtyPercentage() > 10){
        printf("The protection cover is too dirty! Please clean it right now! Exit here ...\n");
        // exit(0);
      }
    }
    usleep(500);
  }
  printf("\n");
  sleep(2);

  // UDP
  std::string destination_ip;
  unsigned short destination_port;
  destination_ip = "127.0.0.1";
  destination_port = 12345;
  UDPHandler client;
  client.CreateSocket();

  // Parse PointCloud and IMU data
  MessageType result;
  std::string version;
  PointCloudUnitree cloud;
  int pointCloudSize;
  float x;
  float y;
  float z;
  char string_template[] = "(%f,%f,%f)";
  while (true)
  {
    result = lreader->runParse(); // You need to call this function at least 1500Hz

    switch (result)
    {
    
    case POINTCLOUD: {
      cloud = lreader->getCloud();
      pointCloudSize = cloud.points.size();
      //scanMsg.stamp = cloudMsg.stamp;
      
	  for (int i = 0; i < pointCloudSize; i++){
		  
		  x = cloud.points[i].x;
		  y = cloud.points[i].y;
		  z = cloud.points[i].z;
		  char formatted_string[1000];
		  sprintf(formatted_string, string_template, x, y, z);
		  
          client.Send(formatted_string, strlen(formatted_string), (char *)destination_ip.c_str(), destination_port);
      }
      
      break;
      }
      

    default:
      break;
    }

    // usleep(500);
  }
  
  return 0;
}

// pi@raspberrypi:~/Documents/neuromove/src/LiDAR/ $ cmake . && make -j2

#include "unitree_lidar_sdk.h"
#include "udp_handler.h"
#include <cstring>
#include <string>
#include <cmath> 
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <chrono>

using namespace unitree_lidar_sdk;

// Constants - LiDAR Initialization
int cloud_scan_num = 18;
std::string port_name = "/dev/ttyUSB0";

// Constants - Occupancy Grid
int z1 = 1; // meters
int LiDAR_radius_cm = 4000;
int resolution = 8;
int NEEDED_POINTCLOUDS_READ = 100;

int point_cloud_to_grid(int resolution, float pc){
  return static_cast<int>(round((pc*100) / resolution));
}

int main(){
    // Initialize Lidar Object
    UnitreeLidarReader* lreader = createUnitreeLidarReader();
    if ( lreader->initialize(cloud_scan_num, port_name) ){
        printf("Unilidar initialization failed! Exit here!\n");
        exit(-1);
    } else {
        printf("Unilidar initialization succeed!\n");
    }

    // Set Lidar Working Mode
    printf("Set Lidar working mode to: STANDBY ... \n");
    lreader->setLidarWorkingMode(STANDBY);
    sleep(1);

    // Set Lidar Working Mode
    printf("Set Lidar working mode to: NORMAL ... \n");
    lreader->setLidarWorkingMode(NORMAL);
    sleep(1);

    printf("\n");

    // Check lidar dirty percentage
    int count_percentage = 0;
    while ( true ) {
        if( lreader->runParse() == AUXILIARY ) {
            printf( "Dirty Percentage = %f %%\n", lreader->getDirtyPercentage() );
            if ( ++count_percentage > 2 ) { break; }
            if ( lreader->getDirtyPercentage() > 10 ) {
                printf( "Protection cover is too dirty! Please clean it right now!\n" );
            }
        }
        usleep(500);
    }
    printf("\n");
    sleep(2);

    // Occupancy Grid Sizing
    std::cout << "Occupancy Grid Sizing" << std::endl;
    int grid_height = (LiDAR_radius_cm * 2) / resolution;
    int grid_width = grid_height;

    // Parse PointCloud and IMU data
    std::cout << "Parse PointCloud and IMU data" << std::endl;
    MessageType result;
    std::string version;
    PointCloudUnitree cloud;
    int pointCloudSize;
    float x;
    float y;
    float z;
    int occupancy_grid[grid_width][grid_height] = {0};
    int pointcloud_reads = 0;

    // OC Shared Memory
    std::cout << "OC Shared Memory" << std::endl;
    size_t shm_size_oc = 28462200;
    const char * shmem_name_oc = "occupancy_grid";
    int shmem_fd_oc = shm_open( shmem_name_oc, O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
    if ( shmem_fd_oc == -1 ) { perror("shm_open"); return 1; }
    std::cout << "Shared Memory segment opened with fd " << shmem_fd_oc << std::endl;
    if ( ftruncate( shmem_fd_oc, shm_size_oc ) == -1 ) { perror( "ftruncate" ); return 1; }
    std::cout << "Shared Memory segment resized to " << shm_size_oc << std::endl;
    void * addr_oc = mmap( 0, shm_size_oc, PROT_WRITE, MAP_SHARED, shmem_fd_oc, 0 );
    if ( addr_oc == MAP_FAILED ) { perror( "mmap" ); return 1; }

    // Read Data
    while (true) {
        result = lreader->runParse(); // You need to call this function at least 1500Hz
        switch (result) {


            case POINTCLOUD: {
                cloud = lreader->getCloud();
                pointCloudSize = cloud.points.size();

                // Build Occupancy Grid
                for (int i = 0; i < pointCloudSize; i++){
                    x = cloud.points[i].y;
                    z = cloud.points[i].z;
                    y = cloud.points[i].x;

                    if ( z < z1 ){
                        int new_x = point_cloud_to_grid( resolution, x ) + ( grid_height / 2 );
                        int new_y = point_cloud_to_grid( resolution, y ) + ( grid_width / 2 );
                        occupancy_grid[new_x][new_y] = 1;
                    }
                }

                // Write Occupancy Grid
                if ( pointcloud_reads >= NEEDED_POINTCLOUDS_READ ) {
                    // Write values to memory and erase old grid
                    std::string values;
                    int points = 0;
                    int total = 0;

                    for( int i = 0; i < grid_width; i++ ) {
                        for( int j = 0; j < grid_height; j++ ) {
                            values += std::to_string( occupancy_grid[i][j] );
                            if (occupancy_grid[i][j] == 1){
                                points += 1;
                            }
                            total += 1;
                            occupancy_grid[i][j] = 0;
                        }
                        values += "|";
                    }

                    strncpy( (char *)addr_oc, values.data(), shm_size_oc );
                    pointcloud_reads = 0;
                    std::cout << "Cloud: " << points << "/" << total << std::endl;
                } else {
                    pointcloud_reads++;
                }
                break;
            }

            default:
                break;
        }
    }

    return 0;
}

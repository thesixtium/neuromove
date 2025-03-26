// pi@raspberrypi:~/Documents/neuromove/src/LiDAR/bin $ cmake .. && make -j2 && sudo ../bin/aleks_lidar


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

using namespace unitree_lidar_sdk;

// Constants - LiDAR Initialization
int cloud_scan_num = 18;
std::string port_name = "/dev/ttyUSB0";

// Constants - Occupancy Grid
int z1 = 1; // meters
int LiDAR_radius_cm = 4000;
int resolution = 20;
int NEEDED_POINTCLOUDS_READ = 5;

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
    int grid_height = (LiDAR_radius_cm * 2) / resolution;
    int grid_width = grid_height;

    // Parse PointCloud and IMU data
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
    size_t shm_size_oc = 284622;
    const char * shmem_name_oc = "occupancy_grid";
    int shmem_fd_oc = shm_open( shmem_name_oc, O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
    if ( shmem_fd_oc == -1 ) { perror("shm_open"); return 1; }
    std::cout << "OC Shared Memory segment opened with fd " << shmem_fd_oc << std::endl;
    if ( ftruncate( shmem_fd_oc, shm_size_oc ) == -1 ) { perror( "ftruncate" ); return 1; }
    std::cout << "OC Shared Memory segment resized to " << shm_size_oc << std::endl;
    void * addr_oc = mmap( 0, shm_size_oc, PROT_WRITE, MAP_SHARED, shmem_fd_oc, 0 );
    if ( addr_oc == MAP_FAILED ) { perror( "mmap" ); return 1; }

    // IMU Shared Memory
    size_t shm_size_imu = 284622;
    const char * shmem_name_imu = "imu";
    int shmem_fd_imu = shm_open( shmem_name_imu, O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP );
    if ( shmem_fd_imu == -1 ) { perror("shm_open"); return 1; }
    std::cout << "IMU Shared Memory segment opened with fd " << shmem_fd_imu << std::endl;
    if ( ftruncate( shmem_fd_imu, shm_size_imu ) == -1 ) { perror( "ftruncate" ); return 1; }
    std::cout << "IMU Shared Memory segment resized to " << shm_size_imu << std::endl;
    void * addr_imu = mmap( 0, shm_size_imu, PROT_WRITE, MAP_SHARED, shmem_fd_imu, 0 );
    if ( addr_imu == MAP_FAILED ) { perror( "mmap" ); return 1; }

    // Read Data
    std::cout << "\n\n\n\nTIME TO READ DATA!!!!!\n\n\n\n" << std::endl;
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

                    for( int i = 0; i < grid_width; i++ ) {
                        for( int j = 0; j < grid_height; j++ ) {
                            values += std::to_string( occupancy_grid[i][j] );
                            occupancy_grid[i][j] = 0;
                        }
                        values += "|";
                    }

                    strncpy( (char *)addr_oc, values.data(), shm_size_oc );
                } else {
                    pointcloud_reads++;
                }
                break;
            }


            case IMU: {
                std::string values;

                values += std::to_string( lreader->getIMU().quaternion[0] );
                values += ",";
                values += std::to_string( lreader->getIMU().quaternion[1] );
                values += ",";
                values += std::to_string( lreader->getIMU().quaternion[2] );
                values += ",";
                values += std::to_string( lreader->getIMU().quaternion[3] );
                
                strncpy( (char *)addr_imu, values.data(), shm_size_imu );
                break;
            }

            default:
                break;
        }
    }

    return 0;
}

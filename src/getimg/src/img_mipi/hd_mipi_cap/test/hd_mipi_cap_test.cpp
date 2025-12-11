#include <cstdio>
#include <thread>
#include <iostream>
#include <chrono>
#include "../hd_mipi_cap.h"

int main(int, char**){
    std::cout << "Hello, from mipi_image_cap!\n";

    // Init dev param
    HDDeviceParam dev_param{};
    dev_param.dev_name = "/dev/video0";
    dev_param.dev_id = "mipi";

    // Open mipi device 
    HDMipiDevState *mipi_dev = hd_open_mipi_video(&dev_param);
    if (mipi_dev == NULL)  {
        fprintf(stderr, "Failed to open mipi device: %s\n", dev_param.dev_name);
        return 1;
    }

    // Start mipi stream
    hd_start_mipi_video(mipi_dev);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Frame cap loop
    HDImage *hd_image = new HDImage{};

    for(int i = 0; ; ++i) {
        hd_get_mipi_video_frame(mipi_dev, hd_image);
        if (hd_image->data != NULL) {
            fprintf(stdout, "Get mipi frame success, image width: %ld, image height: %ld\n", hd_image->param->width, hd_image->param->height);
            
            FILE *fp = fopen("mipi.raw", "wb");
            fwrite(hd_image->data, 1, 640 * 512 * 2, fp);
            fclose(fp);

            hd_free_mipi_frame(mipi_dev, (HDMipiBuffer *)hd_image->hd_buf);
        }
        else {
            fprintf(stdout, "Get mipi frame NULL\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // Release mipi dev resources
    hd_stop_mipi_video(mipi_dev);
    hd_clear_mipi_dev(mipi_dev);
}

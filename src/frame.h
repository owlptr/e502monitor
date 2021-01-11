#define FRAME_FULL 1
#define FRAME_NOT_FULL 0    

typedef struct
{
    //double* data;
    float* data;
    int* channels;
    int* full_channel_markers;
    int size;
} data_frame;

data_frame* create_frame(int frame_size, int* channels);
void destroy_frame(data_frame** dframe);
void add_data_in_frame(data_frame* dframe, double data, int channel);
int is_frame_full(data_frame* dframe);
void clear_frame(data_frame* dframe);


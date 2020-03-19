#include "frame.h"


data_frame* create_frame(int frame_size, int* channels)
{
    data_frame* dframe = (data_frame*)malloc(sizeof(data_frame));

    dframe->data = (double*)malloc(sizeof(double)*frame_size);

    dframe->channels = (int*)malloc(sizeof(int)*frame_size);
    
    for(int i = 0; i < frame_size; i++)
    {
        dframe->channels[i] = channels[i];
    }

    dframe->full_channel_markers = (int*)malloc(sizeof(int)*frame_size);
    
    for(int i = 0; i < frame_size; i++)
    {
        dframe->full_channel_markers[i] = 0;
    }

    dframe->size = frame_size;

    return dframe;
}

void destroy_frame(data_frame** dframe)
{
    free((*dframe)->data);
    free((*dframe)->channels);
    free((*dframe)->full_channel_markers);

    free(*dframe);
}

void add_data_in_frame(data_frame* dframe, double data, int channel)
{
    // search for channel index
    int ch_index = 0;

    for(int i = 0; i < dframe->size; i++)
    {
        if( dframe->channels[i] == channel )
        {
            ch_index = i;
            break;
        }
    }

    dframe->data[ch_index] = data;
    dframe->full_channel_markers[ch_index] = 1;
}

int is_frame_full(data_frame* dframe)
{
    for( int i = 0; i < dframe->size; i++)
    {
        if( dframe->full_channel_markers[i] == 0 )
        {
            return FRAME_NOT_FULL;
        }
    }

    return FRAME_FULL;
}

void clear_frame(data_frame* dframe)
{
    for( int i = 0; i < dframe->size; i++)
    {
        dframe->full_channel_markers[i] = 0;
    }
}
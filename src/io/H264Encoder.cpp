#include "vision/config.h"

#if Vision_HAS_LIBAVCODEC == 1

#include "vision/io/H264Encoder.h"

namespace vision
{

H264Encoder::H264Encoder(int width, int height, int fps)
: m_width(width), m_height(height), m_fps(fps)
{
    m_stride = width*4;

    /* Parametrize x264 for real-time */
    x264_param_default_preset(&m_param, "veryfast", "zerolatency");
    m_param.i_threads = 1;
    m_param.i_width = width;
    m_param.i_height = height;
    m_param.i_fps_num = fps;
    m_param.i_fps_den = 1;
    // Intra refres:
    m_param.i_keyint_max = fps;
    m_param.b_intra_refresh = 1;
    //Rate control:
    m_param.rc.i_rc_method = X264_RC_CRF;
    m_param.rc.f_rf_constant = 25;
    m_param.rc.f_rf_constant_max = 35;
    //For streaming:
    m_param.b_repeat_headers = 1;
    m_param.b_annexb = 1;
    x264_param_apply_profile(&m_param, "baseline");

    Init(&m_param);
}

H264Encoder::H264Encoder(x264_param_t * param)
{
    Init(param);
}

void H264Encoder::Init(x264_param_t * param)
{
    m_encoder = x264_encoder_open(param);
    m_width = param->i_width;
    m_height = param->i_height;
    m_stride = m_width*4;
    x264_picture_alloc(&m_pic_in, X264_CSP_I420, m_width, m_height);
    m_convert_ctx = sws_getContext(m_width, m_height, PIX_FMT_RGBA, m_width, m_height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
}

H264Encoder::~H264Encoder()
{
    sws_freeContext(m_convert_ctx);
    x264_encoder_close(m_encoder);
}
    
H264EncoderResult H264Encoder::Encode(vision::Image<uint32_t, vision::RGB> & img, uint64_t pts)
{
    x264_nal_t* nals;
    int i_nals;
    H264EncoderResult res;

    /* Convert from RGBA to YUV420P */
    uint8_t *buf_in[4]={(uint8_t*)img.raw_data,NULL,NULL,NULL};
    sws_scale(m_convert_ctx, (const uint8_t* const*)buf_in, &m_stride, 0, m_height, m_pic_in.img.plane, m_pic_in.img.i_stride);
    m_pic_in.i_pts = pts;

    /* Encode */
    while( (res.frame_size = x264_encoder_encode(m_encoder, &nals, &i_nals, &m_pic_in, &m_pic_out)) == 0 ) { m_pic_in.i_pts++; }
    res.frame_data = nals[0].p_payload;

    return res;
}

} // namespace vision

#endif // Vision_HAS_LIBAVCODEC


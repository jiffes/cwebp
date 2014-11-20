//
//  cwebp.c
//  cwebp
//
//  Created by Jeffery on 20/11/14.
//  Copyright (c) 2014 Jeffery. All rights reserved.
//

#include <stdio.h>
#include "webp/encode.h"
#include <jpeglib.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#pragma result
#define ERROR(ptr) jpeg_destroy_decompress(ptr);\
                    return 1;\

enum {
    succ=0,
    fail,
};

struct error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

static void error_exit(j_common_ptr dinfo) {
    struct error_mgr* myerr = (struct error_mgr*) dinfo->err;
    (*dinfo->err->output_message) (dinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

static int jpeg_read(FILE *ifile, WebPPicture* const pic) {
    int ok = 0;
    int stride, width, height;
    uint8_t* rgb = NULL;
    uint8_t* row_ptr = NULL;
    struct jpeg_decompress_struct dinfo;
    struct error_mgr jerr;
    JSAMPARRAY buffer;
    
    dinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = error_exit;
    
    if(setjmp(jerr.setjmp_buffer)) {
        ERROR(&dinfo);
    }
    
    jpeg_create_decompress(&dinfo);
    jpeg_stdio_src(&dinfo, ifile);
    jpeg_read_header(&dinfo, TRUE);
    
    dinfo.out_color_space = JCS_RGB;
    dinfo.dct_method = JDCT_IFAST;
    dinfo.do_fancy_upsampling = TRUE;
    
    jpeg_start_decompress(&dinfo);
    
    if(dinfo.output_components != 3) {
        ERROR(&dinfo);
    }
    
    width = dinfo.output_width;
    height = dinfo.output_height;
    stride = dinfo.output_width * dinfo.output_components * sizeof(*rgb);
    
    rgb = (uint8_t*)malloc(stride * height);
    if(rgb == NULL) {
        if (rgb) {
            free(rgb);
        }
        return ok;
    }
    
    row_ptr = rgb;
    
    buffer = (*dinfo.mem->alloc_sarray) ((j_common_ptr) &dinfo,
                                         JPOOL_IMAGE, stride, 1);
    if (buffer == NULL) {
        if (rgb) {
            free(rgb);
        }
        return ok;
    }

    
    while (dinfo.output_scanline < dinfo.output_height) {
        if (jpeg_read_scanlines(&dinfo, buffer, 1) != 1) {
            if (rgb) {
                free(rgb);
            }
            return ok;

        }
        memcpy(row_ptr, buffer[0], stride);
        row_ptr += stride;
    }
    
    jpeg_finish_decompress (&dinfo);
    jpeg_destroy_decompress (&dinfo);
    
    pic->width = width;
    pic->height = height;
    ok = WebPPictureImportRGB(pic, rgb, stride);
    
    if (rgb) {
        free(rgb);
    }
    return ok;
    
}

static int picture_read(const char* const filename, WebPPicture* const pic) {
    int ok = 0;
    FILE* in_file = fopen(filename, "rb");
    if (in_file == NULL) {
        fprintf(stderr, "Error! Cannot open input file '%s'\n", filename);
        return ok;
    }
    
    ok = jpeg_read(in_file, pic);
    if (!ok) {
        fprintf(stderr, "Error! Could not process file %s\n", filename);
    }
    
    fclose(in_file);
    return ok;
}

static inline void _webp_free(WebPPicture* picture, FILE *out)
{
    free(picture->extra_info);
    WebPPictureFree(picture);
    if (out != NULL) fclose(out);
}

static int Writer(const uint8_t* data, size_t data_size,
                  const WebPPicture* const pic) {
    FILE* const out = (FILE*)pic->custom_ptr;
    return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}



int jpeg2webp(char *in_file, char* out_file) {
    int quality=80;
    int method=-1;
    FILE *output=NULL;
    WebPPicture picture;
    WebPConfig config;
    
    if (!WebPPictureInit(&picture) || !WebPConfigInit(&config)) {
        fprintf(stderr, "initialized error\n");
        _webp_free(&picture, output);
        return 1;
    }
    
    if (!picture_read(in_file, &picture)) {
        _webp_free(&picture, output);
        return 1;
    }
    
    output = fopen(out_file, "wb");
    if (output == NULL) {
        fprintf(stderr, "fopen(out) error!!\n");
        _webp_free(&picture, output);
        return 1;
    }
    picture.writer = Writer;
    picture.custom_ptr = (void *)output;
    
    config.quality = quality;
    if (0 <= method && method <= 6)
        config.method = method;
    
    if (!WebPEncode(&config, &picture)) {
        fprintf(stderr, "WebPEncode() error!!\n");
        _webp_free(&picture, output);
        return 1;
    }
    
    return 0;
}

int main() {
    return jpeg2webp("/Users/jeff/Downloads/a.jpg", "/Users/jeff/Downloads/b.webp");
}






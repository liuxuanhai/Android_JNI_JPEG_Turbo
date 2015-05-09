#ifndef __JPEG_TURBO_WRAPPER__
#define __JPEG_TURBO_WRAPPER__

namespace jpegturbo
{
	extern int compress_image_to_jpeg_file(const char* filename, const int width, const int height, const int quality, const unsigned char* rgba_pixels);

	extern unsigned char* decompress_jpeg_image_from_memory(const unsigned char* jpegData, const int jpegDataSize, int *out_width, int* out_height, int* out_components, const int required_components);

	extern int compress_image_to_jpeg_memory(unsigned char** output, const int width, const int height, const int quality, const unsigned char* rgba_buffer);
}

#endif /* __JPEG_TURBO_WRAPPER__ */


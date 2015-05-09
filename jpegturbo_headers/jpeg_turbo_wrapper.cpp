#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <vector>
#include "main/headers/common.h"
#include "jpeg/jpegturbo/jpeg_turbo_wrapper.h"
#include "jpeg/jpegturbo/jpeglib.h"

namespace jpegturbo
{

//------------------------------------------------------------------------------------------------------------------

typedef struct _error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf jb;
} error_mgr;

//------------------------------------------------------------------------------------------------------------------

int compress_image_to_jpeg_file(const char* filename, const int width, const int height, const int quality, const unsigned char* rgba_buffer)
{
	FILE* outfile = NULL;

	/* Open output file */
	if ((outfile = fopen(filename, "wb")) == NULL)
	{
		return -1;
	}

	const int bpp = 4;
	struct jpeg_compress_struct cinfo;

	// Set up the error handler first, in case the initialization step fails. (Unlikely, but it could happen if you are out of memory.)
	// This routine fills in the contents of struct jerr, and returns jerr's address which we place into the link field in cinfo.
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	// Create JPEG compression object
	jpeg_create_compress(&cinfo);

	// Specify data destination (output file)
	jpeg_stdio_dest(&cinfo, outfile);

	// Set parameters for compression
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = bpp;
	cinfo.in_color_space = JCS_EXT_RGBA;
	cinfo.data_precision = 8;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	// Start compressor
	jpeg_start_compress(&cinfo, TRUE);

	// Set up row pointers
	const int row_stride = cinfo.image_width * bpp;

	while (cinfo.next_scanline < cinfo.image_height)
	{
		// jpeg_write_scanlines expects an array of pointers to scan-lines. Here the array is only one
		// element long, but you could pass more than one scan-line at a time if that's more convenient

		const unsigned char *row = &rgba_buffer[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, const_cast<unsigned char**>(&row), 1);
	}

	// Finish compression
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	// Close output file
	fclose(outfile);

	return 0;
}

//------------------------------------------------------------------------------------------------------------------

int __compress_image_to_jpeg_file(const char* filename, const int width, const int height, const int quality, const unsigned char* rgba_buffer)
{
	FILE* outfile = NULL;

	/* Open output file */
	if ((outfile = fopen(filename, "wb")) == NULL)
	{
		return -1;
	}

	struct jpeg_compress_struct cinfo;

	// Set up the error handler first, in case the initialization step fails. (Unlikely, but it could happen if you are out of memory.)
	// This routine fills in the contents of struct jerr, and returns jerr's address which we place into the link field in cinfo.
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	// Create JPEG compression object
	jpeg_create_compress(&cinfo);

	// Specify data destination (output file)
	jpeg_stdio_dest(&cinfo, outfile);

	const int rgb_bpp  = 3;
	const int rgba_bpp = 4;

	// Set parameters for compression
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = rgb_bpp;
	cinfo.in_color_space = JCS_RGB;
	cinfo.data_precision = 8;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	// Start compressor
	jpeg_start_compress(&cinfo, TRUE);

	// Set up row pointers
	const int row_stride_rgb  = cinfo.image_width * rgb_bpp;
	const int row_stride_rgba = cinfo.image_width * rgba_bpp;

	unsigned char *row_rgb = new unsigned char[row_stride_rgb];

	while (cinfo.next_scanline < cinfo.image_height)
	{
		const unsigned char *row_rgba = &rgba_buffer[cinfo.next_scanline * row_stride_rgba];

		// Strip alpha value

		for (int x = 0; x < cinfo.image_width; x++)
		{
			const unsigned char* pixel_in = &row_rgba[x * rgba_bpp];
			unsigned char* pixel_out = &row_rgb[x * rgb_bpp];
			pixel_out[0] = pixel_in[0];
			pixel_out[1] = pixel_in[1];
			pixel_out[2] = pixel_in[2];
		}

		// Process line

		jpeg_write_scanlines(&cinfo, &row_rgb, 1);
	}

	delete[] row_rgb;
	row_rgb = NULL;

	// Finish compression
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	// Close output file
	fclose(outfile);

	return 0;
}

//------------------------------------------------------------------------------------------------------------------

typedef struct
{
	struct jpeg_source_mgr pub;   // public fields

	JOCTET * buffer;              // start of buffer
	boolean start_of_file;        // have we gotten any data yet?
} source_mgr;

typedef source_mgr * src_ptr;

static void jpg_memInitSource(j_decompress_ptr cinfo)
{
  src_ptr src = (src_ptr) cinfo->src;
  src->start_of_file = TRUE;
}

static boolean jpg_memFillInputBuffer(j_decompress_ptr cinfo)
{
	src_ptr src = (src_ptr) cinfo->src;
	src->start_of_file = FALSE;
	return TRUE;
}

static void jpg_memSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
	src_ptr src = (src_ptr) cinfo->src;

	if (num_bytes > 0)
	{
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

static void jpg_memTermSource(j_decompress_ptr cinfo)
{
	// no work necessary here
}

/**
 * Image decompressor
 */
unsigned char* decompress_jpeg_image_from_memory(const unsigned char* jpegData, const int jpegDataSize, int* out_width, int* out_height, int* out_components, const int required_components)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	src_ptr src;

	jpeg_create_decompress(&cinfo);

	cinfo.src = (struct jpeg_source_mgr*)(*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT, sizeof(source_mgr));
	src = (src_ptr) cinfo.src;
	src->buffer = (JOCTET *)jpegData;

	src->pub.init_source = jpg_memInitSource;
	src->pub.fill_input_buffer = jpg_memFillInputBuffer;
	src->pub.skip_input_data = jpg_memSkipInputData;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = jpg_memTermSource;
	src->pub.bytes_in_buffer = jpegDataSize;
	src->pub.next_input_byte = jpegData;

	jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = required_components == 4 ? JCS_EXT_RGBA : JCS_RGB;
	jpeg_start_decompress(&cinfo);

	// Read all scan-lines

	const int width  = (int)cinfo.output_width;
	const int height = (int)cinfo.output_height;
	*out_width  = width;
	*out_height = height;
	*out_components = (int)cinfo.output_components;

	const unsigned int buffer_size = ((width * height) * cinfo.output_components) + 8; // +8 see: http://catid.mechafetus.com/news/news.php?view=363
	unsigned char *raw_output_image = new unsigned char[buffer_size];

	const unsigned int scanline_len = cinfo.output_width * cinfo.output_components;
	unsigned int scanline_count = 0;
	JSAMPROW output_data;

	while (cinfo.output_scanline < height)
	{
		output_data = (raw_output_image + (scanline_count * scanline_len));
		jpeg_read_scanlines(&cinfo, &output_data, 1);
		scanline_count++;
	}

	// Release JPEG compression object

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return raw_output_image;
}

//-----------------------------------------------------------------------------------------------------------------------------

typedef struct _jpeg_destination_mem_mgr
{
    jpeg_destination_mgr mgr;
    std::vector<unsigned char> data;
} jpeg_destination_mem_mgr;

//-----------------------------------------------------------------------------------------------------------------------------

#define JPEG_MEM_DST_MGR_BUFFER_SIZE (8 * (1 << 10))

static void mem_init_destination(j_compress_ptr cinfo)
{
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    dst->data.resize(JPEG_MEM_DST_MGR_BUFFER_SIZE);
    cinfo->dest->next_output_byte = (dst->data.empty() == true) ? 0 : &dst->data.front();
    cinfo->dest->free_in_buffer = dst->data.size();
}

//-----------------------------------------------------------------------------------------------------------------------------

static void mem_term_destination(j_compress_ptr cinfo)
{
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    dst->data.resize(dst->data.size() - cinfo->dest->free_in_buffer);
}

//-----------------------------------------------------------------------------------------------------------------------------

static boolean mem_empty_output_buffer(j_compress_ptr cinfo)
{
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    size_t oldsize = dst->data.size();

    dst->data.resize(oldsize + JPEG_MEM_DST_MGR_BUFFER_SIZE);
    cinfo->dest->next_output_byte = ((dst->data.empty() == true) ? 0 : &dst->data.front()) + oldsize;
    cinfo->dest->free_in_buffer = JPEG_MEM_DST_MGR_BUFFER_SIZE;

    return true;
}

//-----------------------------------------------------------------------------------------------------------------------------

static void jpeg_mem_dest(j_compress_ptr cinfo, jpeg_destination_mem_mgr * dst)
{
    cinfo->dest = (jpeg_destination_mgr*)dst;
    cinfo->dest->init_destination = mem_init_destination;
    cinfo->dest->term_destination = mem_term_destination;
    cinfo->dest->empty_output_buffer = mem_empty_output_buffer;
}

//-----------------------------------------------------------------------------------------------------------------------------

int compress_image_to_jpeg_memory(unsigned char** output, const int width, const int height, const int quality, const unsigned char* rgba_buffer)
{
	const int bpp = 4;
	struct jpeg_compress_struct cinfo;

	// Set up the error handler first, in case the initialization step fails. (Unlikely, but it could happen if you are out of memory.)
	// This routine fills in the contents of struct jerr, and returns jerr's address which we place into the link field in cinfo.

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	// Create JPEG compression object

	jpeg_create_compress(&cinfo);

	// Set parameters for compression

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = bpp;
	cinfo.in_color_space = JCS_EXT_RGBA;
	cinfo.data_precision = 8;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	// Set decompression manager

	jpeg_destination_mem_mgr dst_mem;
	jpeg_mem_dest(&cinfo, &dst_mem);

	// Start compressor

	jpeg_start_compress(&cinfo, TRUE);

	// Set up row pointers

	const int row_stride = cinfo.image_width * bpp;

	while (cinfo.next_scanline < cinfo.image_height)
	{
		// jpeg_write_scanlines expects an array of pointers to scan-lines. Here the array is only one
		// element long, but you could pass more than one scan-line at a time if that's more convenient

		const unsigned char *row = &rgba_buffer[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, const_cast<unsigned char**>(&row), 1);
	}

	// Finish compression

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	// Set the output data

	const size_t outputSize = dst_mem.data.size();
	*output = new unsigned char[outputSize];

	memcpy(*output, &dst_mem.data.front(), outputSize);
	dst_mem.data.clear();

	return outputSize;
}

//-----------------------------------------------------------------------------------------------------------------------------

} /* end namespace */

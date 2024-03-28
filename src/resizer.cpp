/***************************************************************************\
    resizer.cpp - Image resizer using libgd, using pre-scaled images
		  from libjpeg/libpng to be faster and use less memory.

    Copyright (C) 2008 piespy@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\**************************************************************************/

#include <gd.h>
#include <memory>

#include <iqdb/debug.h>
#include <iqdb/imgdb.h>
#include <iqdb/resizer.h>

namespace iqdb {

enum image_types { IMG_UNKNOWN, IMG_JPEG };

image_types get_image_info(const unsigned char *data, size_t length) {
  if (length >= 2 && data[0] == 0xff && data[1] == 0xd8) {
    return IMG_JPEG;
  } else {
    return IMG_UNKNOWN;
  }
}

RawImage resize_image_data(const unsigned char *data, size_t len, unsigned int thu_x, unsigned int thu_y) {
  auto type = get_image_info(data, len);

  if (type != IMG_JPEG) {
    throw image_error("unsupported image format (only JPG is supported)");
  }

  RawImage thu(gdImageCreateTrueColor(thu_x, thu_y), &gdImageDestroy);
  if (!thu) {
    throw image_error("failed to run gdImageCreateTrueColor: out of memory");
  }

  RawImage img(gdImageCreateFromJpegPtr((int)len, const_cast<unsigned char *>(data)), &gdImageDestroy);
  if (!img) {
    throw image_error("failed to run gdImageCreateFromJpegPtr: could not read image");
  }

  if ((unsigned int)img->sx == thu_x && (unsigned int)img->sy == thu_y && gdImageTrueColor(img)) {
    return img;
  }

  gdImageCopyResampled(thu.get(), img.get(), 0, 0, 0, 0, thu_x, thu_y, img->sx, img->sy);
  DEBUG("resized {}x{} to {}x{}\n", img->sx, img->sy, thu_x, thu_y);

  return thu;
}

}

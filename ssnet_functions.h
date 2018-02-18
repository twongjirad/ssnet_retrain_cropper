#include <iostream>
#include <vector>

// larcv
#include "DataFormat/ImageMeta.h"
#include "DataFormat/Image2D.h"
#include "DataFormat/ROI.h"

// larlite
#include "DataFormat/mctrack.h"
#include "DataFormat/mcshower.h"

// function to generate cropped regions
std::vector< larcv::ROI > generate_regions( const int rows, const int cols,
					    const larcv::ImageMeta& sourceimg_meta, const std::vector<larcv::Image2D>& src_v,
					    const int num_regions, const std::vector<float>& min_occupany_fraction, const std::vector<float>& thresholds,
					    const int maxattempts, const int randseed );

std::vector<larcv::Image2D> make_cropped_label_image( const std::vector<larcv::Image2D>& croppedimgs, const std::vector<larcv::Image2D>& idimgs,
						      const std::vector<larlite::mctrack>& mctrack, const std::vector<larlite::mcshower>& mcshower,
						      const float adcthreshold );


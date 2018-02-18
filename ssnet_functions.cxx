#include "ssnet_functions.h"
#include "TRandom3.h"

// larlite
#include "LArUtil/Geometry.h"
#include "LArUtil/LArProperties.h"

// function to generate cropped regions
std::vector< larcv::ROI > generate_regions( const int rows, const int cols,
					    const larcv::ImageMeta& sourceimg_meta, const std::vector<larcv::Image2D>& src_v,
					    const int num_regions, const std::vector<float>& min_occupancy_fraction, const std::vector<float>& thresholds,
					    const int maxattempts, const int randseed ) {

  // we generate random positions in the detector
  // we accept only if there is a minimum occupany in the image
  // or a max attempt limit is reached

  std::vector< larcv::ROI > roi_v;
  
  std::vector< std::vector<float> > vertices;

  int seed = randseed;
  if ( randseed<0 )
    seed = int(std::time(NULL));

  // determine limits based on row,col boundaries
  float minz = 0.3*(0.5*cols); 
  float maxz = 1036.0 - minz;
  const larutil::Geometry* geo = larutil::Geometry::GetME();
  const larutil::LArProperties* larp = larutil::LArProperties::GetME();

  std::cout << "defining crop regions for source image with: (" << sourceimg_meta.rows() << "," << sourceimg_meta.cols() << ")" << std::endl;
  
  TRandom3 rand( seed );
  int numattempts = 0;
  while ( numattempts<maxattempts && roi_v.size()<num_regions ) {

    // generate random position
    Double_t pos[3];
    pos[0] = rand.Uniform()*256.0;
    pos[1] = -117.0 + 2*117.0*rand.Uniform();
    pos[2] = minz + (maxz-minz)*rand.Uniform();
    
    // define regions in the planes
    std::vector< std::vector<int> > colranges;
    for (int p=0; p<3; p++) {
      float centerwire = geo->WireCoordinate( pos, p );
      float minwire = centerwire-0.5*cols*sourceimg_meta.pixel_width();
      float maxwire = centerwire+0.5*cols*sourceimg_meta.pixel_width();

      if ( maxwire>sourceimg_meta.max_x() ) {
	float dwire = maxwire-sourceimg_meta.max_x()+1;
	maxwire -= dwire;
	minwire -= dwire;
      }
      else if ( minwire<sourceimg_meta.min_x() ) {
	float dwire = sourceimg_meta.min_x()-minwire+1;
	maxwire += dwire;
	minwire += dwire;
      }

      std::vector<int> range(2);
      range[0] = sourceimg_meta.col(minwire);
      range[1] = sourceimg_meta.col(maxwire);
      if ( range[1]-range[0]<cols )
	range[1] = range[0]+cols;
      colranges.push_back( range );
    }

    // set the time bounds
    std::vector<int> rowrange(2);
    float centertick = 3200.0 + pos[0]/(larp->DriftVelocity()*0.5);
    float mintick = centertick - 0.5*rows*sourceimg_meta.pixel_height();
    float maxtick = centertick + 0.5*rows*sourceimg_meta.pixel_height();
    if ( maxtick > sourceimg_meta.max_y() ) {
      float dtick = maxtick - sourceimg_meta.max_y() + 1;
      maxtick -= dtick;
      mintick -= dtick;
    }
    else if ( mintick < sourceimg_meta.min_y() ) {
      float dtick = sourceimg_meta.min_y() - mintick + 1;
      maxtick += dtick;
      mintick += dtick;
    }
    rowrange[0] = sourceimg_meta.row( maxtick );
    rowrange[1] = sourceimg_meta.row( mintick );
    if ( rowrange[1]-rowrange[0]!=rows )
      rowrange[1] = rowrange[0]+rows;

    // generate ranges, now we determine if requirements are satisfied
    int planes_passing = 0;
    std::vector<float> occfrac(3,0);
    for (int p=0; p<3; p++) {
      const larcv::Image2D& src = src_v[p];
      int abovethresh = 0;
      for (int r=rowrange[0]; r<rowrange[1]; r++) {
	for (int c=colranges[p][0]; c<colranges[p][1]; c++) {
	  if ( src.pixel(r,c)>thresholds[p] )
	    abovethresh++;
	}
      }
      occfrac[p] = float(abovethresh)/float(rows*cols);
      if (  occfrac[p] > min_occupancy_fraction[p] )
	planes_passing++;
    }
    
    if ( planes_passing==src_v.size() ) {
      // create bounding boxes and ROI
      larcv::ROI roi;
      for ( int p=0; p<(int)src_v.size(); p++ ) {

	larcv::ImageMeta meta( cols*sourceimg_meta.pixel_width(), rows*sourceimg_meta.pixel_height(),
			       rows, cols,
			       sourceimg_meta.pos_x( colranges[p][0] ), sourceimg_meta.pos_y( rowrange[0] ),
			       src_v[p].meta().plane() );

	roi.AppendBB( meta );
      }
      
      roi_v.emplace_back( std::move(roi) );
    }
    numattempts++;
    std::cout << "attempt " << numattempts << " planes_passing=" << planes_passing << std::endl;
    for (int p=0; p<3; p++)
      std::cout << "  occupancy plane " << p << ": " << occfrac[p] << std::endl;
    
  }//end of attempt loop

  return roi_v;
}


std::vector<larcv::Image2D> make_cropped_label_image( const std::vector<larcv::Image2D>& croppedimgs, const std::vector<larcv::Image2D>& idimgs,
						      const std::vector<larlite::mctrack>& mctrack, const std::vector<larlite::mcshower>& mcshower,
						      const float adcthreshold ) {
  // loop over mctrack and shower and collect IDs
  // loop over cropped image range and label above threshold pixels
  // for pixels with IDs not in the mcshower nor mctrack, it's almost certainly a low energy gamma, so label as shower
  //    might check neighbors and if track, then isolated shower is probably not a good idea for label, so change
  // for pixels with no id, but above threshold, check nearest pixel in time (probably due to deconv)
}


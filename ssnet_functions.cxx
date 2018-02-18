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


void make_cropped_label_image( const std::vector<larcv::Image2D>& croppedimgs,
			       const std::vector<larcv::Image2D>& idimgs, const std::vector<larcv::Image2D>& momimgs,
			       const std::vector<larlite::mctrack>& mctracks, const std::vector<larlite::mcshower>& mcshowers,
			       const float adcthreshold,
			       std::vector<larcv::Image2D>& labelimg_v, std::vector<larcv::Image2D>& weightimg_v ) {
  
  // loop over mctrack and shower and collect IDs
  // loop over cropped image range and label above threshold pixels
  // for pixels with IDs not in the mcshower nor mctrack, it's almost certainly a low energy gamma, so label as shower
  //    might check neighbors and if track, then isolated shower is probably not a good idea for label, so change
  // for pixels with no id, but above threshold, check nearest pixel in time (probably due to deconv)

  std::set<int> showerids;
  std::set<int> trackids;
  for ( auto const& track : mctracks ) {
    trackids.insert( track.TrackID() );
  }
  for ( auto const& shower : mcshowers ) {
    showerids.insert( shower.TrackID() );
  }

  labelimg_v.clear();
  weightimg_v.clear();
  
  // loop over the planes
  for (int p=0; p<3; p++) {
    
    // make output weight and label image
    larcv::Image2D label( croppedimgs.at(p).meta() );
    label.paint(0);

    int numshowerpix = 0;
    int numtrackpix  = 0;
    int numnoisepix  = 0;

    larcv::Image2D weight( croppedimgs.at(p).meta() );
    weight.paint(0);
    
    // get the adc image for the plane. the container stores all three planes. we grab plane p.
    const larcv::Image2D& adcimg    = croppedimgs.at(p);
    const larcv::ImageMeta& adcmeta = adcimg.meta();

    // image where pixels contain track ID of particle responsible for largest energy deposition
    const larcv::Image2D& idimg    = idimgs.at(p);      
    const larcv::Image2D& momimg   = momimgs.at(p);
    const larcv::ImageMeta& idmeta = idimg.meta();

    std::map< int, int > idcounter;
    std::map< int, int > idlabel;
    std::map< int, int > idmom;    

    // loop over rows and columns of the cropped adc image, as it is a subset
    for (size_t radc=0; radc<adcmeta.rows(); radc++) {

      // we have to translate to the bigger image as well
      // we get the absolute coordinate value (tick,wire)
      float tick = adcmeta.pos_y( radc );

      // then go and get the row,col in the full adc image
      int rid = idmeta.row(tick);
      
      for (size_t cadc=0; cadc<adcmeta.cols(); cadc++) {
	// same thing for the x-axis (wires)
	float wire = adcmeta.pos_x( cadc );
	int cid    = idmeta.col(wire);	  
	
	// get the adc value
	float adc = adcimg.pixel(radc,cadc);

	// if below threshold, skip, not interesting
	if ( adc<adcthreshold )
	  continue;

	// get the track id
	int id  = (int)idimg.pixel(rid,cid);
	int mom = (int)momimg.pixel(rid,cid);

	if ( idcounter.find(id)==idcounter.end() ) {
	  idcounter.insert( std::pair<int,int>(id,0) );
	  idlabel.insert( std::pair<int,int>(id,-1) );
	  idmom.insert( std::pair<int,int>(id,-1) );	  
	}
	idcounter[id]++;
	idmom[id] = mom;

	
	bool istrack = false;
	bool isshower = false;
	if ( trackids.find(id)!=trackids.end() ) {
	  istrack = true;
	}
	if ( showerids.find(id)!=showerids.end() ) {
	  isshower = true;
	}

	// if ( id!=-1 && !istrack && !isshower ) {
	//   isshower = true; // above threshold, ID-less energy deposition. probably shower
	// }

	if ( istrack ) {
	  label.set_pixel( radc, cadc, 2.0 );
	  idlabel[id] = 2;
	  numtrackpix++;
	}
	else if (isshower ) {
	  label.set_pixel( radc, cadc, 1.0 );
	  idlabel[id] = 1;
	  numshowerpix++;
	}
	else {

	  if ( id>=0 && mom>0 ) {
	    label.set_pixel( radc, cadc, 2.0 );
	    numtrackpix++;	    
	    idlabel[id] = 2;
	  }
	  else if ( id>=0 && mom<0 ) {
	    label.set_pixel( radc, cadc, 1.0 );
	    numshowerpix++;	    
	    idlabel[id] = 1;
	  }
	  else {
	    label.set_pixel( radc, cadc, 3.0 );
	    idlabel[id] = 3;
	    numnoisepix++;
	  }
	}
	     
	
      }//end of col loop
    }//end of row loop

    std::cout << "[ID Count]" << std::endl;
    for ( auto &it : idcounter ) {
      std::cout << "  (" << it.first << ") " << it.second << " label=" << idlabel[it.first] << " ancestorid=" << idmom[it.first] << std::endl;
    }

    /// weight image. we go 1/(numx)

    int numbg = adcmeta.rows()*adcmeta.cols() - numshowerpix - numtrackpix;
    float trackweight  = 1.0;
    float showerweight = 1.0;
    float bgweight     = 1.0;
    float noiseweight  = 1.0;
    if ( numshowerpix>0 )
      showerweight = 1.0/float(numshowerpix);
    if ( numtrackpix>0 )
      trackweight = 1.0/float(numtrackpix);
    if ( numbg>0 )
      bgweight = 1.0/float(numbg);
    if ( numnoisepix>0 )
      noiseweight = 1.0/float(numnoisepix);
    
    for (size_t radc=0; radc<adcmeta.rows(); radc++) {
      for (size_t cadc=0; cadc<adcmeta.cols(); cadc++) {
	int lbl = label.pixel(radc,cadc);
	if ( lbl==1.0 )
	  weight.set_pixel( radc, cadc, showerweight );
	else if ( lbl==2.0 )
	  weight.set_pixel( radc, cadc, trackweight );
	else if (lbl==3.0 )
	  weight.set_pixel( radc, cadc, noiseweight );
	else
	  weight.set_pixel( radc, cadc, bgweight );
      }
    }

    labelimg_v.emplace_back(  std::move(label)  );
    weightimg_v.emplace_back( std::move(weight) );
    
  }// end of plane loop


}


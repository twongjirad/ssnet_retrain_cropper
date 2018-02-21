#include <iostream>
#include <string>

// larcv
#include "DataFormat/Image2D.h"
#include "DataFormat/ROI.h"

// llcv
#include "Base/DataCoordinator.h"

// llcvp
#include "LLCVBase/Processor.h"
#include "InterTool_App/InterModule.h"
#include "InterTool_Sel_FlashMatch/InterSelFlashMatch.h"

// opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
// larcv opencv utils
#include "CVUtil/CVUtil.h"

#include "ssnet_functions.h"

int main( int nargs, char** argv ) {

  std::cout << "[ Select Tune Sample ]" << std::endl;

  std::string cfg        = argv[1];
  std::string INTER_FILE = argv[2];
  std::string SSNET_FILE = argv[3];
  std::string PGRPH_FILE = argv[4];
  std::string SHR_FILE   = argv[5];
  std::string TRK_FILE   = argv[6];
  std::string FLSH_FILE  = argv[7];
  std::string SUP_FILE   = argv[8];  
  std::string MC_FILE    = argv[9];
  //std::string HIT_FILE  = argv[7];

  // holds images and mcinfo trees per event
  larlitecv::DataCoordinator dataco;
  dataco.add_inputfile( MC_FILE,  "larlite" );
  dataco.add_inputfile( SUP_FILE, "larcv" );  
  dataco.initialize();

  // output for larcv information
  larcv::IOManager outlarcv( larcv::IOManager::kWRITE );
  outlarcv.set_out_file( "baka_larcv.root" );
  outlarcv.initialize();
  
  llcv::Processor llcv_proc;
  llcv::InterModule llcvmod;

  llcv::InterSelFlashMatch flashmatchana;

  llcv::InterDriver& driver = llcvmod.Driver();
  //driver.AttachInterFile(INTER_FILE,"vertex_tree");
  driver.SetOutputFilename("aho_interfile.root");
  //driver.AddSelection(&flashmatchana);

  llcv_proc.add_llcv_ana(&llcvmod);

  llcv_proc.configure( cfg );

  llcv_proc.set_output_lcv_name( "aho_larcv.root" );
  llcv_proc.set_output_ll_name(  "aho_larlite.root" );

  llcv_proc.add_lcv_input_file(SSNET_FILE);
  //llcv_proc.add_lcv_input_file(PGRPH_FILE);
  llcv_proc.add_ll_input_file(SHR_FILE);
  llcv_proc.add_ll_input_file(TRK_FILE);
  llcv_proc.add_ll_input_file(FLSH_FILE);
  //llcv_proc.add_ll_input_file(HIT_FILE);

  llcv_proc.initialize();

  int nentries = llcv_proc.get_n_ll_entries();
  nentries = 1;
  
  for ( int i=0; i<nentries; i++) {
    llcv_proc.batch_process_ll(i,1);
    std::cout << "entry " << i << std::endl;

    // get rse
    int run    = llcv_proc.dataco().run();
    int subrun = llcv_proc.dataco().subrun();
    int event  = llcv_proc.dataco().event();

    // sync up with supera/mcinfo
    dataco.goto_event( run, subrun, event, "larlite" );

    // get event vertex vector
    auto & vtxdata = driver.GetEventVertexData();
    auto & imgdata = driver.GetEventImageData();
    std::cout << "  number of vertices in event: " << vtxdata.size() << std::endl;

    // get data
    larlitecv::DataCoordinator& inputdataco = llcv_proc.dataco();    

    // adc images (event-wise)
    larcv::EventImage2D* ev_img_v = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "wire" );

    // instance ID images
    larcv::EventImage2D* ev_id_v  = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "instance" );
    larcv::EventImage2D* ev_mom_v = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "ancestor" );

    // mctrack/mcshower info
    larlite::event_mctrack* ev_tracks   = (larlite::event_mctrack*)dataco.get_larlite_data( larlite::data::kMCTrack, "mcreco" );
    larlite::event_mcshower* ev_showers = (larlite::event_mcshower*)dataco.get_larlite_data( larlite::data::kMCShower, "mcreco" );    

    const std::vector<larcv::Image2D>& img_v = ev_img_v->Image2DArray();

    if ( img_v.size()==0 )
      continue;
    
    std::vector<float> thresholds(3,10.0);
    std::vector<float> occupancy(3,0.01);
    occupancy[2] = 0.002;
    std::vector<larcv::ROI> roi_v = generate_regions( 512, 512, img_v.front().meta(), img_v, 10, occupancy, thresholds, 100, -1 );

    std::cout << "Number of ROIs returned: " << roi_v.size() << std::endl;

    // crop and save
    int nroi = roi_v.size();
    if ( nroi>10 )
      nroi = 10;
    for ( int iroi=0; iroi<nroi; iroi++) {

      // set the output event container
      larcv::EventImage2D* ev_out    = (larcv::EventImage2D*)outlarcv.get_data( larcv::kProductImage2D, "adc" );
      larcv::EventImage2D* ev_label  = (larcv::EventImage2D*)outlarcv.get_data( larcv::kProductImage2D, "label" );
      larcv::EventImage2D* ev_weight = (larcv::EventImage2D*)outlarcv.get_data( larcv::kProductImage2D, "weight" );      

      larcv::ROI& roi = roi_v.at(iroi);

      std::cout << roi.dump() << std::endl;
      std::cout << "pixelwidth: " << roi.BB(0).pixel_width() << " " << roi.BB(0).pixel_height() << std::endl;
      
      // crop the image
      for ( auto const& img : img_v ) {

	larcv::Image2D cropped = img.crop( roi.BB( img.meta().plane() ) );	
	ev_out->Emplace( std::move(cropped) );
      }

      std::vector<larcv::Image2D> label_v;
      std::vector<larcv::Image2D> weight_v;      
      make_cropped_label_image( ev_out->Image2DArray(), ev_id_v->Image2DArray(), ev_mom_v->Image2DArray(),
				*ev_tracks, *ev_showers, 1.0, label_v, weight_v );
				
      ev_label->Emplace( std::move(label_v) );
      ev_weight->Emplace( std::move( weight_v) );
      
      // set the entry number
      outlarcv.set_id( run, subrun, event*10 + iroi );

      outlarcv.save_entry();
      
    }
    
  }

  outlarcv.finalize();
  llcv_proc.finalize();

  return 0;
}

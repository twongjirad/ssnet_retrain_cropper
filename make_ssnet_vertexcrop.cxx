#include <iostream>
#include <string>

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
  
  
  llcv::Processor llcv_proc;
  llcv::InterModule llcvmod;

  llcv::InterSelFlashMatch flashmatchana;

  llcv::InterDriver& driver = llcvmod.Driver();
  driver.AttachInterFile(INTER_FILE,"vertex_tree");
  driver.SetOutputFilename("aho_interfile.root");
  //driver.AddSelection(&flashmatchana);

  llcv_proc.add_llcv_ana(&llcvmod);

  llcv_proc.configure( cfg );

  llcv_proc.set_output_lcv_name( "aho_larcv.root" );
  llcv_proc.set_output_ll_name(  "aho_larlite.root" );

  llcv_proc.add_lcv_input_file(SSNET_FILE);
  llcv_proc.add_lcv_input_file(PGRPH_FILE);
  llcv_proc.add_ll_input_file(SHR_FILE);
  llcv_proc.add_ll_input_file(TRK_FILE);
  llcv_proc.add_ll_input_file(FLSH_FILE);
  //llcv_proc.add_ll_input_file(HIT_FILE);

  llcv_proc.initialize();

  int nentries = llcv_proc.get_n_ll_entries();

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
    auto const& vtxdata = driver.GetEventVertexData();
    std::cout << "  number of vertices in event: " << vtxdata.size() << std::endl;

    // operations
    
  }

  
  llcv_proc.finalize();

  return 0;
}

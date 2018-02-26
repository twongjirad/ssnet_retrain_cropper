import os,sys,commands

cfg = "inter_tool_test.cfg"

def genFileDict( run, subrun ):
    fid = run*10000+subrun
    f = {"ssnet":data_folder+"/ssnetout-larcv-Run%06d-SubRun%06d.root"%(run,subrun),
         "opreco":data_folder+"/opreco-Run%06d-SubRun%06d.root"%(run,subrun),
         "reco2d":data_folder+"/reco2d-Run%06d-SubRun%06d.root"%(run,subrun),
         "supera":data_folder+"/supera-Run%06d-SubRun%06d.root"%(run,subrun),
         "mcinfo":data_folder+"/mcinfo-Run%06d-SubRun%06d.root"%(run,subrun),         
         "shower":data_folder+"/shower_reco_out_%d.root"%(fid),
         "pgraph":data_folder+"/vertexout_filter_nue_ana_tree_%d.root"%(fid),
         "tracker":data_folder+"/tracker_reco_%d.root"%(fid) }
    return f

#data_folder = "/mnt/raid0/larbys/dbtestfiles/run5925subrun0195/"
#inter       = "/mnt/raid0/larbys/final_files/test6/mcc8v6_bnb5e19_test6_inter.root"

#data_folder = "/mnt/raid0/larbys/dbtestfiles/corsikaex/"
#inter       = "/mnt/raid0/larbys/final_files/test6/corsika_mcc8v3_p00_test6_inter.root"

#data_folder = "/mnt/raid0/larbys/dbtestfiles/cocktail_run1_subrun300/"
#inter       = "/mnt/raid0/larbys/final_files/test6/mcc8v4_cocktail_p00_test6_inter.root"

data_folder = "/home/twongjirad/working/data/larbys/mcc8v6/testdb/bnb5e19/run5925subrun0195"

#ssnet   = data_file+"/goodreco_1mu1p_ssnet.root"
#tracker = data_file+"/goodreco_1mu1p_track.root"
#opreco  = data_file+"/goodreco_1mu1p_opreco.root"
#shower  = data_file+"/goodreco_1mu1p_shower.root"
#pgraph  = data_file+"/goodreco_1mu1p_pgraph.root"
#fdict = genFileDict(5925,195)
#fdict = genFileDict(1,245)
#fdict = genFileDict(1,300)
fdict = genFileDict(5925,195)

# MC
args = "%s %s"%(fdict["supera"],"baka_larcv.root")

print args
os.system("./bin/make_ssnet_vertexcrop_data %s"%(args))





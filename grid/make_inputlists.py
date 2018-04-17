import os,sys

# FILE LISTS COME FROM PUBS DB TOOLS
# specifically, pubs/dlleepubs/utils/dump_
# ------------------------------------------------------------------------

#flists = ["corsika_p00.txt","corsika_p01.txt", "corsika_p02.txt"]
flists = ["filelists/cocktail_mcc8v4/dblist_mcc8v4_cocktail_p00.txt",
          "filelists/cocktail_mcc8v4/dblist_mcc8v4_cocktail_p01.txt",
          "filelists/cocktail_mcc8v4/dblist_mcc8v4_cocktail_p02.txt",
          "filelists/cocktail_mcc8v4/dblist_mcc8v4_cocktail_p03.txt",
          "filelists/cocktail_mcc8v4/dblist_mcc8v4_cocktail_p04.txt"]
os.system("mkdir inputlists")

joblist = open("joblist.txt",'w')

njobs = 0
for f in flists:
    fin = open(f,'r')
    fls = fin.readlines()
    for l in fls:
        l = l.strip()
        i = l.split()
        run = int(i[0])
        subrun = int(i[1])
        jobid = 10000*run + subrun
        print >> joblist,jobid
        infile = open('inputlists/input_%d.txt'%(jobid),'w')
        print >> infile,i[2]
        print >> infile,i[3]
        infile.close()
        njobs+=1
print "number of jobs: ",njobs
joblist.close()
        
        



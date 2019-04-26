[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/dashboard/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)

asdf

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert /path/to/README.md-->
<!--ts-->
   * [QuickStart](#quickstart)
      * [Requirements](#requirements)
      * [Setup](#setup)
         * [Environment loading](#environment-loading)
      * [Execution](#execution)
         * [Basic workflow](#basic-workflow)
         * [Readout chain](#readout-chain)


<!-- Added by: bvonhall, at:  -->

<!--te-->

# QuickStart for After Logging into aliceits@svmithi02 

## Enviornment Setup

Open the working directory: `cd /data/ITSQCq`

Loading the working environment: `source setQCenv.sh`


## Installing QC with Docker (If you want a new copy of the QC codes on svmithi02)


Creating the new folder for QC: `mkdir QC`     

`cd QC`    


Set the software path to be in the current folder: `export ALIBUILD_WORK_DIR="$PWD/sw"`       


Obtain the latest O2: `aliBuild init O2@dev --defaults o2`          


Obtain the QC:  `git clone -b PathLinkFix https://github.com/MYOMAO/QualityControl.git`     

Building O2: `aliBuild build O2 --defaults o2`    


Building QC: `aliBuild build flpproto --default o2`     


Entering QC environment: `alienv enter flpproto/latest`     

Copying O2 Geometry to QC: `cp QualityControl/O2geometry.root  O2/Detectors/ITSMFT/ITS/macros/test/`   

Opening the testing directory: `cd O2/Detectors/ITSMFT/ITS/macros/test/`      

Assuming you have the input raw data file, the code will run :  `qcRunDPL` 
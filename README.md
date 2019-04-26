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

#GUI 


Open a terminal and do: `ssh -L 8080:localhost:8080 aliceits@svmithi02`

Loading the working environment: `source /data/ITSQC/setQCenv.sh`

Open the QCG folder: `cd /data/zhaozhong/aliceQCG`

Entering the QCG environment: `alienv enter qcg/latest-o2-dataflow`

Run QCG: `qcg`



# QC 

Open a new terminal: `ssh -Y aliceits@svmithi02`

Loading the working environment: `source /data/ITSQC/setQCenv.sh`

Going to the QC directory: `cd /data/zhaozhong/alice`

Entering the environment: `alienv enter flpproto/latest`

Going to the working directory:  `cd O2/Detectors/ITSMFT/ITS/macros/test/`

Running the QC Command: `qcRunDPL`


# Viewing the Results of QC

Open a browser and go to the link: http://localhost:8080/


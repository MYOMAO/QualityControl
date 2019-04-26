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

## QC Environment Setup

First, we need to setup the environment: `source /data/ITSQC/setQCenv.sh`

## Check GUI Functionality 

Then, we need to check if the global GUI is running. To do this, open a browser and click on the following link:

`https://qcg-test.cern.ch`

If the GUI is running, you should see an interface with Object and Layout button. In this case, you can skip the local GUI setup and directly jump to the QC part.

If the GUI is not running, you should see the text '504 Gateway Time-out'. In this case, you need to setup your own local GUI


## Local GUI Setup


To set up the local GUI, simply login with port tunneling 

`ssh -L 8080:localhost:8080 aliceits@svmithi02`

and do `source StartLocalGUI.sh`



## QC Running

Simply do `source RunQC.sh`


## Viewing the Results of QC

If you run the global GUI, open a browser and go to the link: https://qcg-test.cern.ch
If you run your own local GUI, open a browser and go to the link: http://localhost:8080/


## Changing Files

To change your files, simply do modify the line named "infile" in the config (JSON) file:  


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

# QC on FLP

## Principles of the QC System

QC is called Quality Control. It reads raw data file, analyzes it, make histograms, and publish them to the database and GUI interface. The basic working principle of the QC is very simple. The QC first checks if there is a new folder for a new run. If there is a new folder for a new run, it will reset all histograms. If there is a new file, it will read the new file and update the histograms. When there is no new files or new folder, it will run in an infinite loop and never stops.

## Check if QC is running

In principle, QC is always running at FLP01 and you do not need to start the QC. To check if the QC is running, we can do the following steps:

### Step 1: Login to FLP01, go to the QC directory, and setup the QC enviornment

ssh -Y its@flpits1

cd  /home/its/msitta/run

Open a browser for the GUI: https://qcg-test.cern.ch/

You should be able to see a page with objects and layout on the top left corner


### Step 2: Check if the QC task exists

Now, you are in the work directory. First check if QC task exists. Do:

ps -A | grep qcRunDPL


You should see 5 qcRunDPL with different RunID and ? as the owner

33273 ?        00:00:01 qcRunDPL

33291 ?        00:01:10 qcRunDPL

33292 ?        00:07:36 qcRunDPL

33293 ?        00:10:10 qcRunDPL

33294 ?        00:10:07 qcRunDPL

If you do not see this, you need to restart the run. Go to the restarting QC section to see how to restart the QC


### Step 3: Check if the QC task runs properly

First, see what files are available in the folder "infiles/Run1"

ls infiles/Run1

You should see some files. For my case

Split2.bin  Split3.bin Split5.bin  Split9.bin

Now check the files in "tempmove/Run1". For my case

Split10.bin  Split11.bin  Split1.bin  Split6.bin  Split7.bin  Split8.bin

Now, you can copy a file from "tempmove/Run1". Make sure the filename copied to the infile/Run1 is DIFFERENT from the file names in infiles/Run1 

cp tempmove/Run1/Split6.bin infiles/Run1

Now go back to the GUI and go to the layout "Current File Processing". Wait for a 1 - 3 minutes and refresh the page

You should be able to see the title "Current File Name: infiles/Run1/Split6.bin" (depending on the name of the files you copied to Run1)

If you do see that, that means QC is running properly.

If you do not see the update of the filename to you copied file. You will need restart the QC. Go to the restarting QC section to see how to restart the QC


## Restarting the QC when it is not running


In case that QC is not running. To restart the QC when it is not running, simply do the following commands:

Open a new terminal

ssh -Y its@flpits1

cd  /home/its/msitta/

alienv enter QualityControl/latest

killall -9 qcRunDPL

qcRunDPL &!

Then you can close the terminal. In your original terminal, repeat step 2 and step 3. If QC still does not work, you will need to contact QC experts by emailing: zzshi@mit.edu




## Using QC to display your own run

If the QC is indeed running, you can do the following to display your own run

### Scenario 1: Test File in a Run

cp infiles/Run1/Split8.bin tempmove/Run1

Wait for about 1 minutes, you should be able to see the histogram of that run uploaded to the database: http://ccdb-test.cern.ch:8080/browse/ITSQcTask
And can be found on the GUI: https://qcg-test.cern.ch/?page=layoutList


### Scenario 2: Test New Runs

cp -r infiles/Run2/ tempmove/ 

Again, after 1 minute, you should see a set of new histograms uploaded to the database and GUI. These histograms have been reset and start reading the files in the new runs from empty histograms.


### Scenario 3. In Real IB Commissioning and Data Taking

In real data taking, all we need to do is to name the Runs by Run + RunID + Extra Info. The name of the files in the run in principle does not matter as long as the file format are raw data. The QC codes currently still have some issues such as uploading increasingly many plots to the database and memory leakage which will result in slowing and eventually stopping the loop. These will be fixed very soon. 


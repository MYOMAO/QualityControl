[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/dashboard/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)


# QC on FLP

## Basic principles of the QC system

The ITS Quality Control during the commissioning at B167 reads raw data files, perform the analysis, and publish them to the database and GUI interface. At present QC runs as a service and checks if there is a new folder for a new run in a dedicated directory. If there is a new folder Run*, the histograms will be reset reset. The string * in the folder name is being used to tag the processed data in the database. If a new data file is detected in the folder, the data will be processed and the histograms updated. 

## Using QC to display data files

In principle, QC is always running at FLP01 and you do not need to start the QC. To check if the QC is running, we can do the following steps:

### Step 1: Login to FLP01, go to the QC directory, and setup the QC enviornment

ssh -Y its@flpits1

cd  /home/its/msitta/run

Create in folder  /home/its/msitta/run/infiles/ a folder named  Run + RunID + Extra Info. Copy the raw data files to the new created folder. 

Repeat the last operation for each run. Please consider that the QC will move the processing to the last folder to be created. The process of the eventual remaining files to be processed in an earlier run will be overwritten. 

 
Wait for about 1 minutes, you should be able to see the histogram of that run uploaded to the database: http://ccdb-test.cern.ch:8080/browse/ITSQcTask and can be found on the GUI: https://qcg-test.cern.ch/?page=layoutList

 
Some test data are already available in the /home/its/msitta/run/infiles/. To have them processed again it is enough to copy the run folder our of the infiele directory and copy it back again after > 1 min.


In case in which the GUI doesn?t update directly some troubleshooting can be done:


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


## Restarting


In case that QC is not running. To restart the QC when it is not running, simply do the following commands:

Open a new terminal

ssh -Y its@flpits1

cd  /home/its/msitta/

alienv enter QualityControl/latest

killall -9 qcRunDPL

qcRunDPL &!

Then you can close the terminal. In your original terminal, repeat step 2 and step 3. If QC still does not work, you will need to contact QC experts by emailing: zzshi@mit.edu

Changing the working folder

To current default working folder on FLP01 is /data/QC/infiles. If you want to change the working folder. You will need to change the part called "infile": "/data/QC/infiles", in the config file: sw/slc7_x86-64/QualityControl/IBContinuous-1/etc/PrintTest.json.  


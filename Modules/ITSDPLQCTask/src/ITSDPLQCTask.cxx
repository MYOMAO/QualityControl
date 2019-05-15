///
/// \file   ITSDPLQCTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "ITSDPLQCTask/ITSDPLQCTask.h"

#include <sstream>

#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"

#include "ITSMFTReconstruction/DigitPixelReader.h"
#include "DetectorsBase/GeometryManager.h"
#include <TCanvas.h>
#include <iostream>
#include <dirent.h> 
#include <stdio.h> 
#include <algorithm>
#include <iterator>
#include <chrono>
#include <thread>




using o2::itsmft::Digit;



using namespace std;
using namespace o2::itsmft;
using namespace o2::ITS;



namespace o2
{
	namespace quality_control_modules
	{
		namespace itsdplqctask
		{

			ITSDPLQCTask::ITSDPLQCTask() : TaskInterface(), mHistogram(nullptr) { 
				mHistogram = nullptr;

				gStyle->SetPadRightMargin(0.15);
				gStyle->SetPadLeftMargin(0.15);
				o2::base::GeometryManager::loadGeometry ();


				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);

				ChipStave->GetXaxis()->SetTitle("Chip ID");
				ChipStave->GetYaxis()->SetTitle("Number of Hits");
				ChipStave->SetTitle("Number of Hits vs Chip ID for Stave 1 at Layer 1");


				for(int i = 0; i < NLayer; i++){

					NChipLay[i] = ChipBoundary[i + 1] - ChipBoundary[i];

					OccupancyPlot[i]	= new TH1D(Form("Occupancy%d",i),Form("Occupancy%d",i),NEventMax[i],0,NEventMax[i]); 
					OccupancyPlot[i]->GetXaxis()->SetTitle ("Occupancy");
					OccupancyPlot[i]->GetYaxis()->SetTitle ("Counts");
					OccupancyPlot[i]->GetYaxis()->SetTitleOffset(2.2);	
					OccupancyPlot[i]->SetTitle(Form("Occupancy Distribution for ITS Layer %d",i));

					LayEtaPhi[i] = new TH2D(Form("Lay1EtaPhiLay%d",i),Form("Lay1EtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhi[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhi[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhi[i]->GetZaxis()->SetTitle("Number of Hits");
					LayEtaPhi[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhi[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhi[i]->SetTitle(Form("Number of Hits for Layer %d #eta and #phi Distribution",i));


					NStaveChip[i] = NChipLay[i]/NStaves[i];
					NColStave[i] = NStaveChip[i] * NColHis;

					LayChipStave[i] = new TH2D(Form("LayChipStave%d",i),Form("LayChipStave%d",i),NStaveChip[i],0,NStaveChip[i],NStaves[i],0,NStaves[i]);
					LayChipStave[i]->GetXaxis()->SetTitle("Chip Number");
					LayChipStave[i]->GetYaxis()->SetTitle("Stave Number");
					LayChipStave[i]->GetZaxis()->SetTitle("Number of Hits");
					LayChipStave[i]->GetZaxis()->SetTitleOffset(1.4);
					LayChipStave[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayChipStave[i]->SetTitle(Form("Number of Hits for Layer %d Chip Number and Stave Number Distribution",i));


				}


				for(int i = 0; i < NError; i++){
					Error[i] = 0;
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIG[i] = new TH2D(Form("HICMAPLay%dStave%d",j,i),Form("HICMAPLay%dStave%d",j,i),NColHis*NStaveChip[j],0,NColHis*NStaveChip[j],NRowHis,0,NRowHis);
						//		Lay1HIG[i] = new TH2D(Form("HICMAPLay%dStave%d",j,i),Form("HICMAPLay%dStave%d",j,i),100,0,NColHis*NStaveChip[j],100,0,NRowHis);
						Lay1HIG[i]->GetXaxis()->SetTitle("Column");
						Lay1HIG[i]->GetYaxis()->SetTitle("Row");
						Lay1HIG[i]->GetYaxis()->SetTitleOffset(1.10);
						Lay1HIG[i]->GetZaxis()->SetTitleOffset(1.50);
						Lay1HIG[i]->SetTitle(Form("Hits Map on Layer %d Stave %d",j,i));
					}
				}

				ErrorPlots->GetXaxis()->SetTitle("Error ID");
				ErrorPlots->GetYaxis()->SetTitle("Counts");
				ErrorPlots->SetTitle("Error Checked During Decoding");
				ErrorPlots->SetMinimum(0);
				cout << "DONE 1" << endl;
				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HIGMAP[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),NColHis,0,NColHis,NRowHis,0,NRowHis);
						//		HIGMAP[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis,100,0,NRowHis);
						HIGMAP[i]->GetXaxis()->SetTitle("Column");
						HIGMAP[i]->GetYaxis()->SetTitle("Row");
						HIGMAP[i]->GetYaxis()->SetTitleOffset(1.10);
						HIGMAP[i]->GetZaxis()->SetTitleOffset(1.50);
						HIGMAP[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Number % d on Layer %d",i,j));
					}
				}
				cout << "DONE 2" << endl;

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HIGMAP6[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),NColHis*11,0,NColHis*11,NRowHis,0,NRowHis);
						//		HIGMAP6[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis*11,100,0,NRowHis);
						HIGMAP6[i]->GetXaxis()->SetTitle("Column");
						HIGMAP6[i]->GetYaxis()->SetTitle("Row");
						HIGMAP6[i]->GetYaxis()->SetTitleOffset(1.10);
						HIGMAP6[i]->GetZaxis()->SetTitleOffset(1.50);
						HIGMAP6[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Sector Number % d on Layer %d",i,j));
					}
				}
				cout << "DONE 3" << endl;

				HIGMAP[6]->SetMaximum(2);	
				HIGMAP[6]->SetMinimum(0);	
				cout << "Clear " << endl;


				//Initiate Looping Files//


				FolderNames = GetFName(workdir);

				cout << "NFolder = " << FolderNames.size() << endl;
				for (int i = 0; i < FolderNames.size(); i++){

					cout << "FDN = " << FolderNames[i] << endl;

					FileNames.push_back(GetFName(FolderNames[i]));

					cout << "FDN File Size = " << FileNames[i].size() << endl;


					for(int j = 0; j < FileNames[i].size(); j++){

						cout << "FDN File = " << FileNames[i][j] << endl;

					}

				}






			}

			ITSDPLQCTask::~ITSDPLQCTask() {
				if (mHistogram) {
					delete mHistogram;
				}
			}

			//void ITSDPLQCTask::initialize(o2::framework::InitContext& ctx)
			void ITSDPLQCTask::initialize(o2::framework::InitContext& ctx,std::string infile)
			{
				QcInfoLogger::GetInstance() << "initialize ITSDPLQCTask" << AliceO2::InfoLogger::InfoLogger::endm;
				QcInfoLogger::GetInstance() << "infile is fucking = " << infile << AliceO2::InfoLogger::InfoLogger::endm;
				//inpName = infile;
				bool mRawDataMode = 1;
				if (mRawDataMode)
				{
					int a = 1;
				}
				else
				{				// clusterizer of digits needs input from the FairRootManager (at the moment)
					mReaderMC = std::make_unique < o2::itsmft::DigitPixelReader > ();
					mReader = mReaderMC.get ();
				}

				LOG (INFO) << "It WORK, PASS 1";

				o2::ITS::GeometryTGeo * geom = o2::ITS::GeometryTGeo::Instance ();
				geom->fillMatrixCache (o2::utils::bit2Mask (o2::TransformType::L2G));	
				numOfChips = geom->getNumberOfChips ();
				cout << "numOfChips = " << numOfChips << endl;
				setNChips (numOfChips);
				cout << "START LOOPING BR getObjectsManager()->startPublishingO" << endl;
				mReaderRaw.setPadding128(true);
				mReaderRaw.setVerbosity(0);
				mReaderRaw.setMinTriggersToCache(256);




				mHistogram = new TH1F("example", "example", 20, 0, 30000);
				//	getObjectsManager()->addCheck(mHistogram, "checkFromITSDPLQCTask", "o2::quality_control_modules::itsdplqctask::ITSDPLQCTaskCheck",	"QcITSDPLQCTask");
				//getObjectsManager()->addCheck(ChipStave, "checkFromITSDPLQCTask", "o2::quality_control_modules::itsdplqctask::ITSDPLQCTaskCheck",	"QcITSDPLQCTask");

			}




			std::vector<string> ITSDPLQCTask::GetFName(std::string folder)
			{

				DIR           *dirp;
				struct dirent *directory;

				char cstr[folder.size()+1];
				strcpy(cstr, folder.c_str());
				dirp = opendir(cstr);
				std::vector<string> names;
				//string search_path = folder + "/*";
				if(dirp){

					while((directory = readdir(dirp)) != NULL){

						//printf("%s\n", directory->d_name);

						if ( !(!strcmp(directory->d_name, ".") || !strcmp(directory->d_name, ".."))) names.push_back(folder + "/" + directory->d_name);

					}

					closedir(dirp);
				}

				cout << "names size = " << names.size() << endl;
				return(names);
			}


			void ITSDPLQCTask::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

			void ITSDPLQCTask::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSDPLQCTask::monitorData(o2::framework::ProcessingContext& ctx)
			{
					


					
					//Getting Latest Folders//


					cout << "------------------------------------------------------------------------------------------" << endl;
					cout << "------------------------------------------------------------------------------------------" << endl;

					cout << "New Cycle" << endl;	

					cout << "Wake Up Bro" << endl;

					cout << "Old Folder Size = " << FolderNames.size() << endl;


					NowFolderNames = GetFName(workdir);



					cout << "Now NFolder = " << NowFolderNames.size() << endl;
					for (int i = 0; i < NowFolderNames.size(); i++){

						//	cout << "FDN = " << NowFolderNames[i] << endl;

						NowFileNames.push_back(GetFName(NowFolderNames[i]));

						//cout << "Now FDN File Size = " << NowFileNames[i].size() << endl;


						for(int j = 0; j < NowFileNames[i].size(); j++){

							//	cout << "Now FDN File = " << NowFileNames[i][j] << endl;

						}

					}

					std::set_difference(NowFolderNames.begin(), NowFolderNames.end(), FolderNames.begin(), FolderNames.end(),std::inserter(DiffFolderName, DiffFolderName.begin()));

					cout << "Difference Size Between New and Initial Runs = " <<   DiffFolderName.size() << endl;


					if( DiffFolderName.size() == 0 ){
						cout << "No New Run -- No Need to Fucking Reset" << endl;
						ResetCommand = 0;
					}


					if( DiffFolderName.size() > 0){
						cout << "New Run Started -- Reset All Histograms" << endl;
						ResetCommand = 1;	
						LOG(INFO) << "DONE Reset Histogram Decision";
						ITSDPLQCTask::reset();
						LOG(INFO) << "Decision DONE";
						ResetCommand = 0;
					}






					LOG(INFO) << "Start Creating New Now Vector";




					LOG(INFO) << "Get IN LOOP";
					for(int i = 0;  i < FolderNames.size(); i++){
						std::set_difference(NowFileNames[i].begin(), NowFileNames[i].end(), FileNames[i].begin(), FileNames[i].end(),std::inserter(DiffFileNamePush, DiffFileNamePush.begin()));
						DiffFileNames.push_back(DiffFileNamePush);
						cout << "Difference File Size Between New and Initial Runs " <<   DiffFileNames[i].size() << endl;
						DiffFileNamePush.clear();
					}

					LOG(INFO) << "DONE GRABING Existing";

					for(int i = FolderNames.size();  i < NowFolderNames.size(); i++){
						DiffFileNames.push_back(NowFileNames[i]);
						cout << "New File Size Between New and Initial Runs " <<   DiffFileNames[i].size() << endl;
					}	

					LOG(INFO) << "DONE Creating Difference";





					for (int i = 0; i < NowFolderNames.size(); i++){


						for(int j = 0; j < DiffFileNames[i].size(); j++){

							inpName = DiffFileNames[i][j];

							LOG(INFO) << "inpName = " << inpName;

							mReaderRaw.openInput (inpName);
							

							process (mReaderRaw);


							LOG(INFO) << "DONE HIS Update = ";
						
							mReaderRaw.clear(true);
							LOG(INFO) << "READER RESET = ";

	
							ChipStave->SetMinimum(1);

							getObjectsManager()->startPublishing(ChipStave);

							//TLegend* l = new TLegend(0.15,0.50,0.90,0.90);
							for(int i =0; i< NError;i++){
								cout << "i = " << i << "  Error Number = " << Error[i] << endl;
								ErrorPlots->SetBinContent(i+1,Error[i]);
								pt[i] = new TPaveText(0.20,0.80 -i*0.05,0.85,0.85-i*0.05,"NDC");
								pt[i]->SetTextSize(0.04);
								pt[i]->SetFillColor(0);
								pt[i]->SetTextAlign(12);
								pt[i]->AddText(ErrorType[i].Data());
								ErrorPlots->GetListOfFunctions()->Add(pt[i]);
							}

							ErrorMax = ErrorPlots->GetMaximum();
							ErrorPlots->SetMaximum(ErrorMax * 4.1+1000);
							getObjectsManager()->startPublishing(ErrorPlots);


							for(int j = 0; j < NLayer; j++){ 
								OccupancyPlot[j]->SetMarkerStyle (22);
								OccupancyPlot[j]->SetMarkerSize (1.5);
								OccupancyPlot[j]->Draw ("ep");
								cout << "Occupancy Total = " << OccupancyPlot[j]->Integral() << endl;

							}


							for(int j = 0; j < NLayer; j++){ 
								LayEtaPhi[j]->Draw("COLZ");
								cout << "Eta Phi Total = " << 	LayEtaPhi[j]->Integral() << endl;

							}

							for(int j = 0; j < NLayer; j++){ 
								LayChipStave[j]->Draw("COLZ");
								cout << "LayChipStave Total = " << 	LayChipStave[j]->Integral() << endl;
							}


							for(int j = 0; j < 1; j++){
								for(int i = 0; i < NStaveChip[j]; i++){
									HIGMAP[i]->GetZaxis()->SetTitle("Number of Hits");
									HIGMAP[i]->GetXaxis()->SetNdivisions(-32);
									HIGMAP[i]->Draw("COLZ");
									ConfirmXAxis(HIGMAP[i]);
									ReverseYAxis(HIGMAP[i]);
									getObjectsManager()->startPublishing(HIGMAP[i]);
								}
							}

							ReverseYAxis(HIGMAP[0]);

							for(int j = 0; j < 1; j++){
								for(int i = 0; i < NStaves[j]; i++){

									Lay1HIG[i]->GetZaxis()->SetTitle("Number of Hits");
									Lay1HIG[i]->GetXaxis()->SetNdivisions(-32);
									Lay1HIG[i]->Draw("COLZ");
									ConfirmXAxis(Lay1HIG[i]);
									ReverseYAxis(Lay1HIG[i]);
									getObjectsManager()->startPublishing(Lay1HIG[i]);
								}
							}




							for(int j = 6; j < 7; j++){
								for(int i = 0; i < 18; i++){
									HIGMAP6[i]->GetZaxis()->SetTitle("Number of Hits");
									HIGMAP6[i]->GetXaxis()->SetNdivisions(-32);
									HIGMAP6[i]->Draw("COLZ");
									ConfirmXAxis(HIGMAP6[i]);
									ReverseYAxis(HIGMAP6[i]);	
									getObjectsManager()->startPublishing(HIGMAP6[i]);
								}
							}



							for(int i = 0; i < NLayer; i++){


								getObjectsManager()->startPublishing(LayEtaPhi[i]);
								getObjectsManager()->startPublishing(LayChipStave[i]);
								getObjectsManager()->startPublishing(OccupancyPlot[i]);


							}

						}
					}


					LOG(INFO) << "START Updateing Vectors";


					FolderNames.clear();
					FileNames.clear();

					FolderNames = NowFolderNames;
					FileNames = NowFileNames;

					LOG(INFO) << "DONE Updateing Vectors";





					NowFolderNames.clear();
					NowFileNames.clear();
					DiffFileNames.clear();
					DiffFolderName.clear();


					LOG(INFO) << "Pushing Reset Histogram Decision";

					LOG(INFO) << "Start Ending Cycle";


				
					cout << "Start Sleeping Bro" << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;
					cout << " " << endl;	

					std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			}


			void ITSDPLQCTask::process (PixelReader & reader){
				cout << "RESET READER" << endl;
				mChips.clear();
				mChipData = nullptr; 
				using RawReader=o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS>;
				auto &rawReader = reinterpret_cast<RawReader&>(reader);
				cout << "START PROCESSING" << endl;

				int Index = 0;
				int IndexMax = -1;
				int TimeFrame = 1;

				/*
				   cout << "Counting Time Frame" << endl;
				   while ((mChipData = reader.getNextChipData (mChips)))
				   {
				   TimeFrame = TimeFrame + 1;
				   }
				   */
				cout << "TimeFrame = " << TimeFrame << endl;

				cout << "START MCHIPDATA" << endl;


				while ((mChipData = reader.getNextChipData (mChips)))
				{
		//			cout << " reader.getNextChipData (mChips) = " <<  reader.getNextChipData (mChips) << endl;

					if(Index < IndexMax) break;
					//      cout << "ChipID Before = " << ChipID << endl; 
					ChipID = mChipData->getChipID ();
					mReaderRaw.getMapping().getChipInfoSW( ChipID, chipInfo );
					//const auto& statRU =  mReaderRaw.getRUDecodingStatSW( chipInfo.ru );

					const auto* ruInfo = rawReader.getCurrRUDecodeData()->ruInfo;	
					const auto& statRU =  rawReader.getRUDecodingStatSW( ruInfo->idSW );
					//printf("ErrorCount: %d\n", (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrPageCounterDiscontinuity] );

					//Error[0] = Error[0]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrGarbageAfterPayload];
					Error[0] = Error[0]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrPageCounterDiscontinuity];
					Error[1] = Error[1]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrRDHvsGBTHPageCnt];
					Error[2] = Error[2]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrMissingGBTHeader];
					Error[3] = Error[3]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrMissingGBTTrailer];
					Error[4] = Error[4]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrNonZeroPageAfterStop];
					Error[5] = Error[5]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrUnstoppedLanes];  
					Error[6] = Error[6]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrDataForStoppedLane];
					Error[7] = Error[7]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrNoDataForActiveLane];
					Error[8] = Error[8]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrIBChipLaneMismatch];
					Error[9] = Error[9]  + (int)statRU->errorCounts[o2::itsmft::GBTLinkDecodingStat::ErrCableDataHeadWrong];

					//cout << "Error 5 = " << Error[5]  << endl;

					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					const Point3D<float> loc(0., 0.,0.); 
					auto glo = gm->getMatrixL2G(ChipID)(loc);

					if (lay < 7)
					{
						//cout << "lay = " <<  lay << endl;
						//cout << "ChipID = " << ChipID << endl;

						int ChipNumber = (ChipID - ChipBoundary[lay])- sta*	NStaveChip[lay];
						ActPix = mChipData->getData().size();
						AveActPix = ActPix/TimeFrame;

						//cout << "ChipNumber = " << ChipNumber << endl;
						eta = glo.eta();
						phi = glo.phi();
						//			Occupancy[ChipID] = Occupancy[ChipID] + ActPix;
						//if(ActPix > 0 ) cout << "Chip ID = " << ChipID << "   Occupancy = " << ActPix << endl;
						OccupancyPlot[lay]->Fill(AveActPix);
						ChipStave->Fill(ChipID, AveActPix);
						LayEtaPhi[lay]->Fill(eta,phi,AveActPix);
						LayChipStave[lay]->Fill(ChipNumber,sta,AveActPix);
						if(sta == 0  && ChipID < NLay1){
							//cout << "ChipID in Stave 0/data/zhaozhong/aliceTest/sw/BUILD/QualityControl-latest/QualityControl = " << ChipID << endl; 
							for(int ip = 0; ip < ActPix; ip++){
								const auto pix = mChipData->getData()[ip];
								row = pix.getRow();
								col = pix.getCol();
								if(row > 0 && col > 0) HIGMAP[ChipID]->Fill(col,row);

							}	
						}

						if(lay == 0){
							//cout << "ChipID in Stave 0 = " << ChipID << endl; 
							for(int ip = 0; ip < ActPix; ip++){
								const auto pix = mChipData->getData()[ip];
								row = pix.getRow();
								col = pix.getCol() + NColHis * ChipNumber;
								if(row > 0 && col > 0) Lay1HIG[sta]->Fill(col,row);

							}	
						}

						if(sta == 0 && lay == 6){
							//cout << "ChipID in Stave 0 = " << ChipID << endl; 
							for(int ip = 0; ip < ActPix; ip++){
								const auto pix = mChipData->getData()[ip];
								ChipIndex6 = ChipNumber/11; 
								int ChipLocal6 = ChipNumber - ChipIndex6 * 11;
								if(ChipLocal6 < 0 ) ChipLocal6 = ChipNumber - (ChipIndex6 -1) * 11;

								row = pix.getRow();
								col = pix.getCol() + ChipLocal6 * NColHis;

								//cout << "ChipNumber = " <<  ChipNumber << "   ChipIndex6 = " <<  ChipIndex6 << "   ChipLocal6 = " << ChipLocal6 << endl;
								if(row > 0 && col > 0) HIGMAP6[ChipIndex6]->Fill(col,row);
							}
						}

					}
					Index = Index + 1;
				}
				cout << "Start Filling" << endl;
				cout << "mChips Size = " << mChips.size() << endl;
				/*
				   for(int j = 0; j < NLayer; j++){ 
				   for (int i = ChipBoundary[j]; i < ChipBoundary[j+1]; i++)
				   {
				//int XBin = ChipStave[j]->GetXaxis()->FindBin(i);
				//AveOcc = Occupancy[i]/NPixels;
				cout << "i = " << i << "   Occupancy = " << Occupancy[i] << endl;
				ChipStave[lay]->Fill(i, Occupancy[i]);

				}
				}
				*/
			}

			void ITSDPLQCTask::ConfirmXAxis(TH1 *h)
			{
				// Remove the current axis
				h->GetXaxis()->SetLabelOffset(999);
				h->GetXaxis()->SetTickLength(0);
				// Redraw the new axis
				gPad->Update();
				XTicks = (h->GetXaxis()->GetXmax()-h->GetXaxis()->GetXmin())/DivisionStep;

				TGaxis *newaxis = new TGaxis(gPad->GetUxmin(),
						gPad->GetUymin(),
						gPad->GetUxmax(),
						gPad->GetUymin(),
						h->GetXaxis()->GetXmin(),
						h->GetXaxis()->GetXmax(),
						XTicks,"N");
				newaxis->SetLabelOffset(0.0);
				newaxis->Draw();
				h->GetListOfFunctions()->Add(newaxis);	
			}
			void ITSDPLQCTask::ReverseYAxis(TH1 *h)
			{
				// Remove the current axis
				h->GetYaxis()->SetLabelOffset(999);
				h->GetYaxis()->SetTickLength(0);

				// Redraw the new axis
				gPad->Update();

				YTicks = (h->GetYaxis()->GetXmax()-h->GetYaxis()->GetXmin())/DivisionStep;
				TGaxis *newaxis = new TGaxis(gPad->GetUxmin(),
						gPad->GetUymax(),
						gPad->GetUxmin()-0.001,
						gPad->GetUymin(),
						h->GetYaxis()->GetXmin(),
						h->GetYaxis()->GetXmax(),
						YTicks,"N");

				newaxis->SetLabelOffset(0);
				newaxis->Draw();
				h->GetListOfFunctions()->Add(newaxis);

			}




			void ITSDPLQCTask::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSDPLQCTask::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSDPLQCTask::reset()
			{
				// clean all the monitor objects here

				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
				ChipStave->Reset();
				for(int i = 0; i < NLayer; i++){
					OccupancyPlot[i]->Reset();
					LayEtaPhi[i]->Reset();
					LayChipStave[i]->Reset();
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIG[i]->Reset();
					}
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HIGMAP[i]->Reset();
					}
				}

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HIGMAP6[i]->Reset();
					}
				}
				ErrorPlots->Reset();

				QcInfoLogger::GetInstance() << "DONE the histogram Resetting" << AliceO2::InfoLogger::InfoLogger::endm;


				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

		} // namespace itsdplqctask
	} // namespace quality_control_modules
} // namespace o2


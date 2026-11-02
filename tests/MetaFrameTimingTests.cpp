#include "meta_bridge/FrameTiming.h"
#include <iostream>
#include <stdexcept>
using diagnostic::meta_bridge::FrameTiming;
void Check(bool b,char const* message){if(!b)throw std::runtime_error(message);}
int main(){try{
 FrameTiming off;off.Waited(0,1,1,12500000,false,0);off.Submitted(2,3,4,true,true);Check(!off.Count()&&off.Report().empty(),"disabled timing recorded frames");
 FrameTiming t;t.Enable(0);
 for(int n=0;n<160;++n){double start=1+n*12.5;t.Waited(start,start+5,(n+1)*12500000LL,12500000,true,start+12.5);t.Prefetched(start-5,start+2,start);t.Acquired(.1);t.Submitted(start+11,start+11.5,start+11.6,true,true);}
 Check(t.ReportDue(2015),"two-second report missing");auto r=t.Report();
 Check(r.find("\"app_submit_fps\":80.000")!=r.npos,"submission FPS is wrong");
 Check(r.find("\"application_and_ipc_ms_mean\":6.000")!=r.npos,"application span is wrong");
 Check(r.find("\"copy_sync_ms_mean\":0.500")!=r.npos,"copy span is wrong");
 Check(r.find("\"native_wait_ms_mean\":7.000")!=r.npos&&r.find("\"overlapped_wait_ms_mean\":5.000")!=r.npos,"overlapped wait attribution is wrong");
 Check(r.find("\"before_wait_ms_mean\":0.900")!=r.npos,"before-wait application span is wrong");
 Check(r.find("\"submitted_after_predicted_display\":0")!=r.npos,"early frame marked late");
 Check(!t.Count(),"window did not reset");
 t.Waited(2200,2201,2025000000LL,12500000,true,2202);t.Submitted(2203,2204,2205,true,true);r=t.Report();
 Check(r.find("\"submitted_after_predicted_display\":1")!=r.npos,"late submission not recorded");
 Check(r.find("\"skipped_prediction_slots\":1")!=r.npos,"skipped prediction slot not recorded");
 t.Waited(2300,2301,2037500000LL,12500000,false,0);t.Submitted(2302,2303,2304,false,true);
 Check(!t.Count(),"UI-only frame counted as stereo");
 t.Waited(2400,2401,2050000000LL,12500000,false,0);t.Submitted(2402,2403,2404,true,true);r=t.Report();
 Check(r.find("\"app_submit_fps\":0.000")!=r.npos,"pause included in FPS");
 t.Waited(120001,120002,0,12500000,false,0);Check(!t.Enabled(),"one-shot capture did not expire");
 std::cout<<"PASS Meta timing: pacing, phases, lateness, pause exclusion, bounded capture\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
